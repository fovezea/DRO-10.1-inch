# ESP-Hosted coprocessor firmware (ESP32-C6)

> **Updated for esp_hosted 3.1.0 on ESP-IDF 6.1.**
> The previous version of this guide said `cd components/espressif__esp_hosted/slave`.
> That project no longer exists: esp_hosted 3.x replaced `slave/` with a `cp/`
> project per example, and the component now lives in `managed_components/`,
> which is wiped on every dependency resolve.

The C6 firmware is vendored in this repository at **`esp32c6_hosted_cp/`**,
copied from esp_hosted 3.1.0 `examples/wifi/sta/cp` and pinned to 3.1.0. See
[`esp32c6_hosted_cp/README.md`](esp32c6_hosted_cp/README.md) for details.

## Build and flash

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
cd esp32c6_hosted_cp
idf.py set-target esp32c6
idf.py build
idf.py -p COMx flash          # COMx = the C6's own USB port, not the P4's
```

Output: `build/eh_cp_wifi_sta.bin` (~1.15 MB).

## Rules

* **Version coupling.** The C6 firmware and the P4 application must use the
  same esp_hosted version. This project pins 3.1.0 in both places
  (`main/idf_component.yml` and `esp32c6_hosted_cp/main/idf_component.yml`).
  Rebuild the C6 whenever esp_hosted changes.
* **Transport coupling.** The C6 runs SDIO (`CONFIG_EH_TRANSPORT_CP_SDIO=y`,
  high speed). The P4 must use the matching `CONFIG_ESP_HOSTED_SDIO_*`
  settings (slot 1, 4-bit, reset GPIO 54).
* **Do not edit the copy under `managed_components/`** — it is regenerated.
  Change `esp32c6_hosted_cp/` instead, and update the vendored copy when
  esp_hosted is bumped.

## Status

Compile-verified with ESP-IDF 6.1 (`eh_cp_wifi_sta.bin`, 0x118d60 bytes).
Not flashed and not runtime-tested. In particular, the SDIO link between the
P4 and this C6 image has not been exercised since the 3.1.0 migration.
