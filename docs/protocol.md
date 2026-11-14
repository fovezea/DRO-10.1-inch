# Frontend-backend protocol

The application protocol is one format carried over up to three transports. Today it exists as
`usb_reports.h` (HID reports); with the stack bus it also has to survive a byte stream (UART) and
a CDC bridge (CH340C plate). This document records what exists, what has to change, and the
framing for the stream transports.

## 1. What exists today

`components/frontend_usb/include/usb_reports.h` and `esp32p4_usb_dro_backend/main/usb_reports.h`
(two copies of the same file -- see §6).

### Input report: backend -> frontend

| Offset | Field | Type | Bytes |
|---|---|---|---|
| 0 | `status_flags` | uint32 | 4 |
| 4 | `spindle_counts` | int32 | 4 |
| 8 | `spindle_rpm` | float | 4 |
| 12 | `axis[0..4]` | int32[5] | 20 |
| **Total** | | | **32** |

### Output report: frontend -> backend

| Offset | Field | Type | Bytes |
|---|---|---|---|
| 0 | `mult_axis[0..4]` | float[5] | 20 |
| 20 | `max_step_hz[0..4]` | uint16[5] | 10 |
| 30 | `acceleration[0..4]` | uint16[5] | 10 |
| 40 | `track_spindle[0..4]` | uint8[5] | 5 |
| 45 | `command_code` | uint8 | 1 |
| **Total** | | | **46** |

Status flags: `STATUS_OVERSPEED_AXIS_0..4`, bits 0-4. Report IDs: 1 = input, 2 = output.

## 2. What has to change

### 2.1 `track_spindle` is too narrow

The field encodes "this axis tracks the spindle" -- a boolean per axis. But the requirement is
**any axis may be the master of any other**, with names ("X", "spindle") being a UI convention
only. A boolean cannot express "Z follows X" or "C follows spindle at 1:2".

Replace with a master index per axis:

```c
int8_t master_idx[NUM_VIRTUAL_AXES];   // 0..4 = axis index, 5 = spindle, -1 (0xFF) = independent
```

That is a strict superset: the lathe case (Z follows spindle) is `master_idx[Z] = 5`. It also
makes the gearing matrix data, not code -- the backend evaluates `slave_target = master_position
* mult_axis[slave] + offset`, with `offset` accumulated on enable so engaging a gear does not
jump the axis.

### 2.2 Stream transports need framing

A HID report is delimited by the transport: one report, one struct, CRC supplied by USB. A UART
or CDC byte stream is not. So the same payload needs a frame:

```
+------+-----+--------+-----+---------+--------+
| 0xA5 | len | seq16  | t16 | payload | crc16  |
+------+-----+--------+-----+---------+--------+
```

- `0xA5` -- start byte, chosen because it is unlikely in a payload of counts and can be escaped
  if a payload ever contains it (COBS or byte stuffing, decided at implementation).
- `len` -- payload length in bytes (36 or 49 below), so a length mismatch is caught before parse
  rather than being parsed as fields.
- `seq16` -- wraps; lets the receiver detect a dropped frame (which for **positions is
  recoverable**, since they are absolute -- the next frame carries the truth).
- `t16` -- sender's tick in milliseconds, low 16 bits; keeps both sides' notion of time aligned
  well enough for RPM and staleness detection.
- `crc16` -- over payload, CRC-16/CCITT. A corrupted position frame that slips through is a wrong
  DRO reading, which is worse than a dropped one.

On HID these bytes are **not** added: the report ID is the frame and USB supplies integrity.

### 2.3 The handshake is missing

Nothing today tells the frontend what the backend can do, so a plain-DRO backend would still show
gearing controls. Add one exchange on connect:

| Field | Type | Meaning |
|---|---|---|
| `magic` | uint8 | 0x5A |
| `proto_version` | uint8 | 2 for this revision |
| `axis_count` | uint8 | 5 for both current plates |
| `caps` | uint8 | bit0 ELS, bit1 spindle RPM, bit2 per-axis acceleration |
| `max_tick_hz` | uint16 | backend's maximum update rate (1000 for the ELS plate) |

The frontend hides ELS controls when bit0 is clear, and refuses to talk to a `proto_version` it
does not know -- better a clear "unsupported backend" than a silently misparsed config.

## 3. Proposed v2 frames

### Input (backend -> frontend), 36-byte payload, 38-byte frame

| Offset | Field | Type |
|---|---|---|
| 0 | `seq` | uint16 |
| 2 | `tick_ms` | uint16 |
| 4 | `status_flags` | uint32 |
| 8 | `spindle_counts` | int32 |
| 12 | `spindle_rpm` | float |
| 16 | `axis[0..4]` | int32[5] |
| 36 | `crc16` | uint16 (stream transports only) |

Payload 36 bytes, 38 with the CRC.

### Output (frontend -> backend), 49-byte payload, 51-byte frame

| Offset | Field | Type |
|---|---|---|
| 0 | `seq` | uint16 |
| 2 | `command_code` | uint8 |
| 3 | `axis_mask` | uint8 (which axes this config touches) |
| 4 | `mult_axis[0..4]` | float[5] |
| 24 | `max_step_hz[0..4]` | uint16[5] |
| 34 | `acceleration[0..4]` | uint16[5] |
| 44 | `master_idx[0..4]` | int8[5] |
| 49 | `crc16` | uint16 (stream transports only) |

Payload 49 bytes, 51 with the CRC.

Note the byte order is little-endian on both sides (RISC-V and Xtensa are LE), and the structs stay
packed -- but the frame is defined by offsets, not by `sizeof`, so a compiler change cannot
silently shift a field.

## 4. Transport mapping

| Transport | Framing | Integrity | Rate |
|---|---|---|---|
| USB HID (existing backends) | report ID | USB CRC | up to 1 kHz |
| UART on the stack bus | `0xA5 len seq t16 payload crc16` | CRC-16 | 250 Hz-1 kHz |
| CDC via CH340C (WROOM plate) | same frame | CRC-16 | 200 Hz |

The payload is identical in all three, so the parser is written once and the transport supplies
or strips the frame.

## 5. Bandwidth

Input is a 38-byte frame: **9.5 kB/s at 250 Hz**, 38 kB/s at 1 kHz. UART at 1 Mbaud (10 bits per
byte) carries 100 kB/s, so 250 Hz uses 10 % of the link and 1 kHz uses 40 %. Output frames are
sent only on change (config updates), not periodically.

## 6. One source of truth

`usb_reports.h` currently exists **twice** -- once in the frontend's `components/frontend_usb/`
and once in `esp32p4_usb_dro_backend/main/`. They are identical today, which is exactly the
condition under which two copies silently diverge. Before v2 lands they should become one copy,
either as a small shared component consumed by both, or generated into both trees at build time.

This matters more for v2 than it did for v1: a field offset that disagrees between the two sides
does not fail to compile, it misreads positions.
