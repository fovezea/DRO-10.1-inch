module uart_rx #(
    parameter CLK_FREQ = 24000000,
    parameter BAUD_RATE = 9600
)(
    input wire clk,
    input wire reset,
    input wire rx_in,
    output reg [7:0] rx_byte,
    output reg rx_dv
);

    localparam CLKS_PER_BIT = CLK_FREQ / BAUD_RATE;
    localparam IDLE = 0;
    localparam START = 1;
    localparam DATA = 2;
    localparam STOP = 3;

    reg [2:0] state;
    reg [15:0] clk_count;
    reg [2:0] bit_index;
    reg [7:0] shift_reg;
    
    // Synchronize RX input
    reg rx_sync, rx_sync_d;
    always @(posedge clk) begin
        rx_sync <= rx_in;
        rx_sync_d <= rx_sync;
    end

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            state <= IDLE;
            clk_count <= 0;
            bit_index <= 0;
            rx_byte <= 0;
            rx_dv <= 0;
        end else begin
            rx_dv <= 0; // Default
            
            case (state)
                IDLE: begin
                    clk_count <= 0;
                    bit_index <= 0;
                    if (rx_sync_d == 0) begin // Start bit detected (falling edge)
                        state <= START;
                    end
                end
                
                START: begin
                    // Wait half bit time to sample middle of start bit
                    if (clk_count == (CLKS_PER_BIT / 2)) begin
                        if (rx_sync_d == 0) begin
                            clk_count <= 0;
                            state <= DATA;
                        end else begin
                            state <= IDLE; // False start
                        end
                    end else begin
                        clk_count <= clk_count + 1;
                    end
                end
                
                DATA: begin
                    if (clk_count == CLKS_PER_BIT) begin
                        clk_count <= 0;
                        shift_reg[bit_index] <= rx_sync_d;
                        if (bit_index < 7) begin
                            bit_index <= bit_index + 1;
                        end else begin
                            bit_index <= 0;
                            state <= STOP;
                        end
                    end else begin
                        clk_count <= clk_count + 1;
                    end
                end
                
                STOP: begin
                    if (clk_count == CLKS_PER_BIT) begin
                        state <= IDLE;
                        rx_byte <= shift_reg;
                        rx_dv <= 1;
                    end else begin
                        clk_count <= clk_count + 1;
                    end
                end
            endcase
        end
    end

endmodule
