# ESP32-WROOM-32E 5-axis DRO backend firmware

Firmware for the `esp32-DRO-backend` board (Rev C, KiCad project in
`C:/SRC/esp32-DRO-backend`): five quadrature scale inputs, CH340C USB serial,
one status LED. It reads the scales and streams raw counts to the DRO frontend
over **ESP-NOW** or a **wired UART**, using a framed, CRC-checked protocol.

This folder is the backend sub-module of the `DRO-10.1-inch` project.

> **Design rule** (from `DRO_IMPLEMENTATION_ROADMAP.md`): *smart client, dumb
> backend*. This firmware does no coordinate maths, no tool offsets, no unit
> conversion. It reports counts, flags and link statistics. Everything else is
> the frontend's job.

---

## 1. Hardware this firmware targets

From the schematic netlist (`esp32-DRO-backend.kicad_sch`):

| Function | Signal | ESP32 pin | Notes |
|---|---|---|---|
| Scale X | A / B / Z | IO34 / IO35 / IO36 | DB9 J4 |
| Scale Y | A / B / Z | IO39 / IO32 / IO33 | DB9 J5 |
| Scale Z | A / B / Z | IO25 / IO26 / IO27 | DB9 J6 |
| Scale W | A / B / Z | IO14 / IO13 / IO4 | DB9 J7 |
| Scale V | A / B / Z | IO16 / IO17 / IO18 | DB9 J8 |
| Status LED | LED_STAT | IO19 | D3 + R4, active high |
| Console / USB serial | U0TXD / U0RXD | IO1 / IO3 | CH340C and J2 pins 3/4 |
| Boot / reset | IO0 / EN | IO0 / EN | SW2 / SW1 |
| Spare header J3 | — | IO21, IO22, IO23 | 3V3, GND, IO21, IO22, IO23 |

* DB9 pinout: 1 = GND, 2 = +5 V, 3 = A, 4 = B, 5 = Z, 6 = shield.
* The 5 V scale signals go through SRV05-4 ESD arrays, 1 k / 100 pF filters and
  SN74LVC245A level translators before reaching the ESP32.
* IO34–IO39 are input-only and have no internal pulls; that is fine, the
  LVC245 drives them push-pull. The Z lines also have 10 k pull-ups.
* Power: 5 V on J1 (JST-XH) or USB-C. Both are OR-ed by D4/Q1.

The PCNT peripheral does all quadrature decoding in hardware at 4× resolution,
so a 5 µm scale at 1 m/s (~200 kHz of edges) does not load the CPU.

---

## 2. Build and flash (ESP-IDF 6.1)

```powershell
# one-time per shell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1

cd C:\SRC\DRO-10.1-inch\esp32_wroom_dro_backend
idf.py set-target esp32      # writes sdkconfig from sdkconfig.defaults
idf.py build
idf.py -p COM7 flash monitor
```

The project is verified against ESP-IDF **6.1** (and 6.0.2 before it). The app is
about 840 kB, well inside the 1.5 MB single-app partition.

To reconfigure:

```powershell
idf.py menuconfig     # "DRO backend" menu
```

---

## 3. Connecting the frontend

### 3.1 UART2 on the J3 spare header (recommended wired link)

| Backend J3 | Signal | Frontend |
|---|---|---|
| pin 1 | +3V3 | do **not** connect if the frontend powers the link |
| pin 2 | GND | GND |
| pin 3 | IO21 → UART2 TX | frontend RX |
| pin 4 | IO22 → UART2 RX | frontend TX |
| pin 5 | IO23 | reserved |

Default 460800 8N1, no flow control. The console stays on UART0, so you can
watch the board on USB while the frontend talks on J3.

### 3.2 UART0 on J2 (bench mode, no USB-TTL adapter needed)

J2 is the 6-pin programming header: `3V3, GND, TXD0, RXD0, IO0, EN`. The same
UART0 also goes to the CH340C, so **do not** have USB and an external UART
connected at the same time.

Set `CONFIG_DRO_LINK_UART_PORT_0=y` in menuconfig (or edit
`sdkconfig.defaults`). The console is then disabled automatically and logging
is silenced so the wire carries only frames. Connect a USB-TTL adapter or the
frontend to J2 pins 2/3/4 and run `tools/dro_link_test.py --port COMx monitor`.

### 3.3 USB-C to the frontend

The USB-C port is a CH340C USB-UART bridge, not a native USB device. A USB host
(the P4) sees a CDC-ACM serial adapter. That works, but the frontend needs a
CDC-ACM host driver; the current `frontend_usb` component implements HID only.
UART2 is less work.

### 3.4 ESP-NOW

The backend is a plain ESP-NOW station: no AP, no IP stack. It beacons a
`HELLO` every second when no frontend session is active, and answers the peer
that sends `HELLO_REQ`.

