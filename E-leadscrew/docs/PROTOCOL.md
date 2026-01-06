# Communication Protocol

**Interface:** UART  
**Baud Rate:** 115200  
**Data Format:** 8N1 (8 Data bits, No Parity, 1 Stop bit)

## Command Structure

The system accepts binary commands to configure the electronic gearbox.

### Axis Identifiers
*   `0x00`: Axis 1 (Primary)
*   `0x01`: Axis 2 (Secondary)

### Commands

#### 1. Set Gear Ratio (0x55)
Configures the electronic gearing ratio for a specific axis. The output frequency is calculated as:  
`F_out = F_in * (Numerator / Denominator)`

*   **Opcode:** `0x55`
*   **Length:** 6 Bytes
*   **Format:**
    ```
    [0x55] [AxisID] [NumH] [NumL] [DenH] [DenL]
    ```
    *   `AxisID`: 0 or 1
    *   `Num`: 16-bit Numerator (Big Endian)
    *   `Den`: 16-bit Denominator (Big Endian)

*   **Example:** Set Axis 1 to 2:1 Ratio (Output = 2x Input)
    *   Numerator = 2 (`0x0002`), Denominator = 1 (`0x0001`)
    *   Send: `0x55 0x00 0x00 0x02 0x00 0x01`

#### 2. Enable/Disable Axis (0x56)
Enables or disables the step output for a specific axis.

*   **Opcode:** `0x56`
*   **Length:** 3 Bytes
*   **Format:**
    ```
    [0x56] [AxisID] [Enable]
    ```
    *   `AxisID`: 0 or 1
    *   `Enable`: `1` = Enable, `0` = Disable

*   **Example:** Disable Axis 2
    *   Send: `0x56 0x01 0x00`
