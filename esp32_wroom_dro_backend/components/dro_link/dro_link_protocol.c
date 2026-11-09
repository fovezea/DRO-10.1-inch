/**
 * @file dro_link_protocol.c
 * @brief Portable implementation of the DRO link framing (COBS + CRC16).
 *
 * No ESP-IDF headers are used here on purpose: the frontend project compiles
 * this same file.
 */

#include "dro_link_protocol.h"

#include <string.h>

/* ------------------------------------------------------------------------- */
/* CRC-16/CCITT-FALSE                                                        */
/* ------------------------------------------------------------------------- */
uint16_t dro_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;

    if (data == NULL) {
        return crc;
    }

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/* ------------------------------------------------------------------------- */
/* COBS                                                                      */
/* ------------------------------------------------------------------------- */
size_t dro_cobs_encode(const uint8_t *src, size_t len, uint8_t *dst, size_t cap)
{
    /* Worst case: one code byte per 254 input bytes, plus the leading code. */
    size_t worst = len + (len / 254) + 1;
    if (dst == NULL || (src == NULL && len != 0) || cap < worst) {
        return 0;
    }

    size_t read = 0;
    size_t write = 1;   /* dst[0] reserved for the first code byte */
    size_t code_idx = 0;
    uint8_t code = 1;

    while (read < len) {
        if (src[read] == 0x00) {
            dst[code_idx] = code;
            code_idx = write++;
            code = 1;
            read++;
        } else {
            dst[write++] = src[read++];
            code++;
            if (code == 0xFF) {
                dst[code_idx] = code;
                code_idx = write++;
                code = 1;
            }
        }
    }

    dst[code_idx] = code;
    return write;
}

int dro_cobs_decode(const uint8_t *src, size_t len, uint8_t *dst, size_t cap)
{
    size_t read = 0;
    size_t write = 0;

    if (src == NULL || dst == NULL) {
        return -1;
    }

    while (read < len) {
        uint8_t code = src[read++];
        if (code == 0x00) {
            return -1;
        }
        for (uint8_t i = 1; i < code; i++) {
            if (read >= len || write >= cap) {
                return -1;
            }
            dst[write++] = src[read++];
        }
        /* A 0xFF code means "254 literal bytes, no implicit zero". */
        if (code < 0xFF && read < len) {
            if (write >= cap) {
                return -1;
            }
            dst[write++] = 0x00;
        }
    }

    return (int)write;
}

/* ------------------------------------------------------------------------- */
/* Frame codec                                                               */
/* ------------------------------------------------------------------------- */
int dro_link_frame_encode(uint8_t type, uint8_t seq, uint8_t flags,
                          const void *payload, uint16_t payload_len,
                          uint8_t *out, size_t out_cap)
{
    uint8_t raw[DRO_LINK_MAX_FRAME];

    if (payload_len > DRO_LINK_MAX_PAYLOAD) {
        return -1;
    }
    if (payload_len > 0 && payload == NULL) {
        return -1;
    }

    raw[0] = DRO_LINK_VERSION;
    raw[1] = type;
    raw[2] = seq;
    raw[3] = flags;
    raw[4] = (uint8_t)(payload_len & 0xFF);
    raw[5] = (uint8_t)(payload_len >> 8);

    if (payload_len > 0) {
        memcpy(&raw[6], payload, payload_len);
    }

    uint16_t crc = dro_crc16(raw, (size_t)6 + payload_len);
    raw[6 + payload_len] = (uint8_t)(crc & 0xFF);
    raw[7 + payload_len] = (uint8_t)(crc >> 8);

    size_t raw_len = (size_t)8 + payload_len;
    size_t written = dro_cobs_encode(raw, raw_len, out, out_cap);
    if (written == 0 || written + 1 > out_cap) {
        return -1;
    }

    out[written] = 0x00;    /* frame delimiter */
    return (int)(written + 1);
}

int dro_link_frame_decode(const uint8_t *wire, size_t wire_len,
                          uint8_t *scratch, size_t scratch_cap,
                          dro_frame_t *out)
{
    if (wire == NULL || scratch == NULL || out == NULL) {
        return -1;
    }

    /* Tolerate callers that pass the trailing delimiter along. */
    while (wire_len > 0 && wire[wire_len - 1] == 0x00) {
        wire_len--;
    }
    if (wire_len == 0 || wire_len > DRO_LINK_MAX_WIRE) {
        return -1;
    }

    int raw_len = dro_cobs_decode(wire, wire_len, scratch, scratch_cap);
    if (raw_len < 0) {
        return -2;
    }
    if (raw_len < 8) {
        return -1;
    }

    dro_frame_hdr_t hdr;
    memcpy(&hdr, scratch, sizeof(hdr));

    if (hdr.version != DRO_LINK_VERSION) {
        return -3;
    }
    if (hdr.payload_len > DRO_LINK_MAX_PAYLOAD ||
        (size_t)raw_len != (size_t)hdr.payload_len + 8) {
        return -4;
    }

    uint16_t crc_calc = dro_crc16(scratch, (size_t)6 + hdr.payload_len);
    uint16_t crc_rx = (uint16_t)scratch[6 + hdr.payload_len] |
                      ((uint16_t)scratch[7 + hdr.payload_len] << 8);
    if (crc_calc != crc_rx) {
        return -5;
    }

    out->version = hdr.version;
    out->type = hdr.type;
    out->seq = hdr.seq;
    out->flags = hdr.flags;
    out->payload_len = hdr.payload_len;
    out->payload = (hdr.payload_len > 0) ? &scratch[6] : NULL;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Debug helper                                                              */
/* ------------------------------------------------------------------------- */
const char *dro_msg_name(uint8_t type)
{
    switch (type) {
    case DRO_MSG_HELLO:        return "HELLO";
    case DRO_MSG_TELEMETRY:    return "TELEMETRY";
    case DRO_MSG_CONFIG:       return "CONFIG";
    case DRO_MSG_ACK:          return "ACK";
    case DRO_MSG_ERROR:        return "ERROR";
    case DRO_MSG_DIAG:         return "DIAG";
    case DRO_MSG_PONG:         return "PONG";
    case DRO_MSG_INDEX_EVENT:  return "INDEX";
    case DRO_MSG_HELLO_REQ:    return "HELLO_REQ";
    case DRO_MSG_SET_STREAM:   return "SET_STREAM";
    case DRO_MSG_ZERO:         return "ZERO";
    case DRO_MSG_PRESET:       return "PRESET";
    case DRO_MSG_SET_AXIS:     return "SET_AXIS";
    case DRO_MSG_GET_CONFIG:   return "GET_CONFIG";
    case DRO_MSG_PING:         return "PING";
    case DRO_MSG_SAVE_CONFIG:  return "SAVE_CONFIG";
    case DRO_MSG_RESET_CONFIG: return "RESET_CONFIG";
    case DRO_MSG_REBOOT:       return "REBOOT";
    case DRO_MSG_SET_LINK:     return "SET_LINK";
    case DRO_MSG_GET_DIAG:     return "GET_DIAG";
    default:                   return "UNKNOWN";
    }
}
