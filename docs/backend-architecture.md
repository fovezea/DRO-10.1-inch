# Backend architecture

Status: design record, no code yet. Two backend products behind one frontend.

## 1. Product split

| | **DRO-lite** (plain DRO) | **ELS plate** (new) |
|---|---|---|
| Purpose | display 5 scales, no gearing | DRO + electronic lead screw, 3 geared slaves |
| MCU | ESP32-WROOM-32E (existing plate) | ESP32-P4 stamp, **single CPU** |
| Counters | 8x PCNT (16-bit, unwrap needed) | 5x **LS7366R** (32-bit, x4 quadrature, index) |
| Step outputs | none | 3x step/dir (RMT/MCPWM) |
| USB to frontend | **CH340C UART bridge** (see §5) | native USB OTG, HID |
| Status | plate under construction, kept as the cheap path | to be designed |

Not every installation needs electronic gearing: a plain DRO is cheaper, and the WROOM plate
already exists. So the WROOM stays as a supported backend rather than a stopgap. The ELS plate
is a separate board, because two CPUs in the backend is not worth it when one P4 plus real
counter chips does the job.

## 2. Counters are the component that decides everything

The P4 has **4 PCNT units** and a full installation has **5 scales**, so at least one counter
must live outside the P4 on *both* boards. The options:

| | 8x PCNT (WROOM) | 5x LS7366R (P4 stamp) |
|---|---|---|
| Width | 16-bit, software unwrap | 32-bit, no wrap |
| Edge decode | PCNT (x1/x2/x4 configurable) | hardware x4 + index latch |
| Gearing loop latency | every geared axis crosses SPI | stays on the P4 |
| MCUs / firmware | 2 + a protocol | 1 |
| Reuses | the existing plate | the existing DB9 + 74LVC245 front end |

Decision: **DRO-lite keeps the WROOM** (it is being built anyway). **ELS plate uses 5x
LS7366R**, so the gearing loop never depends on an inter-MCU link.

Bus load is not a concern: 5 counters x 5 SPI bytes at 10 MHz is ~25 us per 250 us tick, 10 %
of the budget. Run the LS7366Rs at 3.3 V so SPI is 3.3 V end to end; the existing 74LVC245s
already translate the 5 V TTL scales down. A 5 um scale at 1 m/s is 200 kcount/s -- three
orders of magnitude below what the chip does.

## 3. ELS plate

```
5x DB9 (5V TTL) -> 74LVC245A -> 5x LS7366R (3.3V) --SPI--> ESP32-P4 stamp
                                                              |-- RMT/MCPWM: 3x step/dir
                                                              |-- USB OTG: HID to frontend
                                                              |-- 5V in (single source)
```

- **Axes are generic.** Nothing in the hardware or firmware knows "X" or "spindle"; there are
  five counter indices and three step outputs. Names live in the frontend/UI.
- **Gearing is a matrix**: `(master_idx, slave_idx, ratio, offset, enable)`, up to 3 rows
  active. Any axis may be the master of any other, which is what makes the same plate useful
  for a lathe (spindle master) and a mill (one linear axis master).
- **Tick 250 us**, timer-driven: unwrap (DRO-lite only), evaluate the matrix, compute each
  slave's target, emit the difference as pulses. Position-based, never velocity integration:
  a late tick must not become a permanent position error.
- **Pulse generation in hardware** (RMT): the CPU computes targets, it never bit-bangs steps.

## 4. Timing budget (250 us tick)

| Step | Budget |
|---|---|
| SPI: 5x LS7366R counter reads @10 MHz | ~25 us |
| unwrap + gearing matrix (3 rows) | <5 us |
| RMT reload for 3 channels | ~10 us |
| **Total per tick** | **<50 us of 250 us** |

The remaining 200 us is headroom for USB HID servicing (frontend traffic), which is
low-priority and interrupt-driven. If a tick overruns, the design degrades to a longer tick
(slightly coarser gearing), never to lost counts -- because positions are absolute.

## 5. The transport differs, and that is a protocol decision

This is the only thing that does **not** come for free from the two-board split:

- **ELS plate**: native USB -> HID, exactly what `esp32p4_usb_dro_backend` does today. No change.
- **DRO-lite**: the WROOM has **no USB OTG**. Its USB is a CH340C bridge, so the frontend sees
  a USB-UART, not a HID device. That means the frontend needs a second USB class (confirmed available as
  `espressif/usb_host_ch34x_vcp` 2.2.x, with `espressif/usb_host_cdc_acm` 2.4.x as the
  generic fallback if the bridge is ever swapped) and
  the *same application framing carried over CDC*.

Decision: keep **one application protocol** (fixed-size frames, sequence + tick + 8 x 32-bit
positions + status + CRC) and let the USB class be a transport detail. The backend reports its
capabilities in the handshake -- number of axes, ELS present or not -- and the frontend enables
or hides the gearing UI accordingly. That way a DRO-lite installation never sees dead ELS
controls, and the frontend stays one codebase.

Bandwidth check for DRO-lite: 32-byte frames at 1 kHz over 1-2 Mbaud CH340C is ~160-320 us of
UART per frame, so 1 kHz is feasible but not generous. Plain DRO does not need 4 kHz; 200 Hz
is plenty for a readout, which leaves plenty of margin.

## 6. Power

Steady-state draw is modest -- the boards already run from a USB port today. The rail is sized
for **inrush and headroom**, not for average current.

| Load | @5 V |
|---|---|
| 10.1" backlight (SY7200 boost), full brightness | 0.35-0.45 A |
| Frontend P4 + C5 + panel logic | 0.20-0.30 A |
| Backend P4 stamp + 5x LS7366R + buffers | 0.25-0.35 A |
| 5x 5 V TTL scales + 74LVC245 | ~0.10 A |
| **Steady state total** | **0.9-1.2 A** |
| **Peak**: boost startup, co-processor TX burst | **~1.5-1.8 A** |

So a **2 A continuous** supply is ample, and 3 A is design headroom rather than a requirement --
useful if a later plate (Ethernet PHY, a second radio) is added. The backlight is the one
dominant, uncertain term: at half brightness it halves, which is why the number wants measuring
rather than estimating.

Two things the average ignores:

- **Inrush.** The boost and the bulk caps pull a short surge on power-up; a 2 A supply with sane
  capacitance handles it, but a supply that folds back below 1.5 A may not start reliably.
- **Source rating, not rail rating.** A PC USB-A port guarantees 0.5 A and a USB-C data port
  ~0.9 A; either caps the stack well below 1.2 A, at which point full backlight is unreachable
  and the earlier flicker returns. The rail must be fed by a PD sink negotiating >=2 A, or a
  dedicated 5 V feed.

### Power flow through the stack

The quadrature plate is the source; everything above it is a sink, so the frontend's ~0.7 A and
the backend's ~0.3 A pass up through the intermediate board. At ~1.2 A total that is
undemanding: **3-4 parallel pins per rail** with a wide pour between the header rows is
plenty (~10 mV drop across two board-to-board interfaces). The pour still matters for the
*pass-through* -- the middle plate carries current it does not consume, so route it as a pour,
not as tracks.

If the frontend's own USB-C is also plugged in (normal during development, for flashing), that
is two sources on one rail: feed it data-only, or put an ideal-diode OR on the frontend, or the
plate back-feeds the host.

## 7. Next steps

1. **Measure** the frontend's actual 5 V current at full backlight -- turns §6 from estimate
   into a number before the plate's power section is committed.
2. **Schematic first** for the ELS plate: 5x DB9 -> 74LVC245 -> 5x LS7366R (3.3V) -> P4 stamp
   socket, 3x step/dir headers, single 5 V input with the protection from §6.
3. Frontend: add the transport abstraction (HID | CDC) and the capability handshake; this is
   the piece that lets DRO-lite and ELS plates be interchangeable.
4. Firmware split: `esp32p4_usb_dro_backend` gains a BSP variant for the ELS plate (5 counters
   over SPI, 3 step channels) and keeps the existing variants for the current boards.
