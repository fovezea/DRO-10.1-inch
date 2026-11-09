# DRO link protocol v1

Wire protocol between a DRO **frontend** (HMI) and a DRO **backend** (scale
reader, and in the future the ELS controller).

The same protocol runs over both transports:

| Transport | Framing |
|---|---|
| UART (UART0 or UART2) | byte stream, `0x00`-delimited COBS frames |
| ESP-NOW | one frame per packet (also COBS + `0x00`, so the codec is shared) |

The C implementation is `components/dro_link/` and is deliberately free of
ESP-IDF dependencies so the frontend can compile the exact same files.

---

## 1. Design goals

1. **Resynchronise from anything.** A reset, a boot log, a hot-plug or noise
   must never wedge the receiver. COBS + a `0x00` delimiter gives that for free.
2. **Detect corruption.** CRC-16 over the whole frame, not just the payload.
3. **One active peer.** A desktop DRO has one display; multiple senders would
   fight over the stream. The backend therefore streams only to the session
   that last identified itself.
4. **Small.** Telemetry must fit in one ESP-NOW packet (250 bytes) and be cheap
   at 200 Hz. It is 32 bytes of payload.
5. **Extensible without breaking.** A version byte, a length field, and a rule
   that new fields are appended.

---

## 2. Frame format

```
 decoded frame (before COBS)
 +---------+--------+-------+--------+-------------+---------------+---------+
 | version |  type  |  seq  | flags  | payload_len |   payload     |  crc16  |
 | 1 byte  | 1 byte | 1 byte| 1 byte |  2 bytes LE |  0..192 bytes | 2 bytes |
 +---------+--------+-------+--------+-------------+---------------+---------+
   ^                                                                   ^
   +------------------------- CRC-16/CCITT-FALSE ----------------------+
        computed over version .. last payload byte (6 + payload_len bytes)
```

* `version` = `0x01`.
* `type` — see the message table.
* `seq` — sender's counter, 0..255, wrapping. Used for ACK correlation and
  duplicate detection. The backend increments it per transmitted frame.
* `flags` — bit 0 `ACK_REQ`, bit 1 `IS_REPLY`.
* `payload_len` — little-endian.
* `crc16` — CRC-16/CCITT-FALSE: polynomial `0x1021`, init `0xFFFF`, no
  reflection, no final XOR. Check value for `"123456789"` is `0x29B1`.

Then the whole decoded frame is **COBS encoded** and followed by one `0x00`
byte. COBS removes every `0x00` from the encoded body, so the delimiter is
unambiguous. The maximum encoded frame is 202 bytes.

### Why COBS and not a magic header?

A magic header (`0xAA 0x55`) only tells you where a frame *might* start; after
noise you have to search for it again and you can still lock onto a false
positive inside binary data. `0x00` gives an absolute boundary and the CRC
rejects anything that slipped through.

### Receiver behaviour

1. Accumulate bytes until `0x00`.
2. COBS-decode. If the body is malformed, drop it and wait for the next `0x00`.
3. Check `len >= 8`, `version == 1`, `payload_len <= 192` and
   `len == payload_len + 8`.
4. Check CRC. On failure drop silently and count the error.
5. Dispatch on `type`.

The UART parser in this firmware keeps at most one frame buffer and drops
overlong input until the next delimiter.

---

## 3. Session model

The backend is a "dumb pipe" but not a broadcast beacon: it only streams to a
frontend that has introduced itself.

```
frontend                                    backend
   |  HELLO_REQ (want_hz, axes)  ------------->|   session := {transport, mac}
   |<-----------------------------  HELLO      |   reply
   |<-----------------------------  TELEMETRY  |   starts at want_hz
   |  PING / HELLO_REQ / command  ------------->|   keeps session alive
   |<-----------------------------  TELEMETRY  |
   ... 3 s of silence ...
   |                                           |   session expires
   |                                           |   ESP-NOW HELLO beacon, 1 Hz
```

Rules:

* **Any valid frame** from the session peer refreshes `last_rx`. There is no
  separate keepalive message; `PING` with an empty payload is the cheapest one.
* **Timeout is 3 s** (`CONFIG_DRO_SESSION_TIMEOUT_MS`). When it expires the
  session is cleared and streaming stops. A dead frontend is never spammed.
