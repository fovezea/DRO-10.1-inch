/**
 * @file dro_link_protocol.h
 * @brief Wire protocol shared by the DRO frontend (HMI) and the ESP32 DRO backends.
 *
 * This header is deliberately free of any ESP-IDF / FreeRTOS dependency so that
 * the exact same file can be dropped into the frontend project.  It only needs
 * a C99 compiler and <stdint.h>.
 *
 * ---------------------------------------------------------------------------
 * Frame on the wire
 * ---------------------------------------------------------------------------
 * Every frame is COBS-encoded and terminated by a single 0x00 byte.  COBS
 * guarantees that 0x00 never appears inside an encoded frame, so a receiver can
 * always resynchronise after noise, a reset or interleaved log text.
 *
 * Decoded frame (before COBS):
 *
 *   +---------+--------+-------+--------+-------------+-------------------+
 *   | version | type   | seq   | flags  | payload_len | payload ... | crc16 |
 *   | 1 byte  | 1 byte | 1 byte| 1 byte | 2 bytes LE  |  len bytes  | 2 LE  |
 *   +---------+--------+-------+--------+-------------+-------------------+
 *
 *   crc16 = CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over every byte
 *           from `version` up to and including the last payload byte.
 *
 * All multi-byte fields are little-endian.  Both the ESP32 and the ESP32-P4 are
 * little-endian, so the packed structs below can be used directly.
 *
 * Maximum payload is DRO_LINK_MAX_PAYLOAD (192) bytes, which also keeps a
 * complete frame inside a single ESP-NOW packet (250 byte limit).
 */

#ifndef DRO_LINK_PROTOCOL_H
#define DRO_LINK_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRO_LINK_VERSION       0x01
#define DRO_LINK_MAX_PAYLOAD   192
#define DRO_LINK_MAX_FRAME     (DRO_LINK_MAX_PAYLOAD + 8)
#define DRO_LINK_MAX_WIRE      (DRO_LINK_MAX_FRAME + DRO_LINK_MAX_FRAME / 254 + 2)

#define DRO_LINK_MAX_AXES      5
#define DRO_LINK_NAME_LEN      16

/* ------------------------------------------------------------------------- */
/* Message types                                                             */
/* ------------------------------------------------------------------------- */
typedef enum {
    /* Backend -> frontend */
    DRO_MSG_HELLO        = 0x01, /* dro_hello_t        */
    DRO_MSG_TELEMETRY    = 0x02, /* dro_telemetry_t    */
    DRO_MSG_CONFIG       = 0x03, /* dro_config_t       */
    DRO_MSG_ACK          = 0x04, /* dro_ack_t          */
    DRO_MSG_ERROR        = 0x05, /* dro_error_t        */
    DRO_MSG_DIAG         = 0x06, /* dro_diag_t         */
    DRO_MSG_PONG         = 0x07, /* echoed 8 bytes     */
    DRO_MSG_INDEX_EVENT  = 0x08, /* dro_index_event_t  */

    /* Frontend -> backend */
    DRO_MSG_HELLO_REQ    = 0x10, /* dro_hello_req_t    */
    DRO_MSG_SET_STREAM   = 0x11, /* dro_stream_cfg_t   */
    DRO_MSG_ZERO         = 0x12, /* dro_zero_t         */
    DRO_MSG_PRESET       = 0x13, /* dro_preset_t       */
    DRO_MSG_SET_AXIS     = 0x14, /* dro_set_axis_t     */
    DRO_MSG_GET_CONFIG   = 0x15, /* (empty)            */
    DRO_MSG_PING         = 0x16, /* 0 .. 8 bytes       */
    DRO_MSG_SAVE_CONFIG  = 0x17, /* (empty)            */
    DRO_MSG_RESET_CONFIG = 0x18, /* (empty)            */
    DRO_MSG_REBOOT       = 0x19, /* (empty)            */
    DRO_MSG_SET_LINK     = 0x1A, /* dro_set_link_t     */
    DRO_MSG_GET_DIAG     = 0x1B, /* (empty)            */
} dro_msg_type_t;

/* Frame flags */
#define DRO_FLAG_ACK_REQ    0x01    /* sender wants a DRO_MSG_ACK back       */
#define DRO_FLAG_IS_REPLY   0x02    /* frame is a response to a request      */

/* ------------------------------------------------------------------------- */
/* Telemetry status bits (dro_telemetry_t.status)                          */
/* ------------------------------------------------------------------------- */
#define DRO_ST_UART_SESSION  (1u << 0)  /* a frontend session is active on UART */
#define DRO_ST_ENOW_SESSION  (1u << 1)  /* a frontend session is active on ESP-NOW */
#define DRO_ST_WIFI_READY    (1u << 2)  /* Wi-Fi/ESP-NOW stack is up            */
#define DRO_ST_WIFI_ERROR    (1u << 3)  /* Wi-Fi/ESP-NOW init failed            */
#define DRO_ST_CFG_DIRTY     (1u << 4)  /* unsaved config changes pending       */
#define DRO_ST_SIMULATION    (1u << 5)  /* synthetic counts enabled             */
#define DRO_ST_INDEX_SEEN    (1u << 6)  /* at least one index pulse was captured */
#define DRO_ST_SELFTEST_OK   (1u << 7)  /* protocol self-test passed            */

