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

## 2. The J2 header, as drawn

`J2` is a 2x17 header at 2.54 mm and it alone carries GPIO24-53 -- enough for the entire bus on
one connector, which is why the allocation below uses J2 only and leaves J4 for the C5 side.

Left column is odd pins, right column even, both descending:

| Pin | Signal | Pin | Signal |
|---|---|---|---|
| 1 | VIN_5V | 2 | VIN_5V |
| 3 | GND | 4 | GND |
| 5 | GPIO52 | 6 | GPIO53 |
| 7 | GPIO50 | 8 | GPIO51 |
| 9 | GPIO48 | 10 | GPIO49 |
| 11 | GPIO46 | 12 | GPIO47 |
| 13 | GPIO44 | 14 | GPIO45 |
| 15 | GPIO42 | 16 | GPIO43 |
| 17 | GPIO40 | 18 | GPIO41 |
| 19 | GPIO38 | 20 | GPIO39 |
| 21 | GPIO36 | 22 | GPIO37 |
| 23 | GPIO34 | 24 | GPIO35 |
| 25 | GPIO32 | 26 | GPIO33 |
| 27 | GPIO30 | 28 | GPIO31 |
| 29 | GPIO28 | 30 | GPIO29 |
| 31 | GPIO26 | 32 | GPIO27 |
| 33 | GPIO24 | 34 | GPIO25 |

Both headers use the same pattern: **pins 1-2 = 5 V, pins 3-4 = GND**. So four 5 V and four GND
pins are available across the pair. This was verified against the schematic, not read off the
vendor's drawing -- that drawing places the `GND` / `VIN_5V` labels above the connector with
crossing wires, which reads as 1 = GND and 3 = 5 V, and is wrong. Read the net at the pin, never
the nearest label. At ~1.2 A total that is ~0.6 A per pin, well inside a
2.54 mm header pin's rating -- and using both headers' 5 V pins gives the four the power section
assumed.

**Not usable by a plate**, on this header, because the frontend already drives them:

| GPIO | J2 pins | Frontend use |
|---|---|---|
| 39-44 | 20, 17, 18, 15, 16, 13 | TF card (SD1 D0-D3, CLK, CMD) |
| 45 | 14 | TF card detect |
| 53 | 6 | NS4150B power-amp enable |

**Free and usable: GPIO24-38 and GPIO46-52** -- twenty-two pins on J2 (33, 31, 29, 27, 25, 23,
21, 19, 22, 24, 26, 28, 30, 32, 34 and 11, 12, 9, 7, 8, 5, 10). The bus takes thirteen of them
and leaves nine spare, which is comfortable headroom for a future plate.

## 3. The J4 header, as drawn

J4 covers the low GPIOs, GPIO54, the nine co-processor IOs and a 3.3 V rail. The bus does not
use it, but a future radio plate will.

| Pin | Signal | Pin | Signal |
|---|---|---|---|
| 1 | VIN_5V | 2 | VIN_5V |
| 3 | GND | 4 | GND |
| 5 | +3V3 | 6 | +3V3 |
| 7 | GPIO0 | 8 | GPIO54 |
| 9 | C6_IO15 / C5_IO28 | 10 | C6_IO14 / C5_IO27 |
| 11 | C6_IO13 / C5_IO26 | 12 | C6_IO9 / C5_IO25 |
| 13 | C6_IO8 / C5_IO24 | 14 | C6_IO7 / C5_IO23 |
| 15 | C6_IO6 / C5_IO6 | 16 | C6_IO5 / C5_IO5 |
| 17 | C6_IO4 / C5_IO4 | 18 | GPIO1 |
| 19 | GPIO2 | 20 | GPIO3 |
| 21 | GPIO4 | 22 | GPIO5 |
| 23 | GPIO6 | 24 | GPIO7 |
| 25 | GPIO8 | 26 | GPIO9 |
| 27 | GPIO10 | 28 | GPIO11 |
| 29 | GPIO12 | 30 | GPIO13 |
| 31 | GPIO20 | 32 | GPIO21 |
| 33 | GPIO22 | 34 | GPIO23 |

### What a plate may use here

| Group | Usable? |
|---|---|
| GPIO1, 20-23 | yes -- free on this frontend |
| GPIO2-5 | yes, but they are the JTAG pads (MTCK/MTDI/MTMS/MTDO) |
| GPIO0 | no -- boot strapping |
| GPIO6 | no -- C5 handshake |
| GPIO7, 8 | no -- touch/codec I2C |
| GPIO9-13 | no -- audio I2S |
| GPIO54 | no -- C5 chip enable |
| **C5_IO4-IO6, C5_IO23-IO28** | **yes** -- nine co-processor IOs, available to a radio or bridge plate |

### The 3.3 V pins

J4 brings out +3V3 on pins 5 and 6. Small loads -- the five LS7366Rs draw ~10 mA -- could run from
it, but the **backend P4 module must not**: it wants a few hundred milliamps at 3.3 V, which the
frontend's own regulator is not there to supply. The plate should generate its own 3.3 V from the
5 V rail and treat J4's 3V3 as optional, not as the supply.

## 4. Proposed allocation

Two headers, two jobs -- which is what makes the plates interchangeable. The frontend-backend link
goes on **J4**, the backend-counter SPI on **J2**, because the PoE adapter needs J2's high GPIOs
(section 5).