* **`HELLO_REQ` replaces the session**, even if it comes from another peer.
  This is the only message allowed to steal the session. A `PING` from a
  stranger is answered but does not redirect the stream.
* While no session exists, and if ESP-NOW is up, the backend broadcasts a
  `HELLO` on `FF:FF:FF:FF:FF:FF` once per second, so a frontend can discover
  the node without knowing its MAC.
* The frontend should send `HELLO_REQ` (or `PING`) at **1 Hz** while it wants
  data, and again immediately after any reset.

The backend sends telemetry at the session's `want_hz` (1..200, default 50).
`SET_STREAM` changes it without re-opening the session.

---

## 4. Messages

### 4.1 Backend → frontend

| Type | Name | Payload | Size |
|---|---|---|---|
| `0x01` | `HELLO` | `dro_hello_t` | 32 |
| `0x02` | `TELEMETRY` | `dro_telemetry_t` | 32 |
| `0x03` | `CONFIG` | `dro_config_t` | 46 |
| `0x04` | `ACK` | `dro_ack_t` | 4 |
| `0x05` | `ERROR` | `dro_error_t` | 4 |
| `0x06` | `DIAG` | `dro_diag_t` | 96 |
| `0x07` | `PONG` | echoed bytes | 0..8 |
| `0x08` | `INDEX_EVENT` | `dro_index_event_t` | 12 |

### 4.2 Frontend → backend

| Type | Name | Payload | Size |
|---|---|---|---|
| `0x10` | `HELLO_REQ` | `dro_hello_req_t` | 20 |
| `0x11` | `SET_STREAM` | `dro_stream_cfg_t` | 4 |
| `0x12` | `ZERO` | `dro_zero_t` | 4 |
| `0x13` | `PRESET` | `dro_preset_t` | 8 |
| `0x14` | `SET_AXIS` | `dro_set_axis_t` | 4 |
| `0x15` | `GET_CONFIG` | — | 0 |
| `0x16` | `PING` | arbitrary 0..8 bytes | 0..8 |
| `0x17` | `SAVE_CONFIG` | — | 0 |
| `0x18` | `RESET_CONFIG` | — | 0 |
| `0x19` | `REBOOT` | — | 0 |
| `0x1A` | `SET_LINK` | `dro_set_link_t` | 10 |
| `0x1B` | `GET_DIAG` | — | 0 |

Commands are acknowledged with `ACK` (`acked_type`, `acked_seq`, `status`).
`GET_CONFIG`, `GET_DIAG`, `PING` answer with their own reply type instead.
`HELLO_REQ` answers with `HELLO`.

---

## 5. Payload layouts

All fields are little-endian. C structs are `__attribute__((packed))`.

### 5.1 `HELLO` — 32 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u16 | `fw_version` (`0x0100` = 1.0) |
| 2 | u8 | `protocol_version` (1) |
| 3 | u8 | `axis_count` (5) |
| 4 | u8 | `features` — bit0 quad, bit1 index, bit2 espnow, bit3 uart, bit4 sim |
| 5 | u8 | reserved |
| 6 | u8[6] | `mac` (backend station MAC) |
| 12 | char[16] | `name`, NUL padded |
| 28 | u32 | `uptime_ms` |

### 5.2 `TELEMETRY` — 32 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u32 | `sample_seq` — increments every 1 ms sample |
| 4 | u32 | `uptime_ms` |
| 8 | u16 | `status` — see below |
| 10 | u8 | `axis_count` (5) |
| 11 | u8 | `index_flags` — bit i: index pulse since the previous telemetry |
| 12 | i32[5] | `pos` — reported counts, X Y Z W V |

`status` bits:

| Bit | Name | Meaning |
|---|---|---|
| 0 | `UART_SESSION` | active session is on the wired link |
| 1 | `ENOW_SESSION` | active session is on ESP-NOW |
| 2 | `WIFI_READY` | Wi-Fi/ESP-NOW stack is up |
| 3 | `WIFI_ERROR` | Wi-Fi/ESP-NOW failed to start |
| 4 | `CFG_DIRTY` | settings changed but not yet written to NVS |
| 5 | `SIMULATION` | synthetic counts are enabled |
| 6 | `INDEX_SEEN` | at least one Z pulse has been captured |
| 7 | `SELFTEST_OK` | the codec self-test passed |

