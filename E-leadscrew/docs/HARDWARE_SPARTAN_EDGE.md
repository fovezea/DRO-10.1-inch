# Spartan Edge Accelerator Board - Hardware Reference

**Board:** Xilinx Spartan-7 XC7S15 (Spartan Edge Accelerator Board)  
**System Clock:** 100 MHz (Pin H4)

## Pinout Mapping

### 1. Standard Arduino Header

**Crucial**: These pins are connected **DIRECTLY to the FPGA**. They are not connected to the Onboard ESP32's GPIOs.

| Signal | FPGA Pin | Arduino Pin | Description | Connection Type |
| :--- | :--- | :--- | :--- | :--- |
| **CLK** | **H4** | - | 100MHz System Clock | FPGA System |
| **RST** | **C3** | - | Reset / USER1 Button | FPGA System |
| **UART_TX** | **A12** | D0 | UART Transmit (FPGA Output) | FPGA Direct |
| **UART_RX** | **C12** | D1 | UART Receive (FPGA Input) | FPGA Direct |
| **ENC_A** | **A10** | D2 | Encoder Phase A | FPGA Direct |
| **ENC_B** | **B6** | D3 | Encoder Phase B | FPGA Direct |
| **STEP_1** | **A5** | D4 | Axis 1 Step Output | FPGA Direct |
| **DIR_1** | **B5** | D5 | Axis 1 Direction Output | FPGA Direct |
| **STEP_2** | **A4** | D6 | Axis 2 Step Output | FPGA Direct |
| **DIR_2** | **A3** | D7 | Axis 2 Direction Output | FPGA Direct |

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

---

## 🔌 5-Axis DRO Pin Plan

**Objective**: Use the FPGA to read 5 external linear scales (encoders) + 1 Spindle Encoder, and have the Onboard ESP32 collect this data.

### 1. Spindle Encoder (Existing ELS)

| Signal | Header Pin | FPGA Pin |
| :--- | :--- | :--- |
| **Enc A** | Arduino D2 | A10 |
| **Enc B** | Arduino D3 | B6  |

### 2. Linear Scale Inputs (DRO)

Using the 10-pin FPGA GPIO Header (IO0-IO9).

| Axis | Signal A | Signal B | Connector Pin |
| :--- | :--- | :--- | :--- |
| **X-Axis** | IO0 (N14) | IO1 (M14) | Header Pin 1, 2 |
| **Y-Axis** | IO2 (C4)  | IO3 (B13) | Header Pin 3, 4 |
| **Z-Axis** | IO4 (N10) | IO5 (M10) | Header Pin 5, 6 |
| **W-Axis** | IO6 (B14) | IO7 (D3)  | Header Pin 7, 8 |
| **C-Axis** | IO8 (P5)  | IO9 (E11) | Header Pin 9, 10|

### 3. Onboard ESP32 Role

In this configuration, the Onboard ESP32 is **NOT used for runtime data collection**.

* **Encoders**: Connected directly to FPGA IO0-IO9.
* **Comms**: P4 Frontend communicates directly with FPGA UART Pins (D0/D1).
* **Usage**: The Onboard ESP32 is reserved for **FPGA Bitstream Loading** and firmware updates only.
