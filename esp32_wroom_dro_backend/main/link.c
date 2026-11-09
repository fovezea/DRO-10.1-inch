/**
 * @file link.c
 * @brief Session management, telemetry streaming and command dispatch.
 */

#include "link.h"

#include <stdio.h>
#include <string.h>

#include "axis_reader.h"
#include "config_store.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "link";

#ifndef CONFIG_DRO_SESSION_TIMEOUT_MS
#define CONFIG_DRO_SESSION_TIMEOUT_MS 3000
#endif

#define RX_QUEUE_LEN        8
#define BEACON_PERIOD_US    1000000LL
#define FW_VERSION          0x0100

typedef struct {
    dro_transport_id_t tr;
    uint8_t  mac[6];
    uint8_t  type;
    uint8_t  seq;
    uint8_t  flags;
    uint16_t len;
    uint8_t  payload[DRO_LINK_MAX_PAYLOAD];
} rx_msg_t;

typedef struct {
    bool               valid;
    dro_transport_id_t tr;
    uint8_t            mac[6];
    int64_t            last_rx_us;
    uint16_t           hz;
    uint8_t            axes_mask;
    bool               index_events;
} session_t;

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static const dro_transport_ops_t *s_ops[3];
static QueueHandle_t s_rxq;
static session_t     s_session;
static uint8_t       s_own_mac[6];
static uint8_t       s_tx_seq;
static int64_t       s_next_stream_us;

static uint32_t s_tx_frames;
static uint32_t s_rx_frames;
static uint32_t s_crc_errors;
static uint32_t s_dropped;
static uint32_t s_tx_by_tr[3];
static uint32_t s_rx_by_tr[3];

static bool s_wifi_ready;
static bool s_wifi_error;
static bool s_index_seen;
static bool s_self_test_ok;

/* Cached from the config so the RX hot path does not take the config lock. */
static uint8_t s_link_mode = DRO_LINK_MODE_AUTO;
static bool    s_peer_locked;
static uint8_t s_peer_mac[6];

/* ------------------------------------------------------------------------- */
/* Small helpers                                                             */
/* ------------------------------------------------------------------------- */
static int64_t now_us(void)
{
    return esp_timer_get_time();
}

static const char *mac_to_str(const uint8_t mac[6], char out[18])
{
    snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             (unsigned)mac[0], (unsigned)mac[1], (unsigned)mac[2],
             (unsigned)mac[3], (unsigned)mac[4], (unsigned)mac[5]);
    return out;
}

static uint32_t uptime_ms(void)
{
    return (uint32_t)(now_us() / 1000);
}

static void session_get(session_t *out)
{
    portENTER_CRITICAL(&s_mux);
    *out = s_session;
    portEXIT_CRITICAL(&s_mux);
}

static void session_put(const session_t *in)
{
    portENTER_CRITICAL(&s_mux);
    s_session = *in;
    portEXIT_CRITICAL(&s_mux);
}

static const dro_transport_ops_t *ops_of(dro_transport_id_t tr)
{
    if (tr < 0 || tr > 2) {
        return NULL;
    }
    return s_ops[tr];
}

static void stats_add(uint32_t *counter)
{
    portENTER_CRITICAL(&s_mux);
    (*counter)++;
    portEXIT_CRITICAL(&s_mux);
}

/* ------------------------------------------------------------------------- */
/* Transmit                                                                  */
/* ------------------------------------------------------------------------- */
static esp_err_t send_msg(dro_transport_id_t tr, const uint8_t *mac, uint8_t type,
                          uint8_t flags, const void *payload, uint16_t len)
{
    const dro_transport_ops_t *ops = ops_of(tr);
    if (ops == NULL || ops->send == NULL || (ops->ready != NULL && !ops->ready())) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t wire[DRO_LINK_MAX_WIRE];
    uint8_t seq;
    portENTER_CRITICAL(&s_mux);
    seq = s_tx_seq++;
    portEXIT_CRITICAL(&s_mux);

    int n = dro_link_frame_encode(type, seq, flags, payload, len, wire, sizeof(wire));
    if (n < 0) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err = ops->send(mac, wire, (size_t)n);
    if (err == ESP_OK) {
        portENTER_CRITICAL(&s_mux);
        s_tx_frames++;
        s_tx_by_tr[tr]++;
        portEXIT_CRITICAL(&s_mux);
    }
    return err;
}