`pos` is a signed 32-bit count that wraps. Internally the backend keeps 64-bit
counts; the wrap is intentional so the wire stays small. A 5 µm scale would
have to travel 10.7 km to wrap.

### 5.3 `HELLO_REQ` — 20 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u8 | `client_kind` (1 = DRO HMI, free-form) |
| 1 | u8 | `axes_mask` — axes the frontend wants (0 = all enabled) |
| 2 | u16 | `want_hz` — 1..200 (0 = backend default) |
| 4 | char[16] | `client_name` |

### 5.4 `CONFIG` — 46 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u16 | `fw_version` |
| 2 | u8 | `protocol_version` |
| 3 | u8 | `axis_count` |
| 4 | u8 | `enabled_mask` |
| 5 | u8 | `invert_mask` |
| 6 | u8 | `index_mask` |
| 7 | u8 | `link_mode` (0 auto, 1 uart, 2 espnow) |
| 8 | u8 | `espnow_channel` |
| 9 | u8 | `peer_locked` |
| 10 | u16 | `stream_hz` |
| 12 | u16 | reserved |
| 14 | u32 | `uart_baud` |
| 18 | u8[6] | `peer_mac` |
| 24 | u8[6] | `mac` |
| 30 | char[16] | `name` |

### 5.5 `ACK` — 4 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u8 | `acked_type` |
| 1 | u8 | `acked_seq` |
| 2 | u8 | `status` (0 = ok) |
| 3 | u8 | reserved |

### 5.6 `ERROR` — 4 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u8 | `code` |
| 1 | u8 | `detail` (e.g. offending axis) |
| 2 | u16 | `arg` |

Error codes: 0 none, 1 bad axis, 2 bad argument, 3 bad state, 4 not supported.

### 5.7 `DIAG` — 96 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u32 | `free_heap` |
| 4 | u32 | `loop_hz` |
| 8 | u16 | `stream_hz` |
| 10 | u8 | `axis_count` |
| 11 | u8 | reserved |
| 12 | u32[5] | `edges_per_s` — counts per second (4× edges) |
| 32 | u32[5] | `overflow_count` |
| 52 | u32[5] | `index_count` |
| 72 | u32 | `uart_rx_frames` |
| 76 | u32 | `uart_tx_frames` |
| 80 | u32 | `espnow_rx_frames` |
| 84 | u32 | `espnow_tx_frames` |
| 88 | u32 | `crc_errors` |
| 92 | u32 | `dropped_frames` |

### 5.8 `INDEX_EVENT` — 12 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u8 | `axis` |
| 1 | u8[3] | reserved |
| 4 | i32 | `pos` — reported position at the pulse |
| 8 | u32 | `t_ms` |

Sent as soon as a Z pulse is latched, independent of the telemetry rate.

### 5.9 `SET_STREAM` — 4 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u16 | `hz` |
| 2 | u8 | `axes_mask` |
| 3 | u8 | `flags` — bit0 emit `INDEX_EVENT`s |

### 5.10 `ZERO` — 4 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u8 | `axis_mask` |
| 1 | u8[3] | reserved |

Sets the origin of every selected axis to the current raw count.

### 5.11 `PRESET` — 8 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u8 | `axis` |
| 1 | u8[3] | reserved |
| 4 | i32 | `value` |

Sets the reported position of one axis, preserving the physical count.

### 5.12 `SET_AXIS` — 4 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u8 | `axis` |
| 1 | u8 | `enabled` |
| 2 | u8 | `invert` |
| 3 | u8 | `index_enable` |

### 5.13 `SET_LINK` — 10 bytes

| Off | Type | Field |
|---|---|---|
| 0 | u8 | `mode` (0 auto, 1 uart, 2 espnow) |
| 1 | u8 | `channel` (1..13, 0 = keep) |
| 2 | u8 | `lock` (1 = only talk to `peer_mac`) |
| 3 | u8 | reserved |
| 4 | u8[6] | `peer_mac` |

Changing the channel drops the current ESP-NOW peer; the frontend must follow
on the new channel and send `HELLO_REQ` again.

