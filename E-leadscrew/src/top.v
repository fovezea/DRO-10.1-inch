module top #(
    parameter HEARTBEAT_MS = 100
) (
    input wire clk, // 25MHz onboard clock
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
    output wire dir_out_2
);

    wire rst = ~rst_n;
    
    // UART Configuration
    wire [7:0] rx_data;
    wire rx_valid;
    wire [15:0] num_1, den_1;
    wire [15:0] num_2, den_2;
    wire enable_1, enable_2;
    wire signed [31:0] phase_err_1, phase_err_2;
    
    // UART TX Signals
    wire tx_start, tx_active, tx_done;
    wire [7:0] tx_byte;
    
    uart_rx #(
        .CLK_FREQ(25000000),
        .BAUD_RATE(9600)
    ) u_rx (
        .clk(clk),
        .reset(rst),
        .rx_in(uart_rx),
        .rx_byte(rx_data),
        .rx_dv(rx_valid)
    );
    
    uart_config u_cfg (
        .clk(clk),
        .reset(rst),
        .rx_byte(rx_data),
        .rx_dv(rx_valid),
        .num_1(num_1),
        .den_1(den_1),
        .num_2(num_2),
        .den_2(den_2),
        .enable_1(enable_1),
        .enable_2(enable_2)
    );
    
    // Heartbeat Generator
    heartbeat_gen #(
        .HEARTBEAT_MS(HEARTBEAT_MS)
    ) hb (
        .clk(clk),
        .reset(rst),
        .enable_1(enable_1),
        .enable_2(enable_2),
        .phase_err_1(phase_err_1),
        .phase_err_2(phase_err_2),
        .tx_start(tx_start),
        .tx_byte(tx_byte),
        .tx_done(tx_done),
        .tx_active(tx_active)
    );
    
    // UART Transmitter
    uart_tx #(
        .CLK_FREQ(25000000),
        .BAUD_RATE(9600)
    ) u_tx (
        .clk(clk),
        .reset(rst),
        .tx_start(tx_start),
        .tx_byte(tx_byte),
        .tx_active(tx_active),
        .tx_done(tx_done),
        .tx_line(uart_tx)
    );

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
