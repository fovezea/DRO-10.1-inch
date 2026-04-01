# ESP32-P4 DRO (Digital Readout) & Electronic Leadscrew

This project provides a complete Digital Readout (DRO) and Electronic Leadscrew (ELS) system for milling machines and lathes using the JC8012P4A1C 10.1-inch ESP32-P4 display board. It features a modern, high-performance HMI built with LVGL 9.2, designed with a **"Smart Client, Dumb Backend"** philosophy.

## 🏗️ System Architecture

- **Frontend (HMI)**: The 10.1-inch P4 Display calculates all math (ABS/INC, PCD, Tapers, Threading Ratios) locally.
- **Backend (Hardware Controller)**: A separate ESP32 handles rapid pulse-counting (PCNT) from glass scales/encoders and outputs motor control signals (RMT/MCPWM) to stepper drivers.
- **Communication**: The system uses a reliable **USB HID** protocol instead of legacy UART to transmit configuration, axis telemetry, and synchronization data between the display and the motion controller.

---

## 🖥️ The Frontend (HMI)

- **Main Board**: ESP32-P4 (JC8012P4A1C 10.1-inch board)
- **Display**: 1280x800 MIPI-DSI LCD (JD9365 controller) in Landscape mode.
- **Touch**: GSL3680 capacitive touch controller.
- **UI Framework**: Strictly C-based LVGL (EEZ Studio generation is now deprecated, modifying `screens.c` directly).

![frontend](res/1.jpg)

## 🎛️ The Backend (Hardware Controllers)

This project has completely migrated away from FPGA-based controllers. The Electronic Leadscrew and encoder tracking are now entirely driven by powerful ESP32-series dual-core and quad-core microcontrollers.

Because we use a unified **ESP-IDF Kconfig Architecture**, the same `esp32p4_usb_dro_backend` codebase targets multiple backend boards seamlessly by isolating pinouts into a multiplexed Board Support Package (`bsp.h`).

### Supported Backend Hardware

#### 1. ESP32-P4C6-osprey Baseboard

The primary high-performance development board.
![Osprey P4 Baseboard](res/OspreyPI-P4C6.avif)

#### 2. ESP32-P4 WT9932P4-TINY

A compact ESP32-P4 variant with carefully re-routed USB tracking and bootstrap pins to preserve USB-HID capability without stepper collision.
![TINY P4 Board](res/TINY.webp)

#### 3. ESP32-S3 Generic Board

A cost-effective fallback for legacy ESP32-S3 dual-core architectures.

---

## 🚀 Key Features

- ✅ **HMI-Centric Design**: All math (ABS/INC, PCD, Tapers) happens on the display unit.
- ✅ **Multi-Axis Support**: 5 virtual axes (X, Y, Z, W, C) + dedicated Spindle tracking with customizable mapping.
- ✅ **Button-Tap Centric UI**: The UI has been rigidly locked to prevent accidental horizontal swiping/sliding on settings tabs, optimized for industrial machine screens with dirty/gloved hands.
- ✅ **Electronic Leadscrew (ELS)**: Stepper synchronization for threading, follow mode, and conical turning operations directly through the ESP32 backend.
- ✅ **USB HID Bridge**: No fragile RS485/UART wiring required. The display and backend talk natively over a customized standard USB connection.

---

## 📦 Project Structure

```text
DRO-10.1-inch/
├── components/                        # Local frontend components
│   ├── dro_system/                   # Core DRO business logic & Axis mapping
│   ├── eez/                          # LVGL UI implementation (Manual C edits)
│   ├── frontend_usb/                 # USB Host HID driver bridging
│   ├── machine_params/               # NVS storage for machine settings
│   ├── screw_calc/                   # Threading ratio calculator
│   └── cone_calc/                    # Taper turning calculator
├── esp32p4_usb_dro_backend/          # THE UNIFIED BACKEND REPOSITORY
│   ├── main/                         # USB Device HID stack & ELS algorithm
│   └── modules/BSP/                  # Multi-Board Support Package (Pinouts)
└── README.md                         # This file
```

## 💻 Development

### Compiling the Backend

The Backend uses ESP-IDF's Target and Kconfig systems to select the physical board logic at compile-time.

```bash
# 1. Set your silicon target (e.g. esp32p4 or esp32s3)
idf.py set-target esp32p4

# 2. Open the Configuration Menu to select the EXACT board
idf.py menuconfig
# -> Navigate to "E-Leadscrew Hardware Selection"
# -> Choose "ESP32-P4 WT9932P4-TINY" or "ESP32-P4C6-osprey Baseboard"

# 3. Build & Flash
idf.py build flash monitor
```

> [!WARNING]
> Ensure you plug the USB cable into your **FS (Full-Speed / JTAG)** port when flashing the backend board, but plug the display (or PC for testing) into the **HS (High-Speed / OTG)** port for the USB HID protocol to communicate!

## 🛣️ Implementation Roadmap

### ✅ What is Working Right Now

- **The Modern LVGL 9.2 Frontend**: Fast, dark-mode styling, and entirely button-driven navigation optimized to prevent accidental gestures in an industrial shop environment.
- **Smart Math Core**: Fully decoupled local math inside the display processor handling all PCD, Taper, and ABS/INC calculations.
- **USB HID Bridge**: A stable, high-speed custom USB HID datalink connecting the Frontend Display directly to the Backend microcontrollers, fully deprecating the legacy FPGA/RS485 setups.
- **Multi-Board SDK Architecture**: The backend successfully targets custom ESP32-P4 footprints (Osprey, TINY) and legacy S3 boards via a clean Kconfig matrix that automatically patches pinouts at compile-time to prevent hardware conflicts.
- **Pulse Counting (PCNT)**: Hardware quadrature decoding using the ESP32's native PCNT peripherals to track physical glass scales.

### 🚧 What is Not Yet Working (Next Steps)

- **Physical Stepper Output (RMT / MCPWM)**: Translating the algorithmic ELS math into precise step/dir signals sent directly to the motor drivers.
- **Open-Loop ELS Mode**: A "blind" threading/turning mode based strictly on the spindle encoder syncing linearly against the step outputs, without demanding linear scale validation.
- **Closed-Loop Feedback Algorithm**: Dynamic error-correction logic that tracks the exact hardware position off the linear scale against the intended stepper location to guarantee zero missed steps during precision threading modes.
- **Hardware Machine Interlocks**: Complete bridging of physical Limit Switches and E-Stop pins into the LVGL warning dialogs.

## 📄 License

This project follows the same license terms as the original ESP-IDF examples.
