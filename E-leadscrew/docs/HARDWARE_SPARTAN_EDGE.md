# Spartan Edge Accelerator Board - Hardware Reference

**Board:** Xilinx Spartan-7 XC7S15 (Spartan Edge Accelerator Board)  
**System Clock:** 100 MHz (Pin H4)

## Pinout Mapping

### 1. Standard Arduino Header
These pins are compatible with standard Arduino shields.

| Signal | FPGA Pin | Arduino Pin | Description |
| :--- | :--- | :--- | :--- |
| **CLK** | **H4** | - | 100MHz System Clock |
| **RST** | **C3** | - | Reset / USER1 Button |
| **UART_TX** | **A12** | D0 | UART Transmit (FPGA Output) |
| **UART_RX** | **C12** | D1 | UART Receive (FPGA Input) |
| **ENC_A** | **A10** | D2 | Encoder Phase A |
| **ENC_B** | **B6** | D3 | Encoder Phase B |
| **STEP_1** | **A5** | D4 | Axis 1 Step Output |
| **DIR_1** | **B5** | D5 | Axis 1 Direction Output |
| **STEP_2** | **A4** | D6 | Axis 2 Step Output |
| **DIR_2** | **A3** | D7 | Axis 2 Direction Output |

### 2. FPGA GPIO Header (10-Pin)
dedicated FPGA I/O pins located on the 10-pin header (labeled IO0-IO9).

| Header Pin | FPGA Pin |
| :--- | :--- |
| IO0 | N14 |
| IO1 | M14 |
| IO2 | C4 |
| IO3 | B13 |
| IO4 | N10 |
| IO5 | M10 |
| IO6 | B14 |
| IO7 | D3 |
| IO8 | P5 |
| IO9 | E11 |

### 3. On-Board User Interface

| Component | FPGA Pin | Note |
| :--- | :--- | :--- |
| **Switch K1** | M2 | |
| **Switch K2** | L2 | |
| **Switch K3** | L3 | |
| **Switch K4** | K3 | |
| **Button USER1**| C3 | Often used as Reset |
| **Button USER2**| M4 | |
| **LED 1 (Yel)** | J1 | |
| **LED 2 (Red)** | A13 | |

### 4. Grove Connectors

| Signal | FPGA Pin |
| :--- | :--- |
| **I2C SDA** | P13 |
| **I2C SCL** | P12 |
