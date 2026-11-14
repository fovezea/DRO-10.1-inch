# Backend architecture

Status: design record, no code yet. Two backend products behind one frontend.

## 1. Product split

| | **DRO-lite** (plain DRO) | **ELS plate** (new) |
|---|---|---|
| Purpose | display 5 scales, no gearing | DRO + electronic lead screw, 3 geared slaves |
| MCU | ESP32-WROOM-32E (existing plate) | ESP32-P4 stamp, **single CPU** |
| Counters | 8x PCNT (16-bit, unwrap needed) | 5x **LS7366R** (32-bit, x4 quadrature, index) |
| Step outputs | none | 3x step/dir (RMT/MCPWM) |
| USB to frontend | CH340C bridge, or UART when stacked (see §5) | UART on the stack bus, no USB (see §5) |
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

## 5. Transport: one protocol, three carriers

The stacked products need no USB at all, which removes the most complicated part of the old
design.

| Product | Transport | Why |
|---|---|---|
| DRO / ELS (stacked) | **UART on the stack bus** | no cable, no USB host/device stack, bounded latency |
| Existing USB backends | USB HID | unchanged; `esp32p4_usb_dro_backend` already does this |
| Plain DRO (WROOM plate) | USB-UART via the CH340C bridge | that plate has no USB OTG at all |

For the CH340C path the frontend needs a serial-class host driver -- `espressif/usb_host_ch34x_vcp`
2.2.x is available, with `espressif/usb_host_cdc_acm` 2.4.x as the generic fallback if the
bridge chip is ever swapped.

Decision: keep **one application protocol** -- fixed-size frames, sequence + tick + 5 x 32-bit
positions + status + CRC -- and treat the carrier as a detail. The backend reports its
capabilities in the handshake (number of axes, ELS present or not) and the frontend enables or
hides the gearing UI accordingly, so no installation shows dead controls and the frontend stays
one codebase.

Bandwidth: 32-byte frames at 250 Hz need ~8 kB/s, which is nothing for a 1 Mbaud UART. Even the
CH340C path at 1-2 Mbaud sustains 1 kHz, so the transport never becomes the bottleneck -- the
250 us tick on the backend does.

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

## 7. Stacking platform

The frontend breaks every GPIO out on 2x2x17 side headers, so a plate sits directly on its back
with no cables. The stack is **two levels**: frontend on top, one backend plate below carrying
the P4, the five LS7366Rs and the 5 V input. Counters and P4 share that plate precisely so the
10 MHz SPI never crosses a connector -- the reason a three-level stack was rejected.

```
[ frontend P4C5 ]        panel, touch, audio, TF, Wi-Fi
      || headers: UART + IRQ + reset + boot + 5V/GND
[ backend plate ]        M5Stamp + 5x LS7366R + DB9s + 5V input
                         (+ SPI between P4 and counters, on-board)
```

Consequences:

- **The bus carries no high-speed signals** -- five control lines plus power, so 2.54 mm headers
  are entirely adequate and the interface cannot become an EMI problem.
- **Power flows upward** from the bottom plate: it is the source, the frontend is a sink of
  ~0.7 A. At ~1.2 A total, 3-4 pins per rail with a wide pour is ample.
- **A plate may bring bulk connectors instead of stacking under them.** The PoE-Ethernet adapter
  occupies the slot rather than having a board above it, so the LAN bridge is *PoE plate instead
  of backend plate*, not both.
- **The same bus serves other products**: Zigbee coordinator and LAN bridge reuse the frontend
  with no or different plates. See `docs/stack-bus.md` for the pin contract.

## 8. Backend MCU module

The backend plate is small -- it sits behind a 72x52 mm dev board and carries the counters,
their buffers, five DB9s and a 5 V input. A full-size 88-pin stamp would turn it into a second
full board, so the module is chosen for compactness, not pin count.

**Chosen: M5Stamp ESP32P4**, built on the Espressif **ESP32-P4NRW32** module. P4-only, so no
radio the product does not need, and compact enough to sit beside five LS7366R on one plate.

Confirmed from the vendor's own product data:

| Item | Value | Consequence |
|---|---|---|
| Module | ESP32-P4NRW32 | Espressif module, so its datasheet is authoritative |
| Clock | 360 MHz + 40 MHz LP core | **P4 rev v1.x** |
| Memory | 16 MB flash + 32 MB PSRAM | matches the osptek P4-Core |
| Package | **SMT and DIP** (castellated + 2.54 mm) | the first plate can be **socketed**, not reflowed |
| Multimedia | MIPI CSI/DSI broken out | so GPIO breakout is not a concern -- it exposes far more than the ~20 needed |

Still to confirm before layout:

1. **Supply.** P4 modules are 3.3 V; the plate then needs its own 5 V -> 3.3 V buck feeding both
   the module and the five LS7366Rs. Confirm whether the module expects 3.3 V only, and whether
   the MIPI PHY rails (2.8 V / 1.8 V) are required when MIPI is unused -- for a backend with no
   display they should be tie-off-able, as on the osptek P4-Core.