### J4: frontend <-> backend link

| Signal | GPIO | J4 pin | Direction |
|---|---|---|---|
| `BB_UART_TX` | 20 | 31 | frontend -> backend |
| `BB_UART_RX` | 21 | 32 | backend -> frontend |
| `BB_IRQ` | 22 | 33 | backend -> frontend, optional |
| `BB_RESET` | 23 | 34 | frontend -> backend |
| `BB_BOOT` | 1 | 18 | frontend -> backend, optional |

These five are the only J4 GPIOs that are both free and clear of the PoE reservation. If a sixth is
ever needed, GPIO3-5 (J4 pins 20-22) are available at the cost of JTAG.

This is also the **entire** interconnection between two stacked dev boards: 5 V, GND and the UART
pair are the minimum, so a four-wire link (5 V, GND, TX, RX) is enough to bring a backend up, with
the remaining three for recovery and events.

### J2: backend <-> quadrature plate

| Signal | GPIO | J2 pin | Direction |
|---|---|---|---|
| `SPI_SCK` | 32 | 25 | backend -> counters |
| `SPI_MOSI` | 33 | 26 | backend -> counters |
| `SPI_MISO` | 30 | 27 | counters -> backend |
| `CS0` | 31 | 28 | backend -> counter 0 |
| `CS1` | 28 | 29 | backend -> counter 1 |
| `CS2` | 29 | 30 | backend -> counter 2 |
| `CS3` | 26 | 31 | backend -> counter 3 |
| `CS4` | 27 | 32 | backend -> counter 4 |
| `CNT_DRDY` | 24 | 33 | counters -> backend, optional |
| `CNT_RESET` | 25 | 34 | backend -> counters, optional |
| `STEP0` / `DIR0` | 34 / 35 | 23 / 24 | backend -> driver |
| `STEP1` / `DIR1` | 36 / 37 | 21 / 22 | backend -> driver |
| `STEP2` / `DIR2` | 38 / 46 | 19 / 11 | backend -> driver |
| `ALM0` / `ALM1` / `ALM2` | 47 / 48 / 49 | 12 / 9 / 10 | **drivers -> backend (fault)** |
| `ENA` (all drivers) | 50 | 7 | backend -> drivers, shared |
| spare | 51, 52 | 8, 5 | |

The alarm inputs are what makes a stalled stepper safe: the drivers report faults, and the backend can
stop motion instead of continuing to command a motor that cannot move. ENA is deliberately shared --
one line disables all three drivers, which is the right response to a fault and keeps a GPIO spare.

Note this uses GPIO33, which the PoE adapter wants for TX_EN -- so the quadrature plate and the PoE
plate are mutually exclusive, which they already are by geometry. If a LAN-connected ELS is ever
wanted, `SPI_MOSI` must move off GPIO33 first.

**Pull-ups on CS0-CS4** (10 k to 3V3) on the quadrature plate: while the backend is resetting or
absent, a floating chip select can clock garbage into a counter.

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

## 5. PoE Ethernet adapter: reserved pins

The optional 10/100BASE-TX + PoE adapter plugs onto the dev board headers and drives RMII. Its
pins are therefore unavailable to any plate when it is fitted:

| Signal | GPIO | Signal | GPIO | Signal | GPIO |
|---|---|---|---|---|---|
| RXD0 | 52 | TXD0 | 34 | 50M_CLK | 50 |
| RXD1 | 47 | TXD1 | 35 | MDC | 53 |
| CRS_DV | 51 | TX_EN | 33 | MDIO | 49 |
| | | | | PHY_RESET | 36 |

Two consequences:

1. **GPIO33, 34, 35, 36, 47, 49-52** are spoken for. That is why the frontend-backend link moved to
   J4: leaving it on J2 would have made the UART collide with PHY_RESET (36) and TXD0/TXD1 (34/35),
   so a LAN-connected DRO with a backend would have been impossible.
2. **GPIO53 (MDC) is the dev board's audio power-amp enable**, so the PoE adapter and the speaker
   are mutually exclusive. Nothing to do about it, but worth knowing before wondering why audio
   dies when the adapter is fitted.

The adapter's **2x13P** connector mates with only part of the 2x17 header -- it overlaps a subset of
the positions rather than spanning the whole edge -- so the RMII GPIOs are reached through that
overlap and the rest of the header stays available. Nothing unusual, and it means the adapter can be
fitted without removing the header's other functions.

What matters for plates is unchanged: **GPIO33, 34, 35, 36, 47, 49, 50, 51, 52 and 53** are not
available while the adapter is fitted.

## 6. Frontend-backend transport

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

## 7. Rules a plate must follow

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

## 8. Verification status

Confirmed against the schematic, pin by pin:

- **J2 power**: pins 1-2 = 5 V, 3-4 = GND.
- **J4 power**: same pattern.
- **GPIO mapping**: all assignments in section 4 match J2.
- **GPIO46-52**: free and unconnected on the dev board, so the spares are real.

Nothing about the bus is outstanding. The next artefact is the plate schematic itself: M5Stamp
socket, five LS7366R behind the existing 74LVC245 front end, three step/dir outputs, and a single
5 V input with its own 3.3 V regulator.
