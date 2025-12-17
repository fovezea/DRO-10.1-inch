# ESP-Hosted Firmware Update Guide (ESP32-C6)

This guide details the process for updating and flashing the ESP-Hosted firmware onto the ESP32-C6 co-processor. This is required if you want to update the WiFi/Bluetooth capabilities provided to the main ESP32-P4 host.

## 📋 Prerequisites

*   **ESP-IDF v5.x** installed and active in your terminal.
*   **USB Connection**: Connect the ESP32-C6 **directly** to your PC via its USB port.
    *   *Note: Do not confuse this with the ESP32-P4's USB port.*
*   Identify the **COM Port** of the ESP32-C6 (e.g., `COM3`, `/dev/ttyUSB1`).

## 🚀 Step-by-Step Procedure

### 1. Navigate to the Slave Component
The firmware source code is located within the project's components directory.

```bash
cd components/espressif__esp_hosted/slave
```

### 2. Set the Target
Tell ESP-IDF that we are building for the ESP32-C6.

```bash
idf.py set-target esp32c6
```

### 3. Configuration (Optional)
Verify or modify the configuration (e.g., verify SDIO vs SPI transport settings). The default in this project is **SDIO**.

```bash
idf.py menuconfig
```
*   Check `Example Configuration > Transport layer`.

### 4. Build the Firmware
Compile the project.

```bash
idf.py build
```

### 5. Flash and Monitor
Flash the compiled binary to the ESP32-C6. Replace `COMx` with your actual port.

```bash
idf.py -p COMx flash monitor
```

---

## ❓ Frequently Asked Questions

**Q: Do I need to do this every time I update the main P4 application?**
A: **No.** You only need to update the C6 firmware if the `espressif__esp_hosted` component version changes or if you need to change the transport configuration (e.g., switching from SDIO to SPI).

**Q: My P4 cannot connect to the C6?**
A: Ensure:
1.  The C6 is flashed with the correct transport mode (SDIO).
2.  The strapping pins on the C6 are set correctly (if applicable).
3.  The P4 application matches the transport configuration.
