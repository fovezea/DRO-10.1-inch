/**
 * @file link.h
 * @brief Session-based frontend link: telemetry streaming and command handling.
 *
 * The backend never streams blindly.  A frontend becomes the active peer by
 * sending DRO_MSG_HELLO_REQ (or any other request); from then on telemetry is
 * sent to exactly that peer on exactly that transport.  Any valid frame
 * refreshes the session; after DRO_SESSION_TIMEOUT_MS of silence the session
 * expires and the backend goes back to discovery beacons.
 *
 * This makes the link self-healing: unplug the UART, the ESP-NOW peer can take
 * over; reset the frontend, it just says HELLO again.
 */

#ifndef LINK_H
#define LINK_H

#include <stdbool.h>
#include <stdint.h>

#include "dro_link_protocol.h"
#include "esp_err.h"

typedef enum {
    DRO_TR_NONE = 0,
    DRO_TR_UART = 1,
    DRO_TR_ESPNOW = 2,
} dro_transport_id_t;

typedef struct {
    dro_transport_id_t id;
    const char        *name;
    bool             (*ready)(void);
    esp_err_t        (*send)(const uint8_t *dst_mac, const uint8_t *data, size_t len);
    esp_err_t        (*set_channel)(uint8_t channel);   /* optional */
    esp_err_t        (*set_baud)(uint32_t baud);        /* optional */
} dro_transport_ops_t;

typedef struct {
    bool               session_valid;
    dro_transport_id_t session_transport;
    uint8_t            session_mac[6];
    uint16_t           session_hz;
    uint8_t            session_axes_mask;
    uint32_t           session_age_ms;
    uint32_t           tx_frames;
    uint32_t           rx_frames;
    uint32_t           crc_errors;
    uint32_t           dropped;
    bool               wifi_ready;
    bool               wifi_error;
    bool               self_test_ok;
    uint8_t            own_mac[6];
} dro_link_status_t;

/** Create the RX queue, read the station MAC and start the worker. */
esp_err_t dro_link_init(void);

/** Transports call this once at boot. */
void dro_link_register_transport(const dro_transport_ops_t *ops);

/**
 * @brief Called by a transport for every received datagram / decoded frame.
 *
 * Safe to call from the Wi-Fi task or a UART task; the frame is validated here
 * and handed to the link task through a queue.
 */
void dro_link_rx(dro_transport_id_t id, const uint8_t *src_mac,
                 const uint8_t *data, size_t len);

/** Worker task (started by dro_link_init). */
void dro_link_task(void *arg);

/** Report the Wi-Fi/ESP-NOW state, used for telemetry status and the LED. */
void dro_link_set_wifi_state(bool ready, bool error);

/** Snapshot for the console and the LED. */
void dro_link_get_status(dro_link_status_t *out);

/** True while a frontend session is active. */
bool dro_link_session_active(void);

/** Remember that the protocol self-test passed. */
void dro_link_mark_self_test(bool ok);

/** Push the current config into the transports (baud, channel, mode). */
esp_err_t dro_link_apply_config(void);

#endif /* LINK_H */
