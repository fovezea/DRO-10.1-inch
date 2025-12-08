module electronic_gearbox (
    input wire clk,
    input wire reset,
    
    // Encoder Interface
    input wire quad_A,
    input wire quad_B,
    
    // Configuration (Dual Axis)
    input wire [15:0] num_1,
    input wire [15:0] den_1,
    input wire [15:0] num_2,
    input wire [15:0] den_2,
    
    // Enables
    input wire enable_1,
    input wire enable_2,
    
    // Output (Dual Axis)
    output wire step_1,
    output wire dir_1,
    output wire step_2,
    output wire dir_2,
    
    // Debug / Status
    output wire signed [31:0] phase_err_1,
    output wire signed [31:0] phase_err_2
);

    // 1. Shared Input Processing
    wire in_step;
    wire in_dir;
    quadrature_decoder qd (
        .clk(clk),
        .quad_A(quad_A),
        .quad_B(quad_B),
        .step_pulse(in_step),
        .direction(in_dir)
    );

    wire [31:0] measured_period;
    wire period_valid;
    period_measure pm (
        .clk(clk),
        .reset(reset),
        .input_pulse(in_step),
        .period_counts(measured_period),
        .valid(period_valid)
    );

    // 2. Axis 1 Controller
    gearbox_channel axis1 (
        .clk(clk),
        .reset(reset),
        .enable(enable_1),
        .in_step(in_step),
        .in_dir(in_dir),
        .measured_period(measured_period),
        .period_valid(period_valid),
        .numerator(num_1),
        .denominator(den_1),
        .step_pulse(step_1),
        .direction(dir_1),
        .phase_error_out(phase_err_1)
    );

    // 3. Axis 2 Controller
    gearbox_channel axis2 (
        .clk(clk),
        .reset(reset),
        .enable(enable_2),
        .in_step(in_step),
        .in_dir(in_dir),
        .measured_period(measured_period),
        .period_valid(period_valid),
        .numerator(num_2),
        .denominator(den_2),
        .step_pulse(step_2),
        .direction(dir_2),
        .phase_error_out(phase_err_2)
    );

endmodule
