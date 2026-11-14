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

## 4. Frontend-backend transport

**UART replaces USB.** This is the change the stack enables.

| | USB HID (today) | UART on the bus (stacked) |
|---|---|---|
| Frontend drivers | USB host + HID class | UART only |
| Backend drivers | USB device + HID class | UART only |
| Cable | required | none |
| Latency | USB frames, ~1 ms | bounded by baud and framing |
| Bandwidth at 1 Mbaud | plenty | ~100 kB/s -- far above the ~10 kB/s telemetry needs |

Keep the **application framing** identical to what the USB path uses (sequence, tick, 5x 32-bit
positions, status, CRC) and keep the **capability handshake**: a plain-DRO backend reports no
ELS, and the UI hides gearing. That way the frontend has one protocol and three possible
transports -- HID (existing boards over USB), UART-on-stack, and CDC (the CH340C plate) -- and
none of the application code cares which is in use.

Both UARTs are free (UART0 is the console), so this costs no dedicated peripheral, and DMA on
the P4 keeps the ISR load trivial at these rates.

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
