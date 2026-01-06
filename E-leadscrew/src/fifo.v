module fifo #(
    parameter DATA_WIDTH = 8,
    parameter ADDR_WIDTH = 4 // 16 entries
)(
    input wire clk,
    input wire reset,
    
    // Write Interface
    input wire wr_en,
    input wire [DATA_WIDTH-1:0] wr_data,
    output wire full,
    
    // Read Interface
    input wire rd_en,
    output wire [DATA_WIDTH-1:0] rd_data,
    output wire empty
);

    reg [DATA_WIDTH-1:0] mem [0:(1<<ADDR_WIDTH)-1];
    reg [ADDR_WIDTH-1:0] wr_ptr;
    reg [ADDR_WIDTH-1:0] rd_ptr;
    reg [ADDR_WIDTH:0] count; // Extra bit to distinguish full/empty

    assign full = (count == (1 << ADDR_WIDTH));
    assign empty = (count == 0);
    assign rd_data = mem[rd_ptr];

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            wr_ptr <= 0;
            rd_ptr <= 0;
            count <= 0;
        end else begin
            // Write
            if (wr_en && !full) begin
                mem[wr_ptr] <= wr_data;
                wr_ptr <= wr_ptr + 1;
            end
            
            // Read
            if (rd_en && !empty) begin
                rd_ptr <= rd_ptr + 1;
            end
            
            // Count Update
            if (wr_en && !full && !(rd_en && !empty)) begin
                count <= count + 1;
            end else if (!(wr_en && !full) && (rd_en && !empty)) begin
                count <= count - 1;
            end
            // If both happen, count stays same
        end
    end

endmodule