static void send_hello(dro_transport_id_t tr, const uint8_t *mac, uint8_t flags)
{
    dro_cfg_t cfg;
    config_store_get(&cfg);

    dro_hello_t h = {0};
    h.fw_version = FW_VERSION;
    h.protocol_version = DRO_LINK_VERSION;
    h.axis_count = AXIS_READER_COUNT;
    h.features = DRO_FEAT_QUAD | DRO_FEAT_INDEX | DRO_FEAT_UART | DRO_FEAT_SIM;
    if (s_wifi_ready) {
        h.features |= DRO_FEAT_ESPNOW;
    }
    memcpy(h.mac, s_own_mac, sizeof(h.mac));
    strncpy(h.name, cfg.name, sizeof(h.name) - 1);
    h.uptime_ms = uptime_ms();

    send_msg(tr, mac, DRO_MSG_HELLO, flags, &h, sizeof(h));
}

static void send_config(dro_transport_id_t tr, const uint8_t *mac)
{
    dro_cfg_t cfg;
    config_store_get(&cfg);

    dro_config_t c = {0};
    c.fw_version = FW_VERSION;
    c.protocol_version = DRO_LINK_VERSION;
    c.axis_count = AXIS_READER_COUNT;
    c.enabled_mask = cfg.enabled_mask;
    c.invert_mask = cfg.invert_mask;
    c.index_mask = cfg.index_mask;
    c.link_mode = cfg.link_mode;
    c.espnow_channel = cfg.espnow_channel;
    c.peer_locked = cfg.peer_locked;
    c.stream_hz = cfg.stream_hz;
    c.uart_baud = cfg.uart_baud;
    memcpy(c.peer_mac, cfg.peer_mac, sizeof(c.peer_mac));
    memcpy(c.mac, s_own_mac, sizeof(c.mac));
    strncpy(c.name, cfg.name, sizeof(c.name) - 1);

    send_msg(tr, mac, DRO_MSG_CONFIG, DRO_FLAG_IS_REPLY, &c, sizeof(c));
}

static void send_diag(dro_transport_id_t tr, const uint8_t *mac)
{
    session_t s;
    session_get(&s);

    dro_diag_t d = {0};
    d.free_heap = esp_get_free_heap_size();
    d.loop_hz = 1000;
    d.stream_hz = s.valid ? s.hz : 0;
    d.axis_count = AXIS_READER_COUNT;

    for (int a = 0; a < AXIS_READER_COUNT; a++) {
        axis_reader_diag_t ad;
        axis_reader_get_diag(a, &ad);
        d.edges_per_s[a] = ad.counts_per_s;
        d.overflow_count[a] = ad.overflow_count;
        d.index_count[a] = ad.index_count;
    }

    portENTER_CRITICAL(&s_mux);
    d.uart_rx_frames = s_rx_by_tr[DRO_TR_UART];
    d.uart_tx_frames = s_tx_by_tr[DRO_TR_UART];
    d.espnow_rx_frames = s_rx_by_tr[DRO_TR_ESPNOW];
    d.espnow_tx_frames = s_tx_by_tr[DRO_TR_ESPNOW];
    d.crc_errors = s_crc_errors;
    d.dropped_frames = s_dropped;
    portEXIT_CRITICAL(&s_mux);

    send_msg(tr, mac, DRO_MSG_DIAG, DRO_FLAG_IS_REPLY, &d, sizeof(d));
}

static void send_ack(const rx_msg_t *m, uint8_t status)
{
    dro_ack_t a = {
        .acked_type = m->type,
        .acked_seq = m->seq,
        .status = status,
    };
    send_msg(m->tr, m->mac, DRO_MSG_ACK, DRO_FLAG_IS_REPLY, &a, sizeof(a));
}

static void send_error(const rx_msg_t *m, uint8_t code, uint8_t detail)
{
    dro_error_t e = { .code = code, .detail = detail };
    send_msg(m->tr, m->mac, DRO_MSG_ERROR, DRO_FLAG_IS_REPLY, &e, sizeof(e));
}

static uint16_t status_bits(const session_t *s)
{
    uint16_t st = 0;

    if (s != NULL && s->valid) {
        st |= (s->tr == DRO_TR_UART) ? DRO_ST_UART_SESSION : DRO_ST_ENOW_SESSION;
    }
    if (s_wifi_ready) {
        st |= DRO_ST_WIFI_READY;
    }
    if (s_wifi_error) {
        st |= DRO_ST_WIFI_ERROR;
    }
    if (config_store_dirty()) {
        st |= DRO_ST_CFG_DIRTY;
    }
    if (axis_reader_simulation()) {
        st |= DRO_ST_SIMULATION;
    }
    if (s_index_seen) {
        st |= DRO_ST_INDEX_SEEN;
    }
    if (s_self_test_ok) {
        st |= DRO_ST_SELFTEST_OK;
    }
    return st;
}

