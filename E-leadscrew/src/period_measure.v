module period_measure (
    input wire clk,
    input wire reset,
    input wire input_pulse,
    output reg [31:0] period_counts,
    output reg valid
);

    reg [31:0] counter;
    reg [31:0] last_period;
    reg first_pulse;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            counter <= 0;
            period_counts <= 0;
            valid <= 0;
            first_pulse <= 1;
        end else begin
            if (input_pulse) begin
                if (!first_pulse) begin
                    period_counts <= counter;
                    valid <= 1;
                end
                first_pulse <= 0;
                counter <= 0;
            end else begin
                // Prevent overflow if stopped
                if (counter != 32'hFFFFFFFF) begin
                    counter <= counter + 1;
                end
            end
        end
    end

endmodule
