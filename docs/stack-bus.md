# Stack bus: pins, buses, and the contract between plates

The frontend breaks every GPIO out on its 2x2x17 headers, so a plate can sit on the back with no
cables. This document is the **contract**: which pins the frontend already owns, which are free,
what each bus is for, and the rules a plate design must follow.

**The stack is two levels: frontend plus one plate.** The plate carries the backend P4 *and*
the quadrature receivers on the same board, which keeps the 10 MHz counter SPI entirely on that
plate -- it never crosses a connector. Splitting them onto two plates was considered and
rejected for exactly that reason.

Three consequences worth stating up front:

1. **USB between frontend and backend disappears.** The link becomes a 2-wire UART on the
   stack bus. No USB host stack on the frontend, no HID/CDC class, no CH340/VCP driver, no
   cable. USB on the frontend stays for flashing only.
2. **The bus therefore carries no high-speed signals.** Only the control UART, a few handshake
   lines and power -- which is why 2.54 mm headers are perfectly adequate here.
3. **The counter SPI is a plate-internal bus.** It appears in this document only as a reserved
   pin block for the variant where the counters get their own plate; if that never happens,
   those pins stay free.

## 1. What the frontend already owns

Fixed by the frontend's own peripherals; a plate must treat these as reserved.

| GPIO | Function |
|---|---|
| 7, 8 | I2C: JD9366 in-cell touch + ES8311 codec |
| 9, 10, 11, 12, 13 | I2S: DOUT 9, LRCK 10, DIN 11, SCLK 12, MCLK 13 |
| 14-19 | SDIO link to the ESP32-C5 (D0-D3 14-17, CLK 18, CMD 19) |
| 39-44 | TF card (SD1: CDATA0-3 39-42, CCLK 43, CCMD 44) |
| 45 | TF card detect (also a strapping pin) |
| 53 | NS4150B power-amp enable |
| 54 | C5 chip enable / reset (driven by esp_hosted) |
| 6 | C5 GPIO2 handshake line |
| 0 | boot strapping |
| 2-5 | JTAG pads (MTCK/MTMS/MTDI/MTDO) -- usable only if JTAG is not needed |

Also unavailable by nature: the DSI and CSI lanes, USB DM/DP, and the flash/PSRAM pads.

## 2. Free for the bus

`20-38` and `47-52` are the safe block: not used by any frontend peripheral, and clear of the
strapping and JTAG pads. `1` and `46` are free but are strapping-sensitive, keep them as last
resort.

That is roughly **25 usable signals**, against a need of about 20 for a three-level stack.

## 3. Proposed allocation

Two levels: frontend (top) and one backend plate (bottom) carrying P4 + 5x LS7366R + power
input. The bus is small:

| Signal | GPIO (frontend side) | Direction | Notes |
|---|---|---|---|
| `BB_UART_TX` | 20 | frontend -> backend | UART1 or UART2, 1 Mbaud is ample |
| `BB_UART_RX` | 21 | backend -> frontend | |
| `BB_IRQ` | 22 | backend -> frontend | optional; line-rate events, otherwise polled |
| `BB_RESET` | 23 | frontend -> backend | lets the frontend recover a wedged backend |
| `BB_BOOT` | 24 | frontend -> backend | optional: backend download mode without a button |
| spare | 25-38, 47-52 | -- | reserved for the next plate (PoE, radio, split counters) |

That is **five signals** against ~25 free pins: the bus is dominated by power, not by signals,
and the mechanical footprint is set by the headers rather than by routing.

### Variant: counters on their own plate

If the quadrature receivers ever move to a separate plate (more axes, isolation, a different
scale standard), these become the bus signals -- reserved now so that plate stays possible:

| Signal | GPIO | Notes |
|---|---|---|
| `SPI_SCK` / `MOSI` / `MISO` | 25 / 26 / 27 | 10 MHz, interleave GND between them |
| `CS0..CS4` | 28-32 | one per LS7366R |
| `CNT_DRDY` / `CNT_RESET` | 33 / 34 | optional |

### Power pins

| Rail | Pins | Notes |
|---|---|---|
| 5 V | 4 | ~1.2 A steady state, 4 pins gives ~0.3 A each and ~10 mV across two interfaces |
| 3V3 | 2 | housekeeping only; each plate makes its own rails where it needs them |
| GND | 6+ | interleaved with the signals, not bunched at one end |

