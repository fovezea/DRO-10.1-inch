module uart_config (
    input wire clk,
    input wire reset,
    input wire [7:0] rx_byte,
    input wire rx_dv,
    
    // Axis 1 Config
    output reg [15:0] num_1,
    output reg [15:0] den_1,
    
    // Axis 2 Config
    output reg [15:0] num_2,
    output reg [15:0] den_2,
    
    // Enables
    output reg enable_1,
    output reg enable_2
);

    // Protocol: [0x55] [AxisID] [NumH] [NumL] [DenH] [DenL]
    // Protocol: [0x56] [AxisID] [Enable]
    
    localparam WAIT_SYNC = 0;
    localparam GET_AXIS = 1;
    localparam GET_NUM_H = 2;
    localparam GET_NUM_L = 3;
    localparam GET_DEN_H = 4;
    localparam GET_DEN_L = 5;
    localparam GET_ENABLE = 6;
    
    reg [7:0] cmd_type;

    reg [2:0] state;
    reg [7:0] axis_id;
    reg [15:0] temp_num;
    reg [15:0] temp_den;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            state <= WAIT_SYNC;
            num_1 <= 1; den_1 <= 1; enable_1 <= 1; // Default Enabled
            num_2 <= 1; den_2 <= 1; enable_2 <= 1;
        end else begin
            if (rx_dv) begin
                case (state)
                    WAIT_SYNC: begin
                        if (rx_byte == 8'h55 || rx_byte == 8'h56) begin
                            cmd_type <= rx_byte;
                            state <= GET_AXIS;
                        end
                    end
                    
                    GET_AXIS: begin
                        axis_id <= rx_byte;
                        if (cmd_type == 8'h56)
                            state <= GET_ENABLE;
                        else
                            state <= GET_NUM_H;
                    end
                    
                    GET_ENABLE: begin
                        if (axis_id == 0) enable_1 <= (rx_byte != 0);
                        else if (axis_id == 1) enable_2 <= (rx_byte != 0);
                        state <= WAIT_SYNC;
                    end
                    
                    GET_NUM_H: begin
                        temp_num[15:8] <= rx_byte;
                        state <= GET_NUM_L;
                    end
                    
                    GET_NUM_L: begin
                        temp_num[7:0] <= rx_byte;
                        state <= GET_DEN_H;
                    end
                    
                    GET_DEN_H: begin
                        temp_den[15:8] <= rx_byte;
                        state <= GET_DEN_L;
                    end
                    
                    GET_DEN_L: begin
                        temp_den[7:0] <= rx_byte;
                        
                        // Update specific axis
                        if (temp_num > 0 && (temp_den[15:8] > 0 || rx_byte > 0)) begin
                            if (axis_id == 0) begin
                                num_1 <= temp_num;
                                den_1 <= {temp_den[15:8], rx_byte};
                            end else if (axis_id == 1) begin
                                num_2 <= temp_num;
                                den_2 <= {temp_den[15:8], rx_byte};
                            end
                        end
                        state <= WAIT_SYNC;
                    end
                    
                    default: state <= WAIT_SYNC;
                endcase
            end
        end
    end

endmodule
