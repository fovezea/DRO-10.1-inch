module heartbeat_gen (
    input wire clk,
    input wire reset,
    
    // Status Inputs
    input wire enable_1,
    input wire enable_2,
    input wire signed [31:0] phase_err_1,
    input wire signed [31:0] phase_err_2,
    
    // UART Interface
    output reg tx_wr_en,
    output reg [7:0] tx_byte,
    input wire tx_ready // Connected to !fifo_full
);

    parameter CLK_FREQ = 25000000;
    parameter HEARTBEAT_MS = 100;
    localparam TIMER_LIMIT = (CLK_FREQ / 1000) * HEARTBEAT_MS;

    // Packet Structure (16 Bytes)
    // 0: 0xAA (Header 1)
    // 1: 0x55 (Header 2)
    // 2: 0x0A (Length = 10)
    // 3: 0x20 (Cmd = Status)
    // 4: Enable 1
    // 5: Enable 2
    // 6-9: Phase Err 1 (Big Endian)
    // 10-13: Phase Err 2 (Big Endian)
    // 14: CRC
    // 15: 0x0A (Footer)

    reg [31:0] timer;
    reg [4:0] byte_index;
    reg [7:0] crc;
    reg sending;
    
    // CRC-8 Polynomial 0x07
    function [7:0] calc_crc;
        input [7:0] data;
        input [7:0] current_crc;
        integer i;
        reg [7:0] temp;
        begin
            temp = current_crc ^ data;
            for (i = 0; i < 8; i = i + 1) begin
                if (temp[7])
                    temp = (temp << 1) ^ 8'h07;
                else
                    temp = temp << 1;
            end
            calc_crc = temp;
        end
    endfunction

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            timer <= 0;
            sending <= 0;
            byte_index <= 0;
            tx_wr_en <= 0;
            tx_byte <= 0;
            crc <= 0;
        end else begin
            tx_wr_en <= 0; // Default
            
            if (!sending) begin
                if (timer >= TIMER_LIMIT) begin
                    timer <= 0;
                    sending <= 1;
                    byte_index <= 0;
                    crc <= 0;
                end else begin
                    timer <= timer + 1;
                end
            end else begin
                // Sending State Machine - Writes to FIFO if Ready
                if (tx_ready) begin
                    case (byte_index)
                        0: begin tx_byte <= 8'hAA; tx_wr_en <= 1; end
                        1: begin tx_byte <= 8'h55; tx_wr_en <= 1; end
                        2: begin tx_byte <= 10;    tx_wr_en <= 1; crc <= calc_crc(10, crc); end
                        3: begin tx_byte <= 8'h20; tx_wr_en <= 1; crc <= calc_crc(8'h20, crc); end
                        
                        4: begin tx_byte <= {7'b0, enable_1}; tx_wr_en <= 1; crc <= calc_crc({7'b0, enable_1}, crc); end
                        5: begin tx_byte <= {7'b0, enable_2}; tx_wr_en <= 1; crc <= calc_crc({7'b0, enable_2}, crc); end
                        
                        6: begin tx_byte <= phase_err_1[31:24]; tx_wr_en <= 1; crc <= calc_crc(phase_err_1[31:24], crc); end
                        7: begin tx_byte <= phase_err_1[23:16]; tx_wr_en <= 1; crc <= calc_crc(phase_err_1[23:16], crc); end
                        8: begin tx_byte <= phase_err_1[15:8];  tx_wr_en <= 1; crc <= calc_crc(phase_err_1[15:8], crc); end
                        9: begin tx_byte <= phase_err_1[7:0];   tx_wr_en <= 1; crc <= calc_crc(phase_err_1[7:0], crc); end
                        
                        10: begin tx_byte <= phase_err_2[31:24]; tx_wr_en <= 1; crc <= calc_crc(phase_err_2[31:24], crc); end
                        11: begin tx_byte <= phase_err_2[23:16]; tx_wr_en <= 1; crc <= calc_crc(phase_err_2[23:16], crc); end
                        12: begin tx_byte <= phase_err_2[15:8];  tx_wr_en <= 1; crc <= calc_crc(phase_err_2[15:8], crc); end
                        13: begin tx_byte <= phase_err_2[7:0];   tx_wr_en <= 1; crc <= calc_crc(phase_err_2[7:0], crc); end
                        
                        14: begin tx_byte <= crc;  tx_wr_en <= 1; end
                        15: begin tx_byte <= 8'h0A; tx_wr_en <= 1; end
                        
                        16: begin sending <= 0; end // Done
                    endcase
                    
                    if (byte_index < 16)
                        byte_index <= byte_index + 1;
                end
            end
        end
    end

endmodule
