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
| 1 | GND | 2 | VIN_5V |
| 3 | VIN_5V | 4 | GND |
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

Both headers follow the same pattern -- **1 = GND, 2 = 5 V, 3 = 5 V, 4 = GND** -- confirmed
from the J4 drawing, which labels its left pair the same way. So four 5 V and four GND pins are
available across the pair. At ~1.2 A total that is ~0.6 A per pin, well inside a
2.54 mm header pin's rating -- and using both headers' 5 V pins gives the four the power section
assumed.

**Not usable by a plate**, on this header, because the frontend already drives them:

| GPIO | J2 pins | Frontend use |
|---|---|---|
| 39-44 | 20, 17, 18, 15, 16, 13 | TF card (SD1 D0-D3, CLK, CMD) |
| 45 | 14 | TF card detect |
| 53 | 6 | NS4150B power-amp enable |
| 46, 47 | 11, 12 | unused by the frontend, but confirmed occupied/unknown -- verify before claiming |

**Free and usable: 24-38** (J2 pins 33, 31, 29, 27, 25, 23, 21, 19, 22, 24, 26, 28, 30, 32,
34) -- fifteen pins, exactly what the bus needs -- plus **48-52** (pins 9, 7, 8, 5, 10) as spares
for later plates.

## 3. The J4 header, as drawn

J4 covers the low GPIOs, GPIO54, the nine co-processor IOs and a 3.3 V rail. The bus does not
use it, but a future radio plate will.

| Pin | Signal | Pin | Signal |
|---|---|---|---|
| 1 | GND | 2 | VIN_5V |
| 3 | VIN_5V | 4 | GND |
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
| **C5_IO4-IO6, C5_IO23-IO28** | **yes, and interesting** -- nine co-processor IOs, which is what a Zigbee/802.15.4 or radio plate would attach to |

### The 3.3 V pins

J4 brings out +3V3 on pins 5 and 6. Small loads -- the five LS7366Rs draw ~10 mA -- could run from
it, but the **backend P4 module must not**: it wants a few hundred milliamps at 3.3 V, which the
frontend's own regulator is not there to supply. The plate should generate its own 3.3 V from the
5 V rail and treat J4's 3V3 as optional, not as the supply.

## 4. Proposed allocation

All on J2, so the plate mates with one header; J4 stays untouched for the C5 side.

| Signal | GPIO | J2 pin | Direction |
|---|---|---|---|
| `BB_UART_TX` | 38 | 19 | frontend -> backend |
| `BB_UART_RX` | 36 | 21 | backend -> frontend |
| `BB_IRQ` | 37 | 22 | backend -> frontend, optional |
| `BB_RESET` | 34 | 23 | frontend -> backend |
| `BB_BOOT` | 35 | 24 | frontend -> backend, optional |
| `SPI_SCK` | 32 | 25 | backend -> counters |
| `SPI_MOSI` | 33 | 26 | backend -> counters |
| `SPI_MISO` | 30 | 27 | counters -> backend |
| `CS0` | 31 | 28 | backend -> counter 0 |
| `CS1` | 28 | 29 | backend -> counter 1 |
| `CS2` | 29 | 30 | backend -> counter 2 |
| `CS3` | 26 | 31 | backend -> counter 3 |
| `CS4` | 27 | 32 | backend -> counter 4 |
| spare | 24 | 33 | |
| spare | 25 | 34 | |
| spare | 48-52 | 9, 7, 8, 5, 10 | for a future plate |

The interleaving is deliberate: `CS0..CS4` land on adjacent pins (28-32) so the ribbon to the
counter chips runs as one bundle, and the UART/RESET group sits next to them at 19-24.

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

## 5. Frontend-backend transport

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

## 6. Rules a plate must follow

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

## 7. What still needs verifying

The mapping came from the vendor's own header drawings, so the plate footprint can be laid out
against it. The power pins are settled (1 = GND, 2 = 5 V, 3 = 5 V, 4 = GND on both headers). Two
small things remain, both checkable on the bench in minutes:

1. **GPIO46/47 (J2 pins 11, 12)**: not used by this frontend, but not confirmed free either. They
   are held in reserve rather than assigned.
2. **GPIO48-52 (J2 pins 9, 7, 8, 5, 10)**: treated as spare. Confirm they are truly unconnected
   on the dev board before relying on them for a future plate.

Nothing else is blocking: module, counters, bus functions, power topology and transport are all
decided and recorded.
