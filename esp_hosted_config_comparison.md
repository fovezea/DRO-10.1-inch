# ESP-Hosted Configuration Comparison

This document compares the ESP-Hosted settings across three configuration files:
- **Current `sdkconfig`** - Uses SPI interface
- **`sdkconfig-waveshare`** - Uses SDIO interface  
- **`sdkconfig-brochesia`** - Uses SDIO interface

## Summary Table

| Setting | Current (SPI) | Waveshare (SDIO) | Brochesia (SDIO) |
|---------|---------------|------------------|-------------------|
| **Interface Type** | SPI | SDIO | SDIO |
| **Slave Target** | ESP32-C6 | ESP32-C6 | ESP32-C6 |
| **Reset GPIO** | GPIO 12 | GPIO 54 | GPIO 54 |
| **Clock Frequency** | 40 MHz | 40 MHz | 40 MHz |
| **Queue Sizes** | TX=20, RX=20 | TX=20, RX=20 | TX=20, RX=20 |
| **Data Throttle** | Enabled (High=90%, Low=60%) | Enabled | Enabled (High=80%, Low=60%) |
| **Memory Pool** | Enabled | Not shown | Enabled |

---

## Detailed Comparison

### 1. Interface Configuration

#### Current `sdkconfig` (SPI)
```ini
CONFIG_ESP_HOSTED_SPI_HOST_INTERFACE=y
CONFIG_ESP_HOSTED_SPI_MODE=3
CONFIG_ESP_HOSTED_SPI_HSPI=y
CONFIG_ESP_HOSTED_SPI_CONTROLLER=1
CONFIG_ESP_HOSTED_SPI_CLK_FREQ=40
```

**SPI GPIO Pins:**
- MOSI: GPIO 8
- MISO: GPIO 10
- CLK: GPIO 9
- CS: GPIO 7
- Handshake: GPIO 6
- Data Ready: GPIO 11
- Reset: GPIO 12

#### `sdkconfig-waveshare` (SDIO)
```ini
CONFIG_ESP_SDIO_HOST_INTERFACE=y
CONFIG_ESP_SDIO_4_BIT_BUS=y
CONFIG_ESP_SDIO_BUS_WIDTH=4
CONFIG_ESP_SDIO_CLOCK_FREQ_KHZ=40000
CONFIG_ESP_SDIO_OPTIMIZATION_RX_STREAMING_MODE=y
```

**SDIO GPIO Pins:**
- CMD: GPIO 19
- CLK: GPIO 18
- D0: GPIO 14
- D1: GPIO 15
- D2: GPIO 16
- D3: GPIO 17
- Reset: GPIO 54

#### `sdkconfig-brochesia` (SDIO)
```ini
CONFIG_ESP_SDIO_HOST_INTERFACE=y
CONFIG_ESP_SDIO_4_BIT_BUS=y
CONFIG_ESP_SDIO_BUS_WIDTH=4
CONFIG_ESP_SDIO_CLOCK_FREQ_KHZ=40000
CONFIG_ESP_SDIO_OPTIMIZATION_RX_STREAMING_MODE=y
```

**SDIO GPIO Pins:** (Same as Waveshare)
- CMD: GPIO 19
- CLK: GPIO 18
- D0: GPIO 14
- D1: GPIO 15
- D2: GPIO 16
- D3: GPIO 17
- Reset: GPIO 54

---

### 2. Task Configuration

| Setting | Current | Waveshare | Brochesia |
|---------|---------|-----------|-----------|
| RPC Task Stack | 4096 | 4096 | 4096 |
| Default Task Stack | 3072 | 3072 | 3072 |

---

### 3. Data Throttle Settings

#### Current `sdkconfig` (SPI)
```ini
CONFIG_ESP_HOSTED_HOST_TO_ESP_WIFI_DATA_THROTTLE=y
CONFIG_ESP_HOSTED_PRIV_WIFI_TX_SPI_HIGH_THRESHOLD=90
CONFIG_ESP_HOSTED_TO_WIFI_DATA_THROTTLE_HIGH_THRESHOLD=90
CONFIG_ESP_HOSTED_TO_WIFI_DATA_THROTTLE_LOW_THRESHOLD=60
```

#### `sdkconfig-waveshare` (SDIO)
```ini
CONFIG_HOST_TO_ESP_WIFI_DATA_THROTTLE=y
# No specific threshold values shown
```

#### `sdkconfig-brochesia` (SDIO)
```ini
CONFIG_HOST_TO_ESP_WIFI_DATA_THROTTLE=y
CONFIG_PRIV_WIFI_TX_SDIO_HIGH_THRESHOLD=80
CONFIG_TO_WIFI_DATA_THROTTLE_HIGH_THRESHOLD=80
CONFIG_TO_WIFI_DATA_THROTTLE_LOW_THRESHOLD=60
```

---

### 4. Memory Pool Settings

#### Current `sdkconfig`
```ini
CONFIG_ESP_HOSTED_USE_MEMPOOL=y
CONFIG_ESP_HOSTED_MAX_SIMULTANEOUS_SYNC_RPC_REQUESTS=5
CONFIG_ESP_HOSTED_MAX_SIMULTANEOUS_ASYNC_RPC_REQUESTS=5
```

#### `sdkconfig-brochesia`
```ini
CONFIG_ESP_USE_MEMPOOL=y
CONFIG_ESP_MAX_SIMULTANEOUS_SYNC_RPC_REQUESTS=5
CONFIG_ESP_MAX_SIMULTANEOUS_ASYNC_RPC_REQUESTS=5
```

---

### 5. Additional Features

#### Current `sdkconfig` (SPI)
- ✅ ITWT enabled (`CONFIG_ESP_HOSTED_ENABLE_ITWT=y`)
- ✅ CLI enabled (`CONFIG_ESP_HOSTED_CLI_ENABLED=y`)
- ✅ Reset on boot (`CONFIG_ESP_HOSTED_SLAVE_RESET_ON_EVERY_HOST_BOOTUP=y`)

#### `sdkconfig-waveshare` & `sdkconfig-brochesia` (SDIO)
- Reset on boot: Not explicitly shown (may be default)
- ITWT: Not shown
- CLI: Not shown

---

## Key Differences Summary

### Interface Type
- **Current**: Uses **SPI** interface (full-duplex, Mode 3)
- **Waveshare & Brochesia**: Use **SDIO** interface (4-bit bus)

### Reset Pin
- **Current**: GPIO 12
- **Waveshare & Brochesia**: GPIO 54

### Data Throttle Thresholds
- **Current (SPI)**: High=90%, Low=60%
- **Brochesia (SDIO)**: High=80%, Low=60%
- **Waveshare**: Not specified (may use defaults)

### Memory Pool
- **Current**: Uses `CONFIG_ESP_HOSTED_USE_MEMPOOL`
- **Brochesia**: Uses `CONFIG_ESP_USE_MEMPOOL`
- **Waveshare**: Not shown

---

## Recommendations

Based on your `esp_hosted_specs.md` document, which specifies **SPI Mode 3** with GPIO 12 for reset, your **current `sdkconfig` matches the documented specification**.

However, if you're experiencing connection issues with the slave device, consider:

1. **If hardware uses SDIO**: Switch to `sdkconfig-waveshare` or `sdkconfig-brochesia` configuration
2. **If hardware uses SPI**: Keep current config but verify GPIO pin assignments match your hardware
3. **Reset pin mismatch**: Ensure GPIO 12 vs GPIO 54 matches your actual hardware wiring

The `sdkconfig-brochesia` appears to be the most complete SDIO configuration with explicit data throttle thresholds.

