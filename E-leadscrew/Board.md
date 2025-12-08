# MLKPAI-FMin01 Board Documentation

**FPGA**: Anlogic EG4S20NG88
**Crystal Oscillator**: 25MHz

## Pin Definitions

### System
| Signal | Pin | Description |
| :--- | :--- | :--- |
| **CLK** | **34** | 25MHz System Clock Input |
| **RST_N** | **31** | Reset (Mapped to Button SW2) |

### UART (USB-C Virtual Serial)
| Signal | Pin | Description |
| :--- | :--- | :--- |
| **UART_RX** | **16** | FPGA RX (Connect to Host TX) |
| **UART_TX** | **17** | FPGA TX (Connect to Host RX) |

### User I/O (LEDs)
| Signal | Pin | Description |
| :--- | :--- | :--- |
| **LED4** | **14** | Red LED |
| **LED5** | **3** | Red LED (Used for `dir_out`) |
| **LED6** | **2** | Red LED (Used for `step_out`) |
| **LED7** | **51** | Red LED |
| **LED8** | **52** | Red LED |
| **LED9** | **57** | Red LED |
| **LED10** | **59** | Red LED |
| **LED11** | **62** | Red LED |

### User I/O (Buttons)
| Signal | Pin | Description |
| :--- | :--- | :--- |
| **SW2** | **31** | User Button 2 (Active Low) |
| **SW3** | **30** | User Button 3 (Active Low) |

### User I/O (DVP / Headers)
*Partial list based on usage:*
| Signal | Pin | Description |
| :--- | :--- | :--- |
| **IO_4** | **4** | General IO (Used for `enc_A`) |
| **IO_5** | **5** | General IO (Used for `enc_B`) |

## Notes
*   **Programming**: The board uses an integrated USB-JTAG solution on the USB-C port.
*   **Power**: Powered via USB-C (5V).
*   **Flash**: W25Q16DVSN (16M-bit SPI Flash).
