# Electronic Leadscrew / Gearbox (FPGA)

This project implements a **Dual-Axis Electronic Gearbox** on an **EG4S20 FPGA** (MLKPAI-FMin01). It reads a quadrature encoder input and generates Step/Direction signals for two independent motors at programmable ratios.

## Key Features
*   **Dual Axis**: Supports two independent output axes driven by a single encoder input.
*   **Digital PLL Architecture**: Uses Period Measurement + Frequency Synthesis to generate smooth output pulses, even when multiplying the frequency (e.g., 2:1 ratio).
*   **Position Locking**: Continuously tracks the phase error to ensure zero long-term drift.
*   **UART Configuration**: Control gear ratios and enable/disable axes on-the-fly via UART.

## UART Control Protocol
*   **Baud Rate**: 9600
*   **Axis IDs**: `0` = Axis 1, `1` = Axis 2

### 1. Set Gear Ratio (Command 0x55)
Sets the Numerator (N) and Denominator (D) for a specific axis.
*   **Format**: 6 Bytes
    ```
    [0x55] [AxisID] [NumH] [NumL] [DenH] [DenL]
    ```
*   **Example**: Set Axis 1 to 2:1 Ratio (Output = 2x Input)
    *   Send: `0x55 0x00 0x00 0x02 0x00 0x01`

### 2. Enable/Disable Axis (Command 0x56)
Enables or disables the output for a specific axis.
*   **Format**: 3 Bytes
    ```
    [0x56] [AxisID] [Enable]
    ```
*   **Enable**: `1` = On, `0` = Off.
*   **Example**: Disable Axis 2
    *   Send: `0x56 0x01 0x00`

## Pinout (MLKPAI-FMin01)
Defined in `src/constraints.adc`:

| Signal | Pin | Description |
| :--- | :--- | :--- |
| `clk` | **34** | 25MHz System Clock |
| `rst_n` | **31** | Reset (Button SW2) |
| `uart_rx` | **16** | UART RX (Connect to ESP32 TX) |
| `enc_A` | **4** | Encoder A Input |
| `enc_B` | **5** | Encoder B Input |
| `step_out_1` | **2** | Axis 1 Step (LED6) |
| `dir_out_1` | **3** | Axis 1 Dir (LED5) |
| `step_out_2` | **14** | Axis 2 Step (LED4) |
| `dir_out_2` | **51** | Axis 2 Dir (LED7) |

## Simulation
A testbench is provided in `test/tb_gearbox.v`. It verifies:
1.  Dual axis operation with different ratios.
2.  Enable/Disable functionality.

**Run with Icarus Verilog:**
```bash
iverilog -o gearbox_test test/tb_gearbox.v src/electronic_gearbox.v src/gearbox_channel.v src/quadrature_decoder.v src/period_measure.v src/pulse_gen.v src/uart_rx.v src/uart_config.v src/top.v
vvp gearbox_test
```
