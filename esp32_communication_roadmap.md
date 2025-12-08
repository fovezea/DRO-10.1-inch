# ESP32 Communication Roadmap

This document outlines the plan for integrating an ESP32 with Touch Screen to control the FPGA Electronic Gearbox.

## 1. Physical Layer
*   **Connection**: UART (Serial).
*   **Voltage**: 3.3V Logic (Direct connection between ESP32 and EG4S20).
*   **Pins**:
    *   ESP32 TX -> FPGA RX (Pin 16).
    *   ESP32 RX <- FPGA TX (Pin 17 - *To be implemented*).
*   **Baud Rate**: Currently 9600. **Recommended Upgrade**: 115200 or 921600 for lower latency UI response.

## 2. Protocol Evolution

### Phase 1: Fire-and-Forget (Current)
*   **Direction**: Unidirectional (ESP32 -> FPGA).
*   **Reliability**: Low. No confirmation that the FPGA received the command.
*   **Format**: `[Sync] [Cmd] [Payload...]`

### Phase 2: Robustness (Recommended Next Step)
To ensure the gearbox doesn't set a wrong ratio due to noise:
1.  **Checksum**: Add a CRC8 or XOR checksum byte to the end of the packet.
    *   FPGA calculates checksum; if mismatch, discard packet.
2.  **Bidirectional Comms**:
    *   Implement `uart_tx` on FPGA.
    *   **ACK/NAK**: FPGA sends `0x06` (ACK) or `0x15` (NAK) after processing a packet.
    *   ESP32 retries command if no ACK received.

### Phase 3: State Synchronization
The ESP32 UI needs to know what the FPGA is doing.
1.  **Heartbeat**: FPGA sends a status packet every 100ms.
    *   Contains: `Current_Position`, `Current_RPM`, `Phase_Error`, `Status_Flags`.
2.  **Readback**: ESP32 can query current settings (`Get_Ratio`, `Get_Enable`).

## 3. ESP32 Software Architecture

### Tech Stack
*   **Framework**: ESP-IDF or Arduino (PlatformIO).
*   **UI Library**: LVGL (Light and Versatile Graphics Library) is the industry standard for ESP32 touch interfaces.
*   **Design Tool**: SquareLine Studio (Drag-and-drop UI builder for LVGL).

### Task Structure (FreeRTOS)
To ensure smooth motor control and smooth UI, separate concerns:

1.  **GUI Task (Low Priority)**
    *   Handles Touch Input.
    *   Updates Screen widgets.
    *   On button press -> Pushes command to **Command Queue**.

2.  **Comm Task (High Priority)**
    *   Monitors **Command Queue**.
    *   Sends UART packets to FPGA.
    *   Waits for ACK (with timeout).
    *   Parses incoming Status/Heartbeat from FPGA.
    *   Updates **State Store** (atomic variables).

3.  **State Store**
    *   Shared data structure holding `TargetRatio`, `CurrentRPM`, `IsConnected`.
    *   **Work Mode**: `Screw`, `Follow`, `Conical`.
    *   GUI Task reads from here to update labels.

## 4. Ratio Calculation Logic (Example)
To cut a specific thread pitch, the ESP32 must calculate `N` and `D`.

**Formula**:
`Ratio (N/D) = (Target_Pitch * Steps_Per_Rev) / (Leadscrew_Pitch * Encoder_Counts_Per_Rev)`

**Example**:
*   **Target Pitch**: 1.0 mm
*   **Leadscrew Pitch**: 2.0 mm
*   **Stepper**: 200 steps/rev * 8 microsteps = 1600 steps/rev
*   **Encoder**: 600 PPR * 4 (Quadrature) = 2400 counts/rev

`Ratio = (1.0 * 1600) / (2.0 * 2400) = 1600 / 4800 = 1 / 3`

*   **Result**: Send `N=1`, `D=3` to FPGA.

## 5. Proposed Protocol Specification (Bidirectional)

**Packet Structure**:
`[Header] [Length] [CmdID] [Payload...] [CRC] [Footer]`

*   **Header**: `0xAA 0x55`
*   **Length**: Number of bytes in payload.
*   **CmdID**:
    *   `0x10`: Set Ratio
    *   `0x11`: Set Enable
    *   `0x12`: Set Work Mode
    *   `0x20`: Request Status
*   **Payload**: Data (e.g., AxisID, Numerator, Denominator).
    *   For `Set Work Mode`: `[ModeID]` (0=Screw, 1=Follow, 2=Conical).
*   **CRC**: CRC-8 checksum.
*   **Footer**: `0x0A` (Newline) for easy debugging.

## 5. Implementation Steps
1.  **FPGA**: Implement `uart_tx` module.
2.  **FPGA**: Update `uart_config` to send ACKs.
3.  **ESP32**: Set up Basic UART sending (verify with current FPGA).
4.  **ESP32**: Implement LVGL UI (Slider for Ratio, Toggle for Enable).
5.  **Integration**: Connect and test "Fire-and-Forget".
6.  **Upgrade**: Switch to Bidirectional Protocol with CRC.
