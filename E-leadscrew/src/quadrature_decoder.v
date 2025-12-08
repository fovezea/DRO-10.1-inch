module quadrature_decoder (
    input wire clk,
    input wire quad_A,
    input wire quad_B,
    output reg step_pulse,
    output reg direction // 1 = Forward, 0 = Reverse
);

    reg [2:0] sync_A, sync_B;

    always @(posedge clk) begin
        sync_A <= {sync_A[1:0], quad_A};
        sync_B <= {sync_B[1:0], quad_B};
    end

    wire A_rise = (sync_A[2:1] == 2'b01);
    wire A_fall = (sync_A[2:1] == 2'b10);
    wire B_rise = (sync_B[2:1] == 2'b01);
    wire B_fall = (sync_B[2:1] == 2'b10);

    always @(posedge clk) begin
        step_pulse <= 0;
        if (A_rise) begin
            step_pulse <= 1;
            direction <= (sync_B[1] == 0); // A leads B (Forward)
        end else if (A_fall) begin
            step_pulse <= 1;
            direction <= (sync_B[1] == 1);
        end else if (B_rise) begin
            step_pulse <= 1;
            direction <= (sync_A[1] == 1);
        end else if (B_fall) begin
            step_pulse <= 1;
            direction <= (sync_A[1] == 0);
        end
    end

endmodule
