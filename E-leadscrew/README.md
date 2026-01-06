# E-leadscrew (Spartan-7)

## Project Overview
**E-leadscrew** is a pure FPGA implementation of a dual-axis electronic gearbox (leadscrew) controller. It is designed to read a quadrature encoder input (e.g., from a lathe spindle) and generate synchronized Step/Direction signals for two independent stepper motors.

This version is optimized for the **Xilinx Spartan-7** FPGA on the **Spartan Edge Accelerator Board**.

### Key Features
*   **Synchronized Motion:** Digital PLL architecture ensures zero long-term drift between input and output.
*   **Dual Axis Support:** Independent gear ratios for Axis 1 (e.g., Carriage) and Axis 2 (e.g., Cross-slide).
*   **High Performance:** Running at 100MHz system clock with 64-bit precision math.
*   **Live Configuration:** Change gear ratios and enable/disable axes in real-time via UART.
*   **Robust I/O:** Glitch filtering on encoder inputs and buffered UART communication.

## Hardware Requirements
*   **FPGA Board:** [Seeed Studio Spartan Edge Accelerator Board](https://wiki.seeedstudio.com/Spartan-Edge-Accelerator-Board/) (Xilinx Spartan-7 XC7S15).
*   **Encoder:** 5V Quadrature Encoder (A/B phases).
*   **Motor Drivers:** Standard Step/Dir stepper motor drivers (e.g., TB6600, DM542).
*   **Power Supply:** 5V via USB-C or 8-17V via VIN.

## Directory Structure
```
E-leadscrew/
├── docs/               # Project Documentation
│   ├── HARDWARE_SPARTAN_EDGE.md  # Pinouts & Board Ref
│   ├── PROTOCOL.md               # UART Command Ref
│   └── ...
├── src/                # Verilog Source Code
│   ├── top.v           # Top-level Module
│   ├── constraints.xdc # Pin Constraints
│   └── ...
├── test/               # Simulation Testbenches
├── build.tcl           # Vivado Build Script
└── README.md           # This file
```

## Getting Started

### 1. Hardware Setup
Refer to [docs/HARDWARE_SPARTAN_EDGE.md](./docs/HARDWARE_SPARTAN_EDGE.md) for detailed pin mappings.
*   Connect Encoder to Arduino D2/D3.
*   Connect Motor Drivers to D4/D5 (Axis 1) and D6/D7 (Axis 2).

### 2. Simulation
The project includes a Verilog testbench (`test/tb_gearbox.v`) that simulates the full gearbox logic.
*   **Requirement:** Icarus Verilog (`iverilog`) and GTKWave.
*   **Run Simulation:**
    ```bash
    # From within the E-leadscrew directory
    iverilog -o gearbox_test test/tb_gearbox.v src/*.v
    vvp gearbox_test
    gtkwave gearbox_test.vcd
    ```

### 3. Build (Vivado)
*   **Requirement:** Xilinx Vivado 2018.3 or later.
*   **Steps:**
    1.  Launch Vivado.
    2.  In the Tcl Console, navigate to this directory: `cd [path]/E-leadscrew`
    3.  Run the build script: `source build.tcl`
    4.  The generated bitstream will be located in: `vivado_out/E-leadscrew-Spartan.runs/impl_1/top.bit`

### 4. Deploy
Load the bitstream (`top.bit`) onto the FPGA using either:
*   **JTAG:** Xilinx Hardware Manager (requires JTAG programmer).
*   **ESP32:** Copy `top.bit` to the SD card (overlay folder) and use the Spartan Edge Accelerator's ESP32 bootloader.

## Build Configuration
The project is configured via `src/top.v` parameters:
*   `CLK_FREQ`: 100,000,000 (100 MHz)
*   `BAUD_RATE`: 115200

Constraints are defined in `src/constraints.xdc` and target the XC7S15 package.

## License
[MIT License](../LICENSE)

## Contact/Support
For issues or feature requests, please open an issue in the repository.
