# ESP32-C6 ESP-Hosted coprocessor firmware

Wi-Fi/Bluetooth for the ESP32-P4 frontend is not on the P4 itself: it runs on
the on-board **ESP32-C6** as an ESP-Hosted coprocessor. This project is the C6
side of that link.

It is vendored from **espressif/esp_hosted 3.1.0**,
`examples/wifi/sta/cp`, because in esp_hosted 3.x the old `slave/` project is
gone (it is now a `cp/` project per example) and `managed_components/` is
wiped and regenerated on every dependency resolve. Keeping the C6 project in
the repository makes the firmware reproducible.

The esp_hosted version here **must match** the one the P4 application uses
(`main/idf_component.yml` in the project root). This tree is pinned to 3.1.0.

## Build

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
cd esp32c6_hosted_cp
idf.py set-target esp32c6
idf.py build
```

Output: `build/eh_cp_wifi_sta.bin`, about 1.15 MB, 4 MB flash with an OTA
partition table.

## Transport

SDIO, high speed — the built configuration contains:

```
CONFIG_EH_TRANSPORT_CP_SDIO=y
CONFIG_EH_TRANSPORT_CP_SDIO_HIGH_SPEED=y
```

This must match the P4 side (`CONFIG_ESP_HOSTED_SDIO_*`, slot 1, 4-bit, Reset
on GPIO 54). Do not switch the coprocessor to SPI without changing the host
configuration as well.

## Flash

Connect the C6's own USB port (not the P4's) and flash:

```powershell
idf.py -p COMx flash
```

## When to rebuild

Every time esp_hosted is bumped in the P4 project, and after changing any
coprocessor feature (`idf.py menuconfig` → Component config → ESP-Hosted →
Configure coprocessor). A host/coprocessor version mismatch shows up as the P4
failing to bring the link up after boot.

## Status

Compile-verified with **ESP-IDF 6.1**: `eh_cp_wifi_sta.bin` = 0x118d60 bytes.
It has **not** been flashed or runtime-tested in the IDF 6.1 migration.