/* Hello feature bits */
#define DRO_FEAT_QUAD         (1u << 0)
#define DRO_FEAT_INDEX        (1u << 1)
#define DRO_FEAT_ESPNOW       (1u << 2)
#define DRO_FEAT_UART         (1u << 3)
#define DRO_FEAT_SIM          (1u << 4)

/* Link modes (dro_config_t.link_mode) */
#define DRO_LINK_MODE_AUTO    0  /* whichever transport spoke last          */
#define DRO_LINK_MODE_UART    1  /* only accept sessions from UART          */
#define DRO_LINK_MODE_ESPNOW  2  /* only accept sessions from ESP-NOW       */

/* Error codes (dro_error_t.code) */
#define DRO_ERR_NONE          0
#define DRO_ERR_BAD_AXIS      1
#define DRO_ERR_BAD_ARG       2
#define DRO_ERR_BAD_STATE     3
#define DRO_ERR_NOT_SUPPORTED 4

/* ------------------------------------------------------------------------- */
/* Payload structures (all packed, little-endian)                            */
/* ------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  type;
    uint8_t  seq;
    uint8_t  flags;
    uint16_t payload_len;
} dro_frame_hdr_t;

typedef struct {
    uint8_t        version;
    uint8_t        type;
    uint8_t        seq;
    uint8_t        flags;
    uint16_t       payload_len;
    const uint8_t *payload;   /* points into the caller's scratch buffer */
} dro_frame_t;

/** Backend identity, sent on request and as the ESP-NOW discovery beacon. */
typedef struct __attribute__((packed)) {
    uint16_t fw_version;      /* 0x0100 == 1.0                       */
    uint8_t  protocol_version;
    uint8_t  axis_count;
    uint8_t  features;        /* DRO_FEAT_*                          */
    uint8_t  reserved;
    uint8_t  mac[6];          /* own Wi-Fi STA MAC                   */
    char     name[DRO_LINK_NAME_LEN];
    uint32_t uptime_ms;
} dro_hello_t;

/** Frontend announces itself and asks for a stream. */
typedef struct __attribute__((packed)) {
    uint8_t  client_kind;     /* free-form, 1 = DRO HMI              */
    uint8_t  axes_mask;       /* axes the frontend cares about       */
    uint16_t want_hz;         /* requested telemetry rate, 1..200    */
    char     client_name[DRO_LINK_NAME_LEN];
} dro_hello_req_t;

/** The workhorse: absolute raw counts for every axis. */
typedef struct __attribute__((packed)) {
    uint32_t sample_seq;
    uint32_t uptime_ms;
    uint16_t status;          /* DRO_ST_*                            */
    uint8_t  axis_count;
    uint8_t  index_flags;     /* bit i: index pulse since last frame */
    int32_t  pos[DRO_LINK_MAX_AXES];
} dro_telemetry_t;            /* 32 bytes                            */

/** Response to DRO_MSG_GET_CONFIG, also pushed after a config change. */
typedef struct __attribute__((packed)) {
    uint16_t fw_version;
    uint8_t  protocol_version;
    uint8_t  axis_count;
    uint8_t  enabled_mask;
    uint8_t  invert_mask;
    uint8_t  index_mask;
    uint8_t  link_mode;       /* DRO_LINK_MODE_*                     */
    uint8_t  espnow_channel;
    uint8_t  peer_locked;
    uint16_t stream_hz;
    uint16_t reserved;
    uint32_t uart_baud;
    uint8_t  peer_mac[6];
    uint8_t  mac[6];
    char     name[DRO_LINK_NAME_LEN];
} dro_config_t;

typedef struct __attribute__((packed)) {
    uint8_t acked_type;
    uint8_t acked_seq;
    uint8_t status;           /* DRO_ERR_*                           */
    uint8_t reserved;
} dro_ack_t;

typedef struct __attribute__((packed)) {
    uint8_t  code;            /* DRO_ERR_*                           */
    uint8_t  detail;          /* e.g. offending axis                  */
    uint16_t arg;
} dro_error_t;

typedef struct __attribute__((packed)) {
    uint32_t free_heap;
    uint32_t loop_hz;         /* measured axis sampling rate          */
    uint16_t stream_hz;
    uint8_t  axis_count;
    uint8_t  reserved;
    uint32_t edges_per_s[DRO_LINK_MAX_AXES];
    uint32_t overflow_count[DRO_LINK_MAX_AXES];
    uint32_t index_count[DRO_LINK_MAX_AXES];
    uint32_t uart_rx_frames;
    uint32_t uart_tx_frames;
    uint32_t espnow_rx_frames;
    uint32_t espnow_tx_frames;
    uint32_t crc_errors;
    uint32_t dropped_frames;
} dro_diag_t;

