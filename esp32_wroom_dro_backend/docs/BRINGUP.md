# Bring-up and test plan

This follows `ASSEMBLY.md` in the hardware repo. The goal is to prove one
subsystem at a time, so a fault is always in the thing you just added.

The firmware is arranged so you can test almost everything **before any scale
is connected**: `selftest` proves the protocol code, and `sim on` produces
synthetic counts through the whole telemetry path.

---

## 0. What you need

* 5 V bench supply (200 mA limit) or a USB-C cable
* Multimeter; a scope or logic analyser is very useful from step 6
* A USB-TTL adapter (3.3 V) if you want to test the J3 link without the frontend
* A scale, or a two-channel generator able to produce A/B 90° apart

---

## 1. Power and first boot (ASSEMBLY stages 1–3)

Follow stages 1–3 of `ASSEMBLY.md` first: 3.3 V rail, ESP32, CH340.

Flash:

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
cd C:\SRC\DRO-10.1-inch\esp32_wroom_dro_backend
idf.py -p COM7 flash monitor
```

Expected boot log (order matters):

```text
I (xxx) main: ESP32-WROOM-32E 5-axis DRO backend ...
I (xxx) axis: [X] A=IO34 B=IO35 Z=IO36
...
I (xxx) axis: 5 axes running at 1 kHz, 4x quadrature, index capture armed
I (xxx) link: transport registered: uart
I (xxx) uart_link: UART2 link on TX=IO21 RX=IO22 at 460800 baud
I (xxx) espnow: ESP-NOW ready on channel 6
I (xxx) link: link ready, device MAC xx:xx:xx:xx:xx:xx
I (xxx) console: console ready - type 'help'
```

At this point the status LED should blink at 1 Hz (no frontend session yet).

If there is no `espnow` line, the Wi-Fi calibration failed; the wired link still
works and `status` will say `wifi: ERROR`.

---

## 2. Console smoke test

```text
dro> info
dro> status
```

`info` must show the five axis pin groups, the UART2 pins (21/22) and the
ESP-NOW channel. `status` must show five rows with plausible raw counts.

---

## 3. Codec self-test — no hardware involved

```text
dro> selftest
crc16    : 0x29b1 (expect 0x29b1)
vector   : OK (42 bytes)
cobs     : 64 -> 65 -> 64
frame    : 42 bytes, decode OK
corrupt  : rejected (OK)
selftest : PASS
```

This checks:

* the CRC implementation against the standard check value,
* the encoder against a **fixed wire vector** that the Python tool also
  produces (if the two disagree, one of them is wrong),
* COBS round-trip with embedded zeros,
* a full frame encode/decode,
* that a single flipped bit is rejected.

If this fails, stop: the link will never work.

---

## 4. Simulated counts — proves the telemetry path

```text
dro> sim on 2000
dro> status
```

All five axes should count up at ~2000 counts/s. `sim off` stops them and the
displayed position stays continuous (the synthetic travel is folded into the
origin, not thrown away).

While `sim` is on, `CONFIG` reports `DRO_ST_SIMULATION` in every telemetry
frame, so the frontend can show a "simulated" badge.

---

## 5. Wired link on J3 (UART2)

Wire only these three:

| Backend J3 | Signal | USB-TTL / frontend |
|---|---|---|
| pin 2 | GND | GND |
| pin 3 | IO21, UART2 TX | RX |
| pin 4 | IO22, UART2 RX | TX |

Do not connect J3 pin 1 (+3V3) to a USB-TTL that also supplies 3.3 V.

With the console still on USB, run:

```powershell
C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe tools\dro_link_test.py --port COM8 monitor
```

Expected:

```text
backend   : wroom-dro  fw 1.0  protocol 1  axes 5
mac       : ...
features  : quad, index, espnow, uart, sim

     time            X          Y          Z          W          V
------------------------------------------------------------------------------
    1.02s        1234      -5678          0          0          0   [uart]
```

Checklist:

* The tool must print a `HELLO` reply. If it times out, the backend is not
  hearing you: swap TX/RX, check GND, confirm 460800 8N1.
* `monitor` sends `HELLO_REQ` once per second; that is what keeps the session
  alive. Closing the tool for 3 s stops the stream — by design.
* `zero X`, `preset X 1000`, `diag`, `ping` should all answer within a few ms.

---

## 6. UART0 bench mode (no USB-TTL adapter)

If you only have the USB-C cable, the link can share UART0 with the CH340:

1. `idf.py menuconfig` → **DRO backend** → link UART → `UART0`.
2. Reflash.
3. The console is now disabled and logs are silenced; the port carries frames
   only.
4. Point the tool at the same COM port the console used.

Trade-off: you lose the shell while the binary link is active. For serious
debugging, use J3 and keep the console.

---

## 7. One scale at a time (ASSEMBLY stages 4–5)

Repeat for X, Y, Z, W, V. Connect the DB9, power the scale, then move the
carriage **by hand** and watch:

```text
dro> status
axis  en dir   raw count        position     counts/s  idx  ovf
 X    on  +          12345            12345      1200    0    0
