module gearbox_channel (
    input wire clk,
    input wire reset,
    input wire enable, // Active High Enable
    
    // Shared Inputs from Decoder/Period Measure
    input wire in_step,
    input wire in_dir,
    input wire [31:0] measured_period,
    input wire period_valid,
    
    // Configuration
    input wire [15:0] numerator,
    input wire [15:0] denominator,
    
    // Output
    output wire step_pulse,
    output wire direction,
    output wire signed [31:0] phase_error_out
);

    // 1. Phase Error Tracking (Accumulator)
    // We want: Out_Count = In_Count * (N / D)
    // So: Out_Count * D = In_Count * N
    // Error = (In_Count * N) - (Out_Count * D)
    
    reg signed [31:0] phase_error; 
    assign phase_error_out = phase_error; 
    
    always @(posedge clk or posedge reset) begin
        if (reset || !enable) begin
            phase_error <= 0;
        end else begin
            if (in_step) begin
                if (in_dir) // Forward
                    phase_error <= phase_error + numerator;
                else        // Reverse
                    phase_error <= phase_error - numerator; 
            end
            
            if (step_pulse) begin
                 if (phase_error > 0)
                    phase_error <= phase_error - denominator;
                 else if (phase_error < 0)
                    phase_error <= phase_error + denominator;
            end
        end
    end

    // 2. Ratio Calculation & Loop Filter
    reg [31:0] target_period;
    
    always @(posedge clk) begin
        if (period_valid && numerator > 0) begin
            // Calculate Base Period: T_out = T_in * D / N
            target_period <= (measured_period * denominator) / numerator;
            
            // Apply correction (P-Control)
            if (phase_error > 100) 
                target_period <= target_period - 100; // Speed up
            else if (phase_error < -100)
                target_period <= target_period + 100; // Slow down
        end
    end

    // 3. Pulse Generator
    assign direction = (phase_error >= 0); 
    
    pulse_gen pg (
        .clk(clk),
        .reset(reset),
        .period_cycles(target_period),
        .enable(period_valid && (phase_error != 0)), 
        .step_pulse(step_pulse)
    );

endmodule
