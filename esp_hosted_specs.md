# ESP-Hosted-MCU: SDIO Configuration Guide (ESP32-P4 + ESP32-C6)

This document provides the proven configuration for a persistent, stable Wi-Fi/BT offloading system using the ESP32-P4 as a Host and ESP32-C6 as a Slave.

## 1. Hardware Overview
- **Host:** ESP32-P4 (Function EV Board)
- **Slave:** ESP32-C6 (On-board co-processor)
- **Interface:** SDIO Slot 1 (High-speed)

## 2. Hardware Pin Mapping (ESP32-P4 Slot 1)
SDIO Slot 1 uses dedicated IOMUX pins. No manual GPIO configuration is required for the data bus, but the Reset pin must be correctly assigned.

| Signal Name | ESP32-P4 GPIO | Description |
| :--- | :---: | :--- |
| **CLK** | 18 | SDIO Clock (Dedicated IOMUX) |
| **CMD** | 19 | SDIO Command (Dedicated IOMUX) |
| **D0** | 14 | SDIO Data 0 (Dedicated IOMUX) |
| **D1** | 15 | SDIO Data 1 (Dedicated IOMUX) |
| **D2** | 16 | SDIO Data 2 (Dedicated IOMUX) |
| **D3** | 17 | SDIO Data 3 (Dedicated IOMUX) |
| **Reset Slave**| 54 | Connected to ESP32-C6 `EN` Pin |

## 3. Persistent Component Setup
To make modifications to the driver persistent, move the `esp_hosted` module from `managed_components` to the local `components` directory.

### Project Structure
```text
project_root/
├── components/
│   └── esp_hosted/      <-- Copy from managed_components
├── main/
│   ├── idf_component.yml
│   └── main.c
└── sdkconfig
```

### Dependency Configuration (`main/idf_component.yml`)
Point to the local component to prevent the manager from overwriting your changes:
```yaml
dependencies:
  espressif/esp_hosted:
    path: "../components/esp_hosted"
  espressif/esp_wifi_remote: "*"
  idf: ">=5.3"
```

## 4. Key `sdkconfig` Settings
Add these to your `sdkconfig` or use `menuconfig` to set them:

```ini
# Core Interface
CONFIG_ESP_HOSTED_ENABLED=y
CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE=y
CONFIG_ESP_SDIO_HOST_INTERFACE=y
CONFIG_ESP_HOSTED_IDF_SLAVE_TARGET="esp32c6"

# SDIO Specifics
CONFIG_ESP_HOSTED_SDIO_SLOT_1=y
CONFIG_ESP_HOSTED_SDIO_BUS_WIDTH=4
CONFIG_ESP_HOSTED_SDIO_CLOCK_FREQ_KHZ=40000
CONFIG_ESP_HOSTED_SDIO_OPTIMIZATION_RX_STREAMING_MODE=y

# Reset Strategy (GPIO 54 for P4 Function Board)
CONFIG_ESP_HOSTED_SLAVE_RESET_ON_EVERY_HOST_BOOTUP=y
CONFIG_ESP_HOSTED_GPIO_SLAVE_RESET_SLAVE=54
CONFIG_ESP_GPIO_SLAVE_RESET_SLAVE=54
CONFIG_ESP_HOSTED_SDIO_GPIO_RESET_SLAVE=54

# WiFi Remote Configuration
CONFIG_ESP_WIFI_REMOTE_ENABLED=y
CONFIG_ESP_WIFI_REMOTE_LIBRARY_HOSTED=y
CONFIG_SLAVE_IDF_TARGET_ESP32C6=y
```

## 5. Software Logic Improvements
In `components/esp_hosted/host/api/src/esp_hosted_api.c`, ensure the `check_transport_up()` macro is modified to wait for the link rather than failing immediately. This prevents scan errors during the initial 1-2 seconds of boot-up.

### Critical Implementation Note
Always initialize the WiFi stack **once** in `app_main`:
1. `nvs_flash_init()`
2. `esp_netif_init()`
3. `esp_event_loop_create_default()`
4. `esp_wifi_init()`
5. `esp_wifi_set_mode()`
6. `esp_wifi_start()`
Then enter your application loop.