---

## 6. Semantics worth copying into the frontend

* **Zero is not a hardware reset.** It records an origin; the raw counter keeps
  running. Re-zeroing an axis never loses counts.
* **Invert preserves the displayed value.** The backend flips the sign *and*
  adjusts the origin, so the number on screen does not jump when the direction
  is changed.
* **Disabling an axis does not stop the counter.** It only clears the enabled
  bit, so a re-enabled axis still has its true position.
* **The backend reports counts, never distances.** Steps/mm, scale resolution
  and unit conversion are frontend state (see the project's calibration screen).
* **`sample_seq` is not the telemetry rate.** It advances at 1 kHz; telemetry is
  a snapshot. Use `uptime_ms` for rate/latency measurements.

---

## 7. Frontend integration (C)

Copy `components/dro_link/` into the frontend project (it only needs
`<<stdint.h>>` and `<string.h>`), then:

```c
#include "dro_link_protocol.h"

static uint8_t s_scratch[DRO_LINK_MAX_FRAME];

static void handle_frame(const dro_frame_t *f)
{
    switch (f->type) {
    case DRO_MSG_TELEMETRY: {
        dro_telemetry_t t;
        if (f->payload_len < sizeof(t)) return;   /* forward compatible */
        memcpy(&t, f->payload, sizeof(t));
        for (int i = 0; i < 5; i++) {
            dro_set_raw_counts(i, t.pos[i]);      /* existing DRO core call */
        }
        break;
    }
    case DRO_MSG_INDEX_EVENT: {
        dro_index_event_t ev;
        memcpy(&ev, f->payload, sizeof(ev));
        /* frontend may use ev.pos as a reference mark */
        break;
    }
    }
}

/* Byte stream (UART or USB CDC): delimiter-driven. */
static uint8_t s_frame[DRO_LINK_MAX_WIRE];
static size_t  s_len;

void dro_link_feed(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        if (b == 0x00) {
            dro_frame_t f;
            if (s_len && dro_link_frame_decode(s_frame, s_len, s_scratch,
                                               sizeof(s_scratch), &f) == 0) {
                handle_frame(&f);
            }
            s_len = 0;
        } else if (s_len < sizeof(s_frame)) {
            s_frame[s_len++] = b;
        } else {
            s_len = 0;   /* overlong: resync on the next delimiter */
        }
    }
}
```

Sending a request:

```c
uint8_t wire[DRO_LINK_MAX_WIRE];
int n = dro_link_frame_encode(DRO_MSG_HELLO_REQ, my_seq++, 0,
                              &req, sizeof(req), wire, sizeof(wire));
uart_write_bytes(UART_NUM_1, wire, n);   /* wire already ends with 0x00 */
```

For ESP-NOW, send `wire` as the packet body with `esp_now_send()`.

The existing P4 firmware already has `hid_input_report_t.axis[5]`, so the
telemetry mapping is one line per axis; the USB HID transport can be kept as a
separate backend for the P4 ELS board.

---

## 8. Timing and bandwidth

| Rate | Payload | Wire bytes | Throughput |
|---|---|---|---|
| 50 Hz | 32 | 42 | 2.1 kB/s |
| 100 Hz | 32 | 42 | 4.2 kB/s |
| 200 Hz | 32 | 42 | 8.4 kB/s |

460800 baud carries ~46 kB/s, so the wired link has ~20× headroom. ESP-NOW
handles 100 Hz comfortably; above that, prefer unicast and a dedicated channel.

Telemetry latency is bounded by the link task's 2 ms poll plus the transport.
The axis itself is sampled every 1 ms, so a frontend polling at 50 Hz sees at
most 1 ms of sampling age and up to 20 ms of reporting age.

---

## 9. Versioning rules

* Unknown `type` → reply `ERROR` with code 4 (not supported); never crash.
* New optional fields are **appended** to a payload; `payload_len` grows.
  Receivers must accept `payload_len >= sizeof(my_struct)`.
* Removing or reordering a field requires `version = 2`, and the backend should
  reject a mismatched version instead of misparsing it.
* `HELLO` carries both `fw_version` and `protocol_version`; a frontend should
  refuse to stream if `protocol_version` is not what it implements.