static void send_telemetry(const session_t *s)
{
    if (s == NULL || !s->valid) {
        return;
    }

    int32_t pos[AXIS_READER_COUNT];
    uint8_t index_flags = 0;
    uint32_t sample_seq = 0;
    axis_reader_snapshot(pos, &index_flags, &sample_seq);

    dro_telemetry_t t = {0};
    t.sample_seq = sample_seq;
    t.uptime_ms = uptime_ms();
    t.status = status_bits(s);
    t.axis_count = AXIS_READER_COUNT;
    t.index_flags = index_flags;
    memcpy(t.pos, pos, sizeof(t.pos));

    send_msg(s->tr, s->mac, DRO_MSG_TELEMETRY, 0, &t, sizeof(t));
}

/* ------------------------------------------------------------------------- */
/* Configuration application                                                 */
/* ------------------------------------------------------------------------- */
static void apply_cfg_to_axes(const dro_cfg_t *cfg)
{
    for (int a = 0; a < AXIS_READER_COUNT; a++) {
        axis_reader_set_invert(a, (cfg->invert_mask >> a) & 1);
        axis_reader_set_enabled(a, (cfg->enabled_mask >> a) & 1);
        axis_reader_set_index_enable(a, (cfg->index_mask >> a) & 1);
    }
}