The bottom plate is the **source**; everything above it is a **sink** and must pass the rail it
does not consume through a wide pour between the header rows, not through tracks.

### A routing constraint on the counter SPI

The ESP32-P4's dedicated `SPI2_CS/D/CK/Q/WP/HOLD` alias sits on **GPIO6-11**, and on this
frontend every one of those is already spoken for: GPIO6 is the C5 handshake line, 7/8 are the
touch/codec I2C, and 9-13 are audio I2S. So the plate cannot use SPI2's default pads -- it must
route SPI2 through the **GPIO matrix** onto the free block (20-38, 47-52).

That is fine at the 10 MHz this bus runs: matrix routing is a crossbar, and the dedicated pads
only matter at the tens-of-MHz rates the counters never need. It does mean the pin choice is
free rather than dictated by the IO MUX, which is why the allocation above is a proposal rather
than a constraint.

## 4. Frontend-backend transport

**Proposed: UART.** Not forced by the silicon -- SPI would work -- but it is the better fit for
what this link carries, and the choice is worth stating explicitly rather than assuming.

The requirement is small: 5 axes x 4 bytes + sequence + tick + CRC is about 32 bytes, so 8 kB/s
at 250 Hz and 32 kB/s at 1 kHz. The ELS loop itself runs on the backend, so nothing time-critical
crosses this link.

| | UART (2 wires + IRQ) | SPI (4 wires + IRQ) |
|---|---|---|
| Throughput | 100 kB/s @1 Mbaud, 300 @3 | ~1.2 MB/s @10 MHz |
| 32-byte frame | 320 us @1 Mbaud, 107 us @3 | 26 us |
| Duty at 250 Hz | 8 % @1 Mbaud | 0.7 % |
| Clocking | async, no skew across a connector | shared clock, needs solid grounding |
| Framing | byte framing required (length+CRC or COBS) | hardware-framed, CS delimits |
| Roles | symmetric | master/slave, frontend must be master |
| Pins | 2 | 4 |

UART wins on simplicity: no clock crossing a connector, no role asymmetry, half the pins, and 8 %
duty even at 1 Mbaud. SPI wins only if the backend ever has to stream something dense -- raw
4 kHz counter data for a scope-style view, or a future plate with many more channels. That is a
product question, not a technical one, and the cost of being wrong in the UART direction is a
respin; the cost of reserving the two extra pins for SPI now is nothing.

Whatever the carrier: keep the **same frame layout as the USB HID protocol** (sequence, tick,
positions, status, CRC) and the capability handshake, so the frontend has one parser and three
transports (HID, UART-on-stack, CDC).

Bandwidth check for the plain-DRO plate: 32-byte frames at 1 kHz over the CH340C at 1-2 Mbaud
is 160-320 us of UART per frame -- feasible but not generous. A readout does not need 1 kHz;
200 Hz is plenty and leaves large margin.

## 5. Rules a plate must follow

1. **Reserve, don't discover.** If a plate needs a pin outside the table above, add it here
   first. Two plates inventing the same pin two years apart is the failure mode this document
   exists to prevent.
2. **A plate may source or sink 5 V, never both.** The plate with the power inlet is the source;
   it must not also accept power from above.
3. **No plate drives a signal the frontend owns** (DSI, I2C, I2S, SDIO, TF). Those pins are on
   the header only because all GPIOs are broken out.
4. **Opto or buffer anything from the machine side.** The 5 V TTL scales arrive on DB9s from
   outside the enclosure; they go through the 74LVC245 the same way the WROOM plate already
   does.
5. **One plate, one purpose.** A plate that brings its own bulk connectors (the PoE adapter)
   occupies the slot rather than having another board stacked above it.

## 6. Open item before drawing anything

The table above assigns **functions to GPIO numbers**, which is the part that has to be agreed.
The final step is mapping those GPIOs to **physical header positions** (which pin of J2/J4 on
the dev board carries GPIO25, etc.) -- that mapping lives in the baseboard schematic, not in the
vendor README, so it must be read off the drawing pin by pin before the plate's footprint is
laid out. Getting the function list right first is what keeps that mechanical step short.