/** Sent the moment a scale index (Z) pulse is latched. */
typedef struct __attribute__((packed)) {
    uint8_t  axis;
    uint8_t  reserved[3];
    int32_t  pos;             /* reported position at the pulse       */
    uint32_t t_ms;
} dro_index_event_t;

typedef struct __attribute__((packed)) {
    uint16_t hz;
    uint8_t  axes_mask;
    uint8_t  flags;           /* bit0: emit index events              */
} dro_stream_cfg_t;

typedef struct __attribute__((packed)) {
    uint8_t axis_mask;
    uint8_t reserved[3];
} dro_zero_t;

typedef struct __attribute__((packed)) {
    uint8_t axis;
    uint8_t reserved[3];
    int32_t value;
} dro_preset_t;

typedef struct __attribute__((packed)) {
    uint8_t axis;
    uint8_t enabled;
    uint8_t invert;
    uint8_t index_enable;
} dro_set_axis_t;

typedef struct __attribute__((packed)) {
    uint8_t mode;             /* DRO_LINK_MODE_*                      */
    uint8_t channel;          /* Wi-Fi channel 1..13, 0 = keep        */
    uint8_t lock;             /* 1 = only talk to peer_mac            */
    uint8_t reserved;
    uint8_t peer_mac[6];
} dro_set_link_t;

/* ------------------------------------------------------------------------- */
/* Codec API                                                                 */
/* ------------------------------------------------------------------------- */

/* The wire format depends on these exact sizes.  If a field is added, the
 * static assert fires and forces a conscious protocol-version decision. */
#if defined(__cplusplus)
#define DRO_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define DRO_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

DRO_STATIC_ASSERT(sizeof(dro_frame_hdr_t) == 6, "frame header must be 6 bytes");
DRO_STATIC_ASSERT(sizeof(dro_hello_t) == 32, "HELLO must be 32 bytes");
DRO_STATIC_ASSERT(sizeof(dro_hello_req_t) == 20, "HELLO_REQ must be 20 bytes");
DRO_STATIC_ASSERT(sizeof(dro_telemetry_t) == 32, "TELEMETRY must be 32 bytes");
DRO_STATIC_ASSERT(sizeof(dro_config_t) == 46, "CONFIG must be 46 bytes");
DRO_STATIC_ASSERT(sizeof(dro_ack_t) == 4, "ACK must be 4 bytes");
DRO_STATIC_ASSERT(sizeof(dro_error_t) == 4, "ERROR must be 4 bytes");
DRO_STATIC_ASSERT(sizeof(dro_diag_t) == 96, "DIAG must be 96 bytes");
DRO_STATIC_ASSERT(sizeof(dro_index_event_t) == 12, "INDEX_EVENT must be 12 bytes");
DRO_STATIC_ASSERT(sizeof(dro_stream_cfg_t) == 4, "SET_STREAM must be 4 bytes");
DRO_STATIC_ASSERT(sizeof(dro_zero_t) == 4, "ZERO must be 4 bytes");
DRO_STATIC_ASSERT(sizeof(dro_preset_t) == 8, "PRESET must be 8 bytes");
DRO_STATIC_ASSERT(sizeof(dro_set_axis_t) == 4, "SET_AXIS must be 4 bytes");
DRO_STATIC_ASSERT(sizeof(dro_set_link_t) == 10, "SET_LINK must be 10 bytes");

/** CRC-16/CCITT-FALSE. */
uint16_t dro_crc16(const uint8_t *data, size_t len);

/**
 * COBS-encode `len` bytes into `dst`.
 * @return number of bytes written, or 0 on overflow.
 */
size_t dro_cobs_encode(const uint8_t *src, size_t len, uint8_t *dst, size_t cap);

/**
 * COBS-decode `len` bytes from `src` into `dst`.
 * @return decoded length, or -1 on malformed input / overflow.
 */
int dro_cobs_decode(const uint8_t *src, size_t len, uint8_t *dst, size_t cap);

/**
 * Build a complete wire frame (COBS + 0x00 delimiter).
 * @return wire length, or -1 if it does not fit.
 */
int dro_link_frame_encode(uint8_t type, uint8_t seq, uint8_t flags,
                          const void *payload, uint16_t payload_len,
                          uint8_t *out, size_t out_cap);

/**
 * Decode one complete wire frame (with or without the trailing 0x00).
 * `scratch` must be at least DRO_LINK_MAX_FRAME bytes; `out->payload` points
 * into it and stays valid until the next call using the same buffer.
 *
 * @return 0 on success, negative error code otherwise:
 *         -1 length, -2 COBS, -3 version, -4 payload length, -5 CRC.
 */
int dro_link_frame_decode(const uint8_t *wire, size_t wire_len,
                          uint8_t *scratch, size_t scratch_cap,
                          dro_frame_t *out);

/** Human-readable name for a message type (for logs). */
const char *dro_msg_name(uint8_t type);

#ifdef __cplusplus
}
#endif

#endif /* DRO_LINK_PROTOCOL_H */
