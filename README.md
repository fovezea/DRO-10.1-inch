# ESP32-P4 DRO (Digital Readout) for Milling Machine & Lathe

This project provides a complete Digital Readout (DRO) system for milling machines and lathes using the JC8012P4A1C 10.1-inch ESP32-P4 board. It features a modern, high-performance HMI built with LVGL 9.2, designed with a **"Smart Client, Dumb Backend"** philosophy.

## 🏗️ System Architecture

- **Frontend (ESP32-P4)**: Handles all business logic, coordinate systems, geometric calculations, and UI rendering.
- **Backend (ESP32)**: Acts as a passive data pipe, streaming raw encoder values.
- **FPGA (Xilinx Spartan-7)**: Handles high-speed real-time motor control for the Electronic Leadscrew (ELS) functions.
- **Communication**: UART (115200 baud) for FPGA control; ESP-NOW/WiFi for wireless encoder connectivity.

- **FPGA board**: Spartan Edge Accelerator Board

[Seeed Wiki](https://wiki.seeedstudio.com/Spartan-Edge-Accelerator-Board/)  

![board](res/9f837cbd7ca3c782115124a9c35eca58.png)  

## 🚀 Key Features

- ✅ **HMI-Centric Design**: All math (ABS/INC, PCD, Tapers) happens on the display unit.
- ✅ **Multi-Axis Support**: X, Y, Z, W, C axis support with customizable mapping.
- ✅ **Electronic Leadscrew (ELS)**: Integration with Spartan-7 FPGA for threading and turning operations.
- ✅ **Modern UI**: Professional dark-mode interface designed for touch input (ISA-101 inspired).
- ✅ **Self-Contained**: All drivers (Display, Touch, Audio) included locally.

## 🔧 Hardware Configuration

- **Main Board**: ESP32-P4 (JC8012P4A1C 10.1-inch board)
- **Display**: 1280x800 MIPI-DSI LCD (JD9365 controller)
- **Touch**: GSL3680 capacitive touch controller
- **FPGA**: Xilinx Spartan-7 (Spartan Edge Accelerator Board)
- **Orientation**: Landscape mode (270-degree rotation)

## 📦 Project Structure

```
DRO-10.1-inch/
├── components/                        # Local components
│   ├── dro_system/                   # Core DRO business logic
│   ├── eez/                          # LVGL UI implementation (EEZ Studio generated assets)
│   ├── fpga_comms/                   # FPGA communication driver
│   ├── machine_params/               # NVS storage for machine settings
│   ├── screw_calc/                   # Threading ratio calculator
│   └── cone_calc/                    # Taper turning calculator
├── common/                           # Shared protocol definitions
│   └── protocol_defs.h               # FPGA communication protocol
├── main/                             # Application entry point
└── README.md                         # This file
```

## 🛠️ Electronic Lead Screw (ELS)

The ELS subsystem uses the **Spartan Edge Accelerator Board**, which contains both a Xilinx Spartan-7 FPGA and an onboard ESP32.

> [!NOTE]
> **Modular Design**: The ELS functionality is optional. The system supports a setting to enable/disable ELS features, allowing the hardware to be used as a standalone DRO without the FPGA board.

**Architectural Note**: The data flow implementation for the Spartan board is currently flexible:

1. **Direct Bypass**: Frontend talks directly to the FPGA (via headers).
2. **Bridged**: Frontend talks to the onboard ESP32, which forwards data to the FPGA.

This ambiguity allows for future optimization (e.g., using the onboard ESP32 for additional wireless connectivity or co-processing).

- **Protocol**: Custom UART protocol (115200 baud). See `common/protocol_defs.h`.
- **Modes**:

  - **Screw Mode**: Automated gear ratio calculation for threading.
  - **Follow Mode**: Synchronized axis movement.
  - **Conical Mode**: Coordinated X/Z movement for tapers.

## 💻 Development

### Building and Flashing

```bash
# Build the project
idf.py build

# Flash to ESP32-P4
idf.py -p /dev/ttyUSB0 flash monitor
```

### Protocol Updates

The communication protocol is defined in `common/protocol_defs.h`. Any changes to command opcodes or structures should be made there and propagated to the FPGA firmware.

## 📄 License

This project follows the same license terms as the original ESP-IDF examples.
