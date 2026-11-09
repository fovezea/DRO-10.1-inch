# Building this project with ESP-IDF 6.1

The project builds cleanly with **ESP-IDF 6.1** (ESP32-P4 target). The changes
are limited to dependency versions, one callback signature and one LVGL
compatibility define; no application logic changed.

## Changes

| Area | Change | Why |
|---|---|---|
| `esp_hosted` | `3.1.0` | the IDF 6.1 release of the hosted stack |
| `esp_codec_dev` | `1.6.2` (was 1.3.2) | 1.3.2 includes the pre-6 `driver/i2s_std.h` style headers; 1.6.x selects `esp_driver_*` on IDF 6 |
| `esp32_p4_function_ev_board` | exact `esp_codec_dev` pin relaxed to `>=1.3.2` | allows the bump above |
| `esp-audio-player` | `2.0.1` (was 1.0.x) | same `driver/i2s_std.h` problem |
| `bsp_extra_i2s_write()` | buffer is now `const void *` | audio player 2.x `write_fn` is const-correct |
| `CMakeLists.txt` | `LV_COLOR_FORMAT_RGB565_SWAPPED` aliased to `RGB565` | esp_lvgl_port 2.9 references it, but the enum only exists in LVGL 9.3; this project stays on 9.2 |

Remove the LVGL alias when LVGL is bumped to 9.3.

## Windows long paths

esp_hosted 3.1.0's archive contains an example file whose full path under the
default cache (`%LOCALAPPDATA%\Espressif\ComponentManager\Cache`) is ~270
characters:

```
...\espressif__esp_hosted_3.1.0_22b08509\examples\power_save\host+cp\
network_split__host_deep_sleep_cp_light_sleep\cp\components\esp_hosted_examples_common\include\esp_hosted_examples_common.h
```

Both the component manager (Python's zipfile) and CMake must be able to address
paths longer than `MAX_PATH`. On this machine it is enabled:

```
HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnabled = 1
```

so no extra configuration is needed — `idf.py build` works with the default
component cache.

If it is disabled (the Windows default), either enable it (admin, no reboot):

```powershell
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" `
  -Name LongPathsEnabled -Value 1 -PropertyType DWORD -Force
```

or put the component cache on a short path, which brings the offending path
down to ~215 characters:

```powershell
$env:IDF_COMPONENT_CACHE_PATH = "C:\cm"
idf.py build
```

Either way only the cache is affected; the project can stay wherever it is, and
`managed_components/` remains a normal directory.
