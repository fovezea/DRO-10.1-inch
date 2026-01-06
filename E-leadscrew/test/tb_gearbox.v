`timescale 1ns / 1ps

module tb_gearbox;

    reg clk;
    reg reset;
    reg quad_A, quad_B;
    reg uart_rx_line;
    wire uart_tx_line;
    wire step_1, dir_1;
    wire step_2, dir_2;

    // Instantiate the Unit Under Test (UUT)
    top #(
        .CLK_FREQ(100000000),
        .BAUD_RATE(115200),
        .HEARTBEAT_MS(1) // Fast heartbeat for simulation
    ) uut (
        .clk(clk),
        .rst_n(~reset),
        .enc_A(quad_A),
        .enc_B(quad_B),
        .uart_rx(uart_rx_line),
        .uart_tx(uart_tx_line),
        .step_out_1(step_1),
        .dir_out_1(dir_1),
        .step_out_2(step_2),
        .dir_out_2(dir_2),
        .led_test()
    );

    // Clock generation (100MHz = 10ns period)
    always #5 clk = ~clk;

    // Encoder emulation task
    task generate_quadrature;
        input integer period_ns;
        input integer cycles;
        integer i;
        begin
            for (i = 0; i < cycles; i = i + 1) begin
                # (period_ns / 4) quad_A = 1; quad_B = 0;
                # (period_ns / 4) quad_A = 1; quad_B = 1;
                # (period_ns / 4) quad_A = 0; quad_B = 1;
                # (period_ns / 4) quad_A = 0; quad_B = 0;
            end
        end
    endtask
    
    // UART Byte Send Task (115200 baud = ~8680 ns per bit)
    localparam BIT_PERIOD = 8680;
    
    task send_uart_byte;
        input [7:0] data;
        integer i;
        begin
            // Start Bit
            uart_rx_line = 0;
            #BIT_PERIOD;
            // Data Bits
            for (i = 0; i < 8; i = i + 1) begin
                uart_rx_line = data[i];
                #BIT_PERIOD;
            end
            // Stop Bit
            uart_rx_line = 1;
            #BIT_PERIOD;
        end
    endtask
    
    task send_config_packet;
        input [7:0] axis;
        input [15:0] num;
        input [15:0] den;
        begin
            send_uart_byte(8'h55); // Sync
            send_uart_byte(axis);  // Axis ID
            send_uart_byte(num[15:8]);
            send_uart_byte(num[7:0]);
            send_uart_byte(den[15:8]);
            send_uart_byte(den[7:0]);
        end
    endtask

    task send_enable_packet;
        input [7:0] axis;
        input enable;
        begin
            send_uart_byte(8'h56); // Cmd 0x56
            send_uart_byte(axis);  // Axis ID
            send_uart_byte(enable); // 1=Enable, 0=Disable
        end
    endtask

    initial begin
        // Initialize Inputs
        clk = 0;
        reset = 1;
        quad_A = 0;
        quad_B = 0;
        uart_rx_line = 1; // Idle High

        // Wait 100 ns for global reset to finish
        #100;
        reset = 0;
        
        // Default is 1:1 for both
        $display("Starting Test: Default 1:1");
        generate_quadrature(100000, 10);
        
        // Configure Axis 1 -> 2:1
        $display("Config Axis 1 -> 2:1");
        send_config_packet(0, 2, 1);
        #100000;
        
        // Configure Axis 2 -> 1:2
        $display("Config Axis 2 -> 1:2");
        send_config_packet(1, 1, 2);
        #100000;
        
        $display("Testing Dual Axis");
        // Axis 1 should be fast (2x), Axis 2 slow (0.5x)
        generate_quadrature(100000, 20);

        // Disable Axis 1
        $display("Disabling Axis 1");
        send_enable_packet(0, 0);
        #100000;
        
        $display("Testing Disable (Axis 1 should be silent)");
        generate_quadrature(100000, 10);
        
        // Re-enable Axis 1
        $display("Re-enabling Axis 1");
        send_enable_packet(0, 1);
        #100000;
        
        $display("Testing Re-enable");
        generate_quadrature(100000, 10);

        $display("Simulation Finished");
        $finish;
    end
    
    // Monitor
    initial begin
        $dumpfile("gearbox_test.vcd");
        $dumpvars(0, tb_gearbox);
    end
    
    // UART RX Monitor (Simple)
    reg [7:0] rx_byte;
    reg [3:0] rx_bit;
    
    always @(negedge uart_tx_line) begin
        // Start bit detected
        # (BIT_PERIOD * 1.5); // Skip start bit and half data bit
        for (rx_bit=0; rx_bit<8; rx_bit=rx_bit+1) begin
            rx_byte[rx_bit] = uart_tx_line;
            # BIT_PERIOD;
        end
        $display("UART TX Byte: 0x%h at %t", rx_byte, $time);
    end

endmodule