```

Good signs:

* counts increase when you move one way and decrease the other,
* `counts/s` tracks your hand speed,
* `idx` increments once per reference mark,
* `ovf` stays 0 (it only increments after 32 000 counts of continuous travel
  in one direction, which is normal on a long scale).

### Resolution check

Move the axis exactly 100 mm. The raw count should change by

```
100 mm / resolution
```

e.g. a 5 µm scale → 20 000 counts; a 1 µm scale → 100 000 counts. This catches
a missing channel (2× instead of 4×) immediately.

### Direction / zero / preset

```text
dro> zero X            # position 0 at the current point
dro> invert X 1        # direction flips, position does not jump
dro> preset X 1000     # position becomes 1000 at the current point
```

### About single-channel signals

A square wave on **A only**, with B held static, produces **zero net counts**
by design: it is not a valid quadrature sequence. That is the decoder working,
not a fault. To test a port with a generator, drive A and B 90° apart; otherwise
use `sim` for the data path and a scope for the electrical path.

---

## 8. Index (Z) pulses

* `idx` only increments if `index <axis> 1` (all axes are enabled by default).
* The backend latches the count in a dedicated task woken by the GPIO ISR, so
  the captured position is typically within tens of microseconds of the pulse.
* Each pulse is also sent as a `INDEX_EVENT` frame to an active frontend, even
  if telemetry is slow. The frontend can use it as a reference mark.
* If `idx` never moves: check the Z line at the LVC245 output, and the 10 k
  pull-up (R21–R25). A floating Z may read as a permanent high.

---

## 9. ESP-NOW

The backend is ready, but remember: **the ESP32-P4 frontend has no radio.**
See `README.md` §3.4. Until the frontend has a local ESP32 (or the C6 is
repurposed as a bridge), ESP-NOW cannot be received.

When you do have a peer:

1. Put both ends on the same channel: `link channel 6` (or set it in the frontend).
2. From the peer, send a `HELLO_REQ` to `FF:FF:FF:FF:FF:FF`; the backend answers
   with `HELLO` and starts streaming.
3. Watch `status` on the console: the session line changes to `espnow`.
4. Lock it down if you want: `link peer aa:bb:cc:dd:ee:ff` then `link lock 1`.

Useful checks:

* `link` on the console prints `espnow: ready, channel N`.
* If the LED blinks at 5 Hz, Wi-Fi failed to start — use the UART.
* Range: the board's antenna is at the right edge; keep metal out of the
  keep-out area (`x = 214…230 mm, y = 36…84 mm`).

---

## 10. Frontend integration checklist

1. Copy `components/dro_link/` into the frontend project.
2. Feed received bytes into the delimiter parser (`docs/PROTOCOL.md` §7).
3. Send `HELLO_REQ` at 1 Hz; parse `HELLO`.
4. On `TELEMETRY`, copy `pos[5]` into `dro_set_raw_counts(i, pos[i])`.
5. On `INDEX_EVENT`, record the reference mark.
6. Treat a 3 s silence as "backend lost"; re-send `HELLO_REQ` when it returns.
7. If you add fields, append them and check `payload_len`, never assume.

---

## 11. Performance expectations

| Quantity | Value |
|---|---|
| Axis sampling | 1 kHz |
| Quadrature resolution | 4× (both edges of both channels) |
| Internal count width | 64-bit |
| Wire count width | 32-bit, wraps |
| Default telemetry | 50 Hz |
| Maximum telemetry | 200 Hz |
| Wire overhead | 10 bytes per frame + COBS (~1 byte per 254) |
| Session timeout | 3 s |
| Index latch latency | tens of microseconds |

---

## 12. Troubleshooting

| Symptom | Likely cause |
|---|---|
| No boot log | 3.3 V rail, EN pull-up (R2/C7), USB cable is charge-only |
| Console works but `selftest` FAIL | wrong firmware flashed; rebuild and reflash |
| `sim` moves positions but `status` shows 0 counts/s | normal: `counts/s` is the real encoder rate, not the simulated one |
| Axis counts but only in one direction | B line open / not soldered at the LVC245 |
| Counts are half of expected | one channel missing (2× instead of 4×) |
| Counts jump randomly | noise: check the 100 pF cap, cable shield, scale ground |
| V axis dead, UART2 works | you enabled UART2's default pins (16/17) — use J3 21/22 |
| Index never increments | Z line, 10 k pull-up, `index <axis> 1` |
| Link tool sees no HELLO | TX/RX swapped, baud mismatch, or the console is on the same UART |
| `espnow: ERROR` | Wi-Fi init failed; board still works on UART |
| Telemetry stops after 3 s | frontend is not sending keepalive frames |
| Settings lost after reboot | NVS write failed; check `status` for `(unsaved)` and run `save` |

---

## 13. Known limitations (v1)

* **No ESP-NOW receiver on the current P4 frontend.** Hardware limitation, see
  `README.md` §3.4.
* **No OTA.** The 1.5 MB single-app partition has no second slot. Add a dual-OTA
  partition table when the firmware stabilises.
* **No spindle input on this board.** The P4 ELS board owns the spindle encoder
  and step/dir outputs; this board is scales only, as planned for the combined
  system.
* **No encryption on ESP-NOW.** Fine for a point-to-point bench link; add an LMK
  before deploying next to other 802.11 traffic.
* **Index capture is a task wake-up, not a hardware latch.** Good to tens of
  microseconds, not to a single count at very high speed.
