# ESP32 DRO Backend

This is the "Dumb Backend" firmware for the DRO project. It reads quadrature encoders and streams the positions to the Frontend via UART.

## 🔌 Supported Boards

We provide pre-configured "BSP" files for popular boards:

### 1. ESP32-DevKitC (Standard ESP32)

* **Chip**: ESP32-WROOM-32
* **Capacity**: Supports ALL 5 Axes (Axis 0 - Axis 4).
* **Build Command**:

    ```bash
    idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32_devkitc" build
    ```

### 2. ESP32-S3-DevKitC (ESP32-S3)

* **Chip**: ESP32-S3-WROOM-1
* **Capacity**: Supports 4 Axes (Axis 0 - Axis 3). Axis 4 is disabled by default due to hardware limits (4 PCNT units).
* **Build Command**:

    ```bash
    idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32s3_devkitc" build
    ```

## 📍 Pinout

The pin mappings are defined in `idf.py menuconfig` -> **DRO Configuration**.
The defaults provided in the SDKCONFIG files above map to standard GPIOs available on the respective headers.

## 🧪 Simulation / Demo Mode

You can enable a **Debug Simulation** mode to test the Frontend UI without connecting physical encoders. In this mode, the backend generates fake moving patterns (bouncing counters) for all enabled axes.

**To Enable:**

1. Run `idf.py menuconfig`
2. Go to **DRO Configuration** -> **Debug Configuration**
3. Enable **Simulate Encoder Data (Demo Mode)**
4. Rebuild and flash.

## 🚀 Flashing

Connect your board via USB and run:

```bash
idf.py -p COMx flash monitor
```