**Important limitation:** the ESP32-P4 frontend has no Wi-Fi radio of its own.
Its ESP32-C6 is an ESP-Hosted slave on SDIO, and `esp_wifi_remote` does not
expose the ESP-NOW API — `esp_now_init()` on the P4 has nothing to talk to.
So the backend can transmit ESP-NOW today, but the current P4 HMI cannot receive
it without one of:

1. a local radio on the frontend (an ESP32-C3/S3 added as a small UART↔ESP-NOW
   bridge is the cheapest option), or
2. reflashing the C6 as a standalone ESP-NOW↔UART bridge — this replaces the
   hosted Wi-Fi stack, and
3. implementing ESP-NOW over the ESP-Hosted custom-RPC channel on the C6.

For now: **bring the board up on UART**, and keep ESP-NOW for when the frontend
gets a radio. The firmware already supports both simultaneously; the first
transport that says `HELLO` wins, and the other takes over if it goes quiet for
3 s.

---

## 4. Is UART2 available on this board?

**Not on its default pins, but yes via the GPIO matrix.**

* UART2's default pins are GPIO16 (RX) and GPIO17 (TX). On this board those are
  `V_A3` and `V_B3` — the V-axis encoder inputs from U4. Using UART2 defaults
  would kill the V axis.
* UART1's default pins are GPIO9/GPIO10, which are the SPI flash pins. Not
  usable.
* UART2 can be routed to *any* GPIO through the GPIO matrix. There are exactly
  three uncommitted pins broken out on this board: **IO21, IO22, IO23** on J3.
  This firmware routes UART2 TX → IO21 and RX → IO22 (configurable in
  menuconfig), leaving IO23 free.
* UART0 is also available on J2 (shared with the CH340C), at the cost of the
  console.

So a wired link is entirely practical; it just does not use the default UART2
pins.

---

## 5. Console

115200 8N1 on the CH340C USB port (UART0). Type `help`.

| Command | Meaning |
|---|---|
| `info` | board, pin map, MAC, link settings |
| `status` | live raw counts, position, counts/s, index/overflow counts |
| `diag` | heap, frame counters, per-axis diagnostics |
| `zero [all\|X\|Y\|Z\|W\|V]` | set position to 0 |
| `preset <axis> <count>` | set position |
| `invert <axis> <0\|1>` | flip counting direction, position preserved |
| `enable <axis> <0\|1>` | mark an axis present |
| `index <axis> <0\|1>` | enable Z capture |
| `rate <1..200>` | default telemetry rate |
| `sim <on\|off> [counts/s]` | synthetic counts for testing without scales |
| `link [status\|mode\|channel\|peer\|lock]` | wired/wireless link settings |
| `uart [status\|baud N]` | wired link baud rate |
| `save` / `reset` | persist / restore defaults |
| `selftest` | CRC + COBS + frame round-trip and corruption test |
| `reboot` | restart |

Settings are stored in NVS and auto-saved 2.5 s after a change.

### Status LED (IO19)

| Pattern | Meaning |
|---|---|
| solid on | frontend session active |
| 1 Hz blink | no session yet (beaconing / waiting for HELLO) |
| 5 Hz blink | ESP-NOW failed to start; wired link still works |

---

## 6. Bring-up

See [`docs/BRINGUP.md`](docs/BRINGUP.md) for a staged procedure that matches
`ASSEMBLY.md`: power, console, codec self-test, simulation, UART link, then one
scale at a time.

The fastest sanity check on a freshly soldered board:

```text
dro> selftest
dro> sim on 2000
dro> status          # all five axes should count up
dro> sim off
```

---

## 7. Protocol

Full specification: [`docs/PROTOCOL.md`](docs/PROTOCOL.md).
Shared C header: [`components/dro_link/include/dro_link_protocol.h`](components/dro_link/include/dro_link_protocol.h)
— drop it into the frontend project; it has no ESP-IDF dependencies.

Frames are COBS-encoded, `0x00`-delimited, and protected by CRC-16/CCITT-FALSE.
A session starts with `HELLO_REQ`, telemetry is 32 bytes, and the backend stops
streaming 3 s after the last frame from the frontend.

---

## 8. Layout

```
CMakeLists.txt              ESP-IDF project
sdkconfig.defaults          target defaults (240 MHz, 1 kHz tick, 1.5 MB app)
components/dro_link/        portable wire protocol (shared with the frontend)
main/
  app_main.c                boot order
  board_pins.h              pin map, traced from the KiCad netlist
  axis_reader.c/.h          PCNT quadrature, index, zero/preset/invert, sim
  link.c/.h                 sessions, telemetry, command dispatch
  transport_uart.c/.h       wired link
  transport_espnow.c/.h     ESP-NOW link
  config_store.c/.h         NVS settings + debounced save
  console_cmds.c/.h         setup shell
  status_led.c/.h           LED patterns
tools/dro_link_test.py      PC-side bench tool
docs/PROTOCOL.md            wire protocol
docs/BRINGUP.md             staged test plan
```
