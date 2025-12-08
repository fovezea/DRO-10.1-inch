module uart_tx (
    input wire clk,
    input wire reset,
    input wire tx_start,
    input wire [7:0] tx_byte,
    output reg tx_active,
    output reg tx_done,
    output reg tx_line
);

    parameter CLK_FREQ = 25000000;
    parameter BAUD_RATE = 9600;
    
    localparam CLKS_PER_BIT = CLK_FREQ / BAUD_RATE;
    
    localparam IDLE = 0;
    localparam START = 1;
    localparam DATA = 2;
    localparam STOP = 3;
    
    reg [2:0] state;
    reg [15:0] clk_count;
    reg [2:0] bit_index;
    reg [7:0] data_temp;
    
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            state <= IDLE;
            tx_active <= 0;
            tx_done <= 0;
            tx_line <= 1; // Idle High
            clk_count <= 0;
            bit_index <= 0;
        end else begin
            tx_done <= 0; // Default
            
            case (state)
                IDLE: begin
                    tx_line <= 1;
                    if (tx_start) begin
                        state <= START;
                        tx_active <= 1;
                        data_temp <= tx_byte;
                        clk_count <= 0;
                    end else begin
                        tx_active <= 0;
                    end
                end
                
                START: begin
                    tx_line <= 0; // Start Bit
                    if (clk_count < CLKS_PER_BIT - 1) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        clk_count <= 0;
                        state <= DATA;
                        bit_index <= 0;
                    end
                end
                
                DATA: begin
                    tx_line <= data_temp[bit_index];
                    if (clk_count < CLKS_PER_BIT - 1) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        clk_count <= 0;
                        if (bit_index < 7) begin
                            bit_index <= bit_index + 1;
                        end else begin
                            state <= STOP;
                        end
                    end
                end
                
                STOP: begin
                    tx_line <= 1; // Stop Bit
                    if (clk_count < CLKS_PER_BIT - 1) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        clk_count <= 0;
                        state <= IDLE;
                        tx_active <= 0;
                        tx_done <= 1;
                    end
                end
                
                default: state <= IDLE;
            endcase
        end
    end

endmodule
