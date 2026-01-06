module top #(
    parameter CLK_FREQ = 100000000,
    parameter BAUD_RATE = 115200,
    parameter HEARTBEAT_MS = 100
) (
    input wire clk,     // System clock (100MHz for Spartan-7)
    input wire rst_n,   // Active low reset
    
    // Encoder Input
    input wire enc_A,
    input wire enc_B,
    
    // UART Interface
    input wire uart_rx,
    output wire uart_tx,
    
    // Motor Output Axis 1
    output wire step_out_1,
    output wire dir_out_1,
    
    // Motor Output Axis 2
    output wire step_out_2,
    output wire dir_out_2,

    // Status LED
    output wire led_test
);

    wire rst = ~rst_n;
    assign led_test = enable_1 | enable_2;
    
    // UART Configuration
    wire [15:0] num_1, den_1;
    wire [15:0] num_2, den_2;
    wire enable_1, enable_2;
    wire signed [31:0] phase_err_1, phase_err_2;
    
    // -----------------------------------------------------------
    // UART RX Path: uart_rx -> rx_fifo -> uart_config
    // -----------------------------------------------------------
    wire [7:0] rx_raw_byte;
    wire rx_raw_dv;
    
    uart_rx #(
        .CLK_FREQ(CLK_FREQ),
        .BAUD_RATE(BAUD_RATE)
    ) u_rx (
        .clk(clk),
        .reset(rst),
        .rx_in(uart_rx),
        .rx_byte(rx_raw_byte),
        .rx_dv(rx_raw_dv)
    );
    
    wire [7:0] rx_fifo_data;
    wire rx_fifo_empty;
    wire rx_fifo_full;
    wire rx_fifo_rd_en;
    
    fifo #(
        .DATA_WIDTH(8),
        .ADDR_WIDTH(4) // 16 bytes deep
    ) rx_fifo (
        .clk(clk),
        .reset(rst),
        .wr_en(rx_raw_dv),
        .wr_data(rx_raw_byte),
        .full(rx_fifo_full),
        .rd_en(rx_fifo_rd_en),
        .rd_data(rx_fifo_data),
        .empty(rx_fifo_empty)
    );
    
    // Drain RX FIFO immediately into Config
    assign rx_fifo_rd_en = !rx_fifo_empty;
    
    uart_config u_cfg (
        .clk(clk),
        .reset(rst),
        .rx_byte(rx_fifo_data),
        .rx_dv(rx_fifo_rd_en), // Valid when we read
        .num_1(num_1),
        .den_1(den_1),
        .num_2(num_2),
        .den_2(den_2),
        .enable_1(enable_1),
        .enable_2(enable_2)
    );
    
    // -----------------------------------------------------------
    // UART TX Path: heartbeat_gen -> tx_fifo -> uart_tx
    // -----------------------------------------------------------
    wire tx_fifo_wr_en;
    wire [7:0] tx_fifo_wr_data;
    wire tx_fifo_full;
    wire tx_fifo_empty;
    wire [7:0] tx_fifo_rd_data;
    reg tx_fifo_rd_en;
    
    // Heartbeat Generator (Producer)
    heartbeat_gen #(
        .CLK_FREQ(CLK_FREQ),
        .HEARTBEAT_MS(HEARTBEAT_MS)
    ) hb (
        .clk(clk),
        .reset(rst),
        .enable_1(enable_1),
        .enable_2(enable_2),
        .phase_err_1(phase_err_1),
        .phase_err_2(phase_err_2),
        .tx_wr_en(tx_fifo_wr_en),
        .tx_byte(tx_fifo_wr_data),
        .tx_ready(!tx_fifo_full)
    );
    
    fifo #(
        .DATA_WIDTH(8),
        .ADDR_WIDTH(5) // 32 bytes deep (enough for 16-byte packet + margin)
    ) tx_fifo (
        .clk(clk),
        .reset(rst),
        .wr_en(tx_fifo_wr_en),
        .wr_data(tx_fifo_wr_data),
        .full(tx_fifo_full),
        .rd_en(tx_fifo_rd_en),
        .rd_data(tx_fifo_rd_data),
        .empty(tx_fifo_empty)
    );

    // TX FIFO Consumer Logic
    reg uart_tx_start;
    wire uart_tx_active;
    wire uart_tx_done;
    reg tx_sending_flag;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            uart_tx_start <= 0;
            tx_fifo_rd_en <= 0;
            tx_sending_flag <= 0;
        end else begin
            // Pulse signals default low
            uart_tx_start <= 0;
            tx_fifo_rd_en <= 0;
            
            if (!tx_sending_flag && !tx_fifo_empty && !uart_tx_active) begin
                uart_tx_start <= 1; // Send current data at Head
                tx_fifo_rd_en <= 1; // Advance Head
                tx_sending_flag <= 1; // Wait for UART to acknowledge busy
            end
            
            // Clear flag when UART becomes busy or if it finishes (fast UART)
            if (tx_sending_flag && uart_tx_active) begin
                tx_sending_flag <= 0;
            end
        end
    end
    
    // UART Transmitter
    uart_tx #(
        .CLK_FREQ(CLK_FREQ),
        .BAUD_RATE(BAUD_RATE)
    ) u_tx (
        .clk(clk),
        .reset(rst),
        .tx_start(uart_tx_start),
        .tx_byte(tx_fifo_rd_data), // Data valid from FIFO FWFT
        .tx_active(uart_tx_active),
        .tx_done(uart_tx_done),
        .tx_line(uart_tx)
    );

    // -----------------------------------------------------------
    // Core Logic
    // -----------------------------------------------------------
    electronic_gearbox gearbox (
        .clk(clk),
        .reset(rst),
        .quad_A(enc_A),
        .quad_B(enc_B),
        .num_1(num_1),
        .den_1(den_1),
        .num_2(num_2),
        .den_2(den_2),
        .enable_1(enable_1),
        .enable_2(enable_2),
        .step_1(step_out_1),
        .dir_1(dir_out_1),
        .step_2(step_out_2),
        .dir_2(dir_out_2),
        .phase_err_1(phase_err_1),
        .phase_err_2(phase_err_2)
    );

endmodule