esp_err_t dro_link_apply_config(void)
{
    dro_cfg_t cfg;
    config_store_get(&cfg);

    s_link_mode = cfg.link_mode;
    s_peer_locked = (cfg.peer_locked != 0);
    memcpy(s_peer_mac, cfg.peer_mac, sizeof(s_peer_mac));

    apply_cfg_to_axes(&cfg);

    /* Drop a session that the new policy no longer allows. */
    session_t s;
    session_get(&s);
    if (s.valid) {
        bool transport_ok = (cfg.link_mode == DRO_LINK_MODE_AUTO) ||
                            (cfg.link_mode == DRO_LINK_MODE_UART && s.tr == DRO_TR_UART) ||
                            (cfg.link_mode == DRO_LINK_MODE_ESPNOW && s.tr == DRO_TR_ESPNOW);
        bool peer_ok = !s_peer_locked || (s.tr == DRO_TR_UART) ||
                       (memcmp(s.mac, s_peer_mac, sizeof(s.mac)) == 0);
        if (!transport_ok || !peer_ok) {
            ESP_LOGW(TAG, "session dropped: link policy changed");
            s.valid = false;
            session_put(&s);
        }
    }

    const dro_transport_ops_t *uart = ops_of(DRO_TR_UART);
    if (uart != NULL && uart->set_baud != NULL) {
        uart->set_baud(cfg.uart_baud);
    }

    const dro_transport_ops_t *enow = ops_of(DRO_TR_ESPNOW);
    if (enow != NULL && enow->set_channel != NULL) {
        enow->set_channel(cfg.espnow_channel);
    }

    return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/* Session handling                                                          */
/* ------------------------------------------------------------------------- */
/** Refresh the session if this message came from the current peer. */
static bool session_refresh(const rx_msg_t *m)
{
    session_t s;
    session_get(&s);
    if (!s.valid) {
        return false;
    }
    if (s.tr != m->tr || memcmp(s.mac, m->mac, sizeof(s.mac)) != 0) {
        return false;
    }
    s.last_rx_us = now_us();
    session_put(&s);
    return true;
}

static void session_open(const rx_msg_t *m, uint16_t hz, uint8_t axes_mask)
{
    dro_cfg_t cfg;
    config_store_get(&cfg);

    if (hz < 1 || hz > 200) {
        hz = cfg.stream_hz;
    }
    if (axes_mask == 0) {
        axes_mask = cfg.enabled_mask;
    }

    session_t s = {
        .valid = true,
        .tr = m->tr,
        .last_rx_us = now_us(),
        .hz = hz,
        .axes_mask = axes_mask,
        .index_events = true,
    };
    memcpy(s.mac, m->mac, sizeof(s.mac));
    session_put(&s);

    s_next_stream_us = 0;   /* send telemetry immediately */

    char mac[18];
    ESP_LOGI(TAG, "session opened: %s %s @ %u Hz",
             ops_of(m->tr) ? ops_of(m->tr)->name : "?", mac_to_str(m->mac, mac),
             (unsigned)hz);
}

/* ------------------------------------------------------------------------- */
/* Command dispatch                                                          */
/* ------------------------------------------------------------------------- */
static void handle_message(const rx_msg_t *m)
{
    /* Any frame from the current peer keeps the session alive. */
    session_refresh(m);

    switch (m->type) {
    case DRO_MSG_HELLO_REQ: {
        if (m->len < sizeof(dro_hello_req_t)) {
            send_error(m, DRO_ERR_BAD_ARG, 0);
            break;
        }
        const dro_hello_req_t *req = (const dro_hello_req_t *)m->payload;
        session_open(m, req->want_hz, req->axes_mask);
        send_hello(m->tr, m->mac, DRO_FLAG_IS_REPLY);
        break;
    }

    case DRO_MSG_SET_STREAM: {
        if (m->len < sizeof(dro_stream_cfg_t)) {
            send_error(m, DRO_ERR_BAD_ARG, 0);
            break;
        }
        const dro_stream_cfg_t *req = (const dro_stream_cfg_t *)m->payload;
        session_t s;
        session_get(&s);
        if (!s.valid) {
            session_open(m, req->hz, req->axes_mask);
            session_get(&s);
        }
        if (req->hz >= 1 && req->hz <= 200) {
            s.hz = req->hz;
        }
        if (req->axes_mask != 0) {
            s.axes_mask = req->axes_mask;
        }
        s.index_events = (req->flags & 0x01) != 0;
        s.last_rx_us = now_us();
        session_put(&s);
        s_next_stream_us = 0;
        send_ack(m, DRO_ERR_NONE);
        break;
    }

    case DRO_MSG_ZERO: {
        uint8_t mask = 0x1F;
        if (m->len >= sizeof(dro_zero_t)) {
            mask = ((const dro_zero_t *)m->payload)->axis_mask;
        }
        axis_reader_zero(mask & 0x1F);
        send_ack(m, DRO_ERR_NONE);
        break;
    }

    case DRO_MSG_PRESET: {
        if (m->len < sizeof(dro_preset_t)) {
            send_error(m, DRO_ERR_BAD_ARG, 0);
            break;
        }
        const dro_preset_t *p = (const dro_preset_t *)m->payload;
        esp_err_t err = axis_reader_preset(p->axis, p->value);
        if (err != ESP_OK) {
            send_ack(m, DRO_ERR_BAD_AXIS);
        } else {
            send_ack(m, DRO_ERR_NONE);
        }
        break;
    }

    case DRO_MSG_SET_AXIS: {
        if (m->len < sizeof(dro_set_axis_t)) {
            send_error(m, DRO_ERR_BAD_ARG, 0);
            break;
        }
        const dro_set_axis_t *p = (const dro_set_axis_t *)m->payload;
        if (p->axis >= AXIS_READER_COUNT) {
            send_ack(m, DRO_ERR_BAD_AXIS);
            break;
        }

        axis_reader_set_enabled(p->axis, p->enabled != 0);
        axis_reader_set_invert(p->axis, p->invert != 0);
        axis_reader_set_index_enable(p->axis, p->index_enable != 0);

        dro_cfg_t cfg;
        config_store_get(&cfg);
        if (p->enabled) {
            cfg.enabled_mask |= (uint8_t)(1u << p->axis);
        } else {
            cfg.enabled_mask &= (uint8_t)~(1u << p->axis);
        }
        if (p->invert) {
            cfg.invert_mask |= (uint8_t)(1u << p->axis);
        } else {
            cfg.invert_mask &= (uint8_t)~(1u << p->axis);
        }
        if (p->index_enable) {
            cfg.index_mask |= (uint8_t)(1u << p->axis);
        } else {
            cfg.index_mask &= (uint8_t)~(1u << p->axis);
        }
        config_store_set(&cfg);

        send_ack(m, DRO_ERR_NONE);
        send_config(m->tr, m->mac);
        break;
    }

    case DRO_MSG_GET_CONFIG:
        send_config(m->tr, m->mac);
        break;

    case DRO_MSG_GET_DIAG:
        send_diag(m->tr, m->mac);
        break;

    case DRO_MSG_PING: {
        uint8_t echo[8] = {0};
        uint16_t n = m->len > sizeof(echo) ? sizeof(echo) : m->len;
        if (n > 0) {
            memcpy(echo, m->payload, n);
        }
        send_msg(m->tr, m->mac, DRO_MSG_PONG, DRO_FLAG_IS_REPLY, echo, n);
        break;
    }

    case DRO_MSG_SAVE_CONFIG:
        send_ack(m, config_store_save() == ESP_OK ? DRO_ERR_NONE : DRO_ERR_BAD_STATE);
        break;

    case DRO_MSG_RESET_CONFIG:
        config_store_reset();
        dro_link_apply_config();
        send_ack(m, DRO_ERR_NONE);
        send_config(m->tr, m->mac);
        break;

    case DRO_MSG_SET_LINK: {
        if (m->len < sizeof(dro_set_link_t)) {
            send_error(m, DRO_ERR_BAD_ARG, 0);
            break;
        }
        const dro_set_link_t *p = (const dro_set_link_t *)m->payload;
        dro_cfg_t cfg;
        config_store_get(&cfg);
        if (p->mode <= DRO_LINK_MODE_ESPNOW) {
            cfg.link_mode = p->mode;
        }
        if (p->channel >= 1 && p->channel <= 13) {
            cfg.espnow_channel = p->channel;
        }
        cfg.peer_locked = p->lock ? 1 : 0;
        memcpy(cfg.peer_mac, p->peer_mac, sizeof(cfg.peer_mac));
        config_store_set(&cfg);
        dro_link_apply_config();
        send_ack(m, DRO_ERR_NONE);
        send_config(m->tr, m->mac);
        break;
    }

    case DRO_MSG_REBOOT:
        ESP_LOGW(TAG, "reboot requested by frontend");
        send_ack(m, DRO_ERR_NONE);
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_restart();
        break;

    default:
        ESP_LOGW(TAG, "unsupported message type 0x%02x", (unsigned)m->type);
        send_error(m, DRO_ERR_NOT_SUPPORTED, m->type);
        break;
    }
}

/* ------------------------------------------------------------------------- */
/* Index events                                                              */
/* ------------------------------------------------------------------------- */
static void on_index_event(int axis, int32_t pos, uint32_t t_ms)
{
    s_index_seen = true;

    session_t s;
    session_get(&s);
    if (!s.valid || !s.index_events) {
        return;
    }

    dro_index_event_t ev = {
        .axis = (uint8_t)axis,
        .pos = pos,
        .t_ms = t_ms,
    };
    send_msg(s.tr, s.mac, DRO_MSG_INDEX_EVENT, 0, &ev, sizeof(ev));
}

/* ------------------------------------------------------------------------- */
/* RX path                                                                   */
/* ------------------------------------------------------------------------- */
void dro_link_rx(dro_transport_id_t id, const uint8_t *src_mac,
                 const uint8_t *data, size_t len)
{
    if (s_rxq == NULL || data == NULL || len == 0) {
        return;
    }

    /* Honour the configured link policy before doing any work. */
    if (s_link_mode == DRO_LINK_MODE_UART && id != DRO_TR_UART) {
        return;
    }
    if (s_link_mode == DRO_LINK_MODE_ESPNOW && id != DRO_TR_ESPNOW) {
        return;
    }
    if (id == DRO_TR_ESPNOW && s_peer_locked && src_mac != NULL &&
        memcmp(src_mac, s_peer_mac, sizeof(s_peer_mac)) != 0) {
        stats_add(&s_dropped);
        return;
    }

    uint8_t scratch[DRO_LINK_MAX_FRAME];
    dro_frame_t f;
    int rc = dro_link_frame_decode(data, len, scratch, sizeof(scratch), &f);
    if (rc != 0) {
        stats_add(&s_crc_errors);
        return;
    }

    rx_msg_t m = {0};
    m.tr = id;
    if (src_mac != NULL) {
        memcpy(m.mac, src_mac, sizeof(m.mac));
    }
    m.type = f.type;
    m.seq = f.seq;
    m.flags = f.flags;
    m.len = f.payload_len;
    if (m.len > 0) {
        memcpy(m.payload, f.payload, m.len);
    }

    if (xQueueSend(s_rxq, &m, 0) != pdTRUE) {
        stats_add(&s_dropped);
        return;
    }

    portENTER_CRITICAL(&s_mux);
    s_rx_frames++;
    s_rx_by_tr[id]++;
    portEXIT_CRITICAL(&s_mux);
}

/* ------------------------------------------------------------------------- */
/* Worker                                                                    */
/* ------------------------------------------------------------------------- */
void dro_link_task(void *arg)
{
    (void)arg;

    int64_t next_beacon_us = 0;

    for (;;) {
        rx_msg_t m;
        /* Drain a few messages so a burst of commands is handled promptly. */
        for (int i = 0; i < 4; i++) {
            if (xQueueReceive(s_rxq, &m, i == 0 ? pdMS_TO_TICKS(2) : 0) != pdTRUE) {
                break;
            }
            handle_message(&m);
        }

        int64_t now = now_us();

        session_t s;
        session_get(&s);
        if (s.valid && (now - s.last_rx_us) > (int64_t)CONFIG_DRO_SESSION_TIMEOUT_MS * 1000LL) {
            ESP_LOGW(TAG, "session timed out (%s)", ops_of(s.tr) ? ops_of(s.tr)->name : "?");
            s.valid = false;
            session_put(&s);
            s_next_stream_us = 0;
        }

        if (s.valid) {
            uint16_t hz = s.hz ? s.hz : 50;
            int64_t period = 1000000LL / hz;
            if (s_next_stream_us == 0 || now >= s_next_stream_us) {
                if (s_next_stream_us == 0 || now - s_next_stream_us > 1000000LL) {
                    s_next_stream_us = now;
                }
                send_telemetry(&s);
                s_next_stream_us += period;
            }
        } else if (s_wifi_ready && now >= next_beacon_us) {
            static const uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            next_beacon_us = now + BEACON_PERIOD_US;
            send_hello(DRO_TR_ESPNOW, bcast, 0);
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Init / status                                                             */
/* ------------------------------------------------------------------------- */
esp_err_t dro_link_init(void)
{
    if (s_rxq != NULL) {
        return ESP_OK;
    }

    s_rxq = xQueueCreate(RX_QUEUE_LEN, sizeof(rx_msg_t));
    if (s_rxq == NULL) {
        return ESP_ERR_NO_MEM;
    }

    memset(s_own_mac, 0, sizeof(s_own_mac));
    esp_read_mac(s_own_mac, ESP_MAC_WIFI_STA);

    axis_reader_set_index_callback(on_index_event);

    if (xTaskCreatePinnedToCore(dro_link_task, "dro_link", 4096, NULL, 6, NULL, 0) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "link ready, device MAC %02x:%02x:%02x:%02x:%02x:%02x",
             (unsigned)s_own_mac[0], (unsigned)s_own_mac[1], (unsigned)s_own_mac[2],
             (unsigned)s_own_mac[3], (unsigned)s_own_mac[4], (unsigned)s_own_mac[5]);
    return ESP_OK;
}

void dro_link_register_transport(const dro_transport_ops_t *ops)
{
    if (ops == NULL || ops->id < 0 || ops->id > 2) {
        return;
    }
    s_ops[ops->id] = ops;
    ESP_LOGI(TAG, "transport registered: %s", ops->name);
}

void dro_link_set_wifi_state(bool ready, bool error)
{
    s_wifi_ready = ready;
    s_wifi_error = error;
}

void dro_link_get_status(dro_link_status_t *out)
{
    if (out == NULL) {
        return;
    }

    session_t s;
    session_get(&s);

    memset(out, 0, sizeof(*out));
    out->session_valid = s.valid;
    out->session_transport = s.tr;
    memcpy(out->session_mac, s.mac, sizeof(out->session_mac));
    out->session_hz = s.hz;
    out->session_axes_mask = s.axes_mask;
    out->session_age_ms = s.valid ? (uint32_t)((now_us() - s.last_rx_us) / 1000) : 0;

    portENTER_CRITICAL(&s_mux);
    out->tx_frames = s_tx_frames;
    out->rx_frames = s_rx_frames;
    out->crc_errors = s_crc_errors;
    out->dropped = s_dropped;
    portEXIT_CRITICAL(&s_mux);

    out->wifi_ready = s_wifi_ready;
    out->wifi_error = s_wifi_error;
    out->self_test_ok = s_self_test_ok;
    memcpy(out->own_mac, s_own_mac, sizeof(out->own_mac));
}

bool dro_link_session_active(void)
{
    session_t s;
    session_get(&s);
    return s.valid;
}

void dro_link_mark_self_test(bool ok)
{
    s_self_test_ok = ok;
}