2. **Pin count and pitch.** "SMT/DIP" means it can be socketed, which is what makes a first
   prototype plate practical. Get the number of castellated/DIP pins and their arrangement from
   the ESP32-P4NRW32 datasheet before drawing the footprint.

### Module selection filter

The counter SPI is the plate's core function, so it is the first filter any candidate module
must pass -- cheaper modules have already failed it.

1. **>=20 free GPIOs.** SPI2 needs no dedicated pads: the ESP32-P4 IO MUX lists dedicated
   `SPI2_CK/D/Q/CS` on GPIO6-11 only as the default alias (F3), and the GPIO matrix routes SPI2
   to *any* GPIO. At the 10 MHz this plate runs, matrix routing is more than adequate. So a
   module is judged on how many free GPIOs it exposes, not on whether it happens to name them
   SPI2.
2. **Eight of those for the counter bus** (3 signals + 5 CS), plus 2 UART, IRQ, reset, boot and
   3 step/dir.
3. **P4-only**, no radio co-processor: the frontend already has one.
   (GPIO2-5 default to MTCK/MTDI/MTMS/MTDO, so using them as GPIO costs JTAG debug -- acceptable
   on a plate, but not the first choice.)
4. **Socketable**, 2.54 mm preferred -- a socketed first prototype is worth more than a cheaper
   module that must be reflowed to be evaluated.

### Chip capability is not the constraint

From the ESP32-P4 (QFN104) pin layout, for the record:

- **55 GPIOs**, GPIO0-54, and **SPI2 routes to any of them** through the GPIO matrix. The filter
  above is therefore about what a *module* breaks out, never about what the chip can do.
- **MIPI DSI/CSI (34-48), USB DM/DP (49/50) and the FLASH_* group (27-33) are dedicated pads**,
  so they do not consume GPIOs. That is why a display-oriented module can look pin-rich and
  still be SPI-poor: it is a breakout choice, not silicon.
- **The chip needs a dozen rails** -- VDD_DCDC/FB_DCDC/EN_DCDC, VDD_HP_0-3, VDD_IO_0-6, VDD_LP,
  VDD_ANA, VDD_LDO, VDD_PSRAM_0/1, VDDO_PSRAM, VDDO_FLASH, VDD_MIPI_DPHY, VDD_USBPHY, VDD_BAT.
  A bare QFN104 is not hand-assembly, which is exactly why the plate sockets a module.

What remains to check is the module's own pin list: whether it exposes >=20 of those GPIOs,
including the eight SPI2-capable ones the counters need.

A candidate was rejected on point 1 during the design: it exposed plenty of pins, but not the
eight SPI2-capable ones the plate needs.

**Fallback: osptek ESP32-P4-Core** -- P4-only (no radio), 88-pin stamp, 16 MB flash / 32 MB
PSRAM, 360 MHz (rev v1.x), fully documented, and its pin definition is already mirrored in this
project. Larger, so the plate becomes full-size, but its specs are not in question. Note it is
not footprint-compatible with the frontend's P4C5 stamp: the same physical positions are the
C5's UART on one module and LDO enables on the other.

Because both candidates are rev v1.x, the backend firmware sits in the same revision family as
the Guition frontend -- the existing rev-family switch covers it with no new mechanism.

> **Revision is the purchasing constraint.** ESP32-P4 ships as v1.x (360 MHz, shield unmarked)
> and v3.x (400 MHz, shield marked "X"); firmware for one refuses to run on the other.

### Product line-up

| Product | Frontend | Plate |
|---|---|---|
| DRO / ELS | osptek P4C5 dev board | M5Stamp + 5x LS7366R + power |
| Plain DRO | osptek P4C5 dev board | existing WROOM scale plate |
| Zigbee coordinator / LAN bridge | osptek P4C5 dev board | none, or the PoE-Ethernet adapter |

The Zigbee case needs no backend board: the C5 has IEEE 802.15.4 in hardware and esp_hosted
ships an `openthread` example, so it is the same frontend plus firmware.

## 9. Next steps

1. **Measure** the frontend's actual 5 V current at full backlight -- turns §6 from estimate
   into a number before the plate's power section is committed.
2. **Schematic first** for the ELS plate: 5x DB9 -> 74LVC245 -> 5x LS7366R (3.3V) -> P4 stamp
   socket, 3x step/dir headers, single 5 V input with the protection from §6.
3. Frontend: add the transport abstraction (HID | CDC) and the capability handshake; this is
   the piece that lets DRO-lite and ELS plates be interchangeable.
4. Firmware split: `esp32p4_usb_dro_backend` gains a BSP variant for the ELS plate (5 counters
   over SPI, 3 step channels) and keeps the existing variants for the current boards.
