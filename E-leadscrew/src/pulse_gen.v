module pulse_gen (
    input wire clk,
    input wire reset,
    input wire [31:0] period_cycles,
    input wire enable,
    output reg step_pulse
);

    reg [31:0] counter;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            counter <= 0;
            step_pulse <= 0;
        end else begin
            if (enable && period_cycles > 0) begin
                if (counter >= period_cycles - 1) begin
                    counter <= 0;
                    step_pulse <= 1;
                end else begin
                    counter <= counter + 1;
                    step_pulse <= 0;
                end
            end else begin
                step_pulse <= 0;
                // Keep counter reset if disabled or invalid period
                counter <= 0;
            end
        end
    end

endmodule
