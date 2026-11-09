/**
 * @file transport_espnow.c
 * @brief ESP-NOW transport.
 *
 * The backend is a plain ESP-NOW station: no access point, no IP stack, no
 * association.  Frames are the same COBS frames as on the UART, carried in a
 * single ESP-NOW packet (well under the 250 byte limit).
 *
 * Peers are added on demand: broadcast for discovery beacons, and one unicast
 * peer per frontend that says HELLO.  No encryption/LMK for now - this is a
 * short-range point-to-point bench link; enable an LMK later if the machine
 * lives somewhere with other traffic.
 */

#include "transport_espnow.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"

static const char *TAG = "espnow";

#ifndef CONFIG_DRO_ESPNOW_CHANNEL
#define CONFIG_DRO_ESPNOW_CHANNEL 6
#endif

static volatile bool s_ready;
static volatile bool s_error;
static uint8_t       s_channel = CONFIG_DRO_ESPNOW_CHANNEL;
static uint32_t      s_tx_fail;

static const uint8_t s_broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool transport_espnow_ready(void)
{
    return s_ready;
}

bool transport_espnow_wifi_error(void)
{
    return s_error;
}

uint8_t transport_espnow_channel(void)
{
    return s_channel;
}

static esp_err_t ensure_peer(const uint8_t *mac)
{
    if (esp_now_is_peer_exist(mac)) {
        return ESP_OK;
    }

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac, sizeof(peer.peer_addr));
    peer.channel = 0;            /* follow the interface channel */
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peer);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "peer added %02x:%02x:%02x:%02x:%02x:%02x",
                 (unsigned)mac[0], (unsigned)mac[1], (unsigned)mac[2],
                 (unsigned)mac[3], (unsigned)mac[4], (unsigned)mac[5]);
    }
    return err;
}

esp_err_t transport_espnow_send(const uint8_t *dst_mac, const uint8_t *data, size_t len)
{
    if (!s_ready || data == NULL || len == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (len > 250) {
        return ESP_ERR_INVALID_SIZE;
    }

    const uint8_t *dst = (dst_mac != NULL) ? dst_mac : s_broadcast;

    esp_err_t err = ensure_peer(dst);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_now_send(dst, data, len);
    if (err == ESP_ERR_ESPNOW_NOT_FOUND) {
        /* Peer table was flushed (e.g. channel change): re-add and retry. */
        if (ensure_peer(dst) == ESP_OK) {
            err = esp_now_send(dst, data, len);
        }
    }
    if (err != ESP_OK) {
        s_tx_fail++;
    }
    return err;
}

esp_err_t transport_espnow_set_channel(uint8_t channel)
{
    if (channel < 1 || channel > 13) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_ready) {
        s_channel = channel;
        return ESP_OK;
    }
    if (channel == s_channel) {
        return ESP_OK;
    }

    esp_err_t err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (err == ESP_OK) {
        s_channel = channel;
        ESP_LOGI(TAG, "channel now %u", (unsigned)channel);
    } else {
        ESP_LOGW(TAG, "channel %u rejected: %s (country code limit?)",
                 (unsigned)channel, esp_err_to_name(err));
    }
    return err;
}

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (info == NULL || data == NULL || len <= 0) {
        return;
    }
    dro_link_rx(DRO_TR_ESPNOW, info->src_addr, data, (size_t)len);
}

static void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status)
{
    (void)info;
    if (status != ESP_NOW_SEND_SUCCESS) {
        s_tx_fail++;
    }
}

esp_err_t transport_espnow_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init: %s", esp_err_to_name(err));
        goto fail;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "event loop: %s", esp_err_to_name(err));
        goto fail;
    }

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wifi_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init: %s", esp_err_to_name(err));
        goto fail;
    }

    /* No station config is ever stored: this radio only speaks ESP-NOW. */
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_mode: %s", esp_err_to_name(err));
        goto fail;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start: %s", esp_err_to_name(err));
        goto fail;
    }

    esp_wifi_set_ps(WIFI_PS_NONE);

    err = esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "channel %u: %s, retrying on 6", (unsigned)s_channel,
                 esp_err_to_name(err));
        s_channel = 6;
        err = esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
        if (err != ESP_OK) {
            goto fail;
        }
    }

#if CONFIG_DRO_ESPNOW_LR
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    ESP_LOGW(TAG, "long-range mode enabled (the frontend must match)");
#endif

    err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_init: %s", esp_err_to_name(err));
        goto fail;
    }

    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    err = ensure_peer(s_broadcast);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "broadcast peer: %s", esp_err_to_name(err));
        goto fail;
    }

    s_ready = true;
    s_error = false;
    dro_link_set_wifi_state(true, false);
    ESP_LOGI(TAG, "ESP-NOW ready on channel %u", (unsigned)s_channel);
    return ESP_OK;

fail:
    s_ready = false;
    s_error = true;
    dro_link_set_wifi_state(false, true);
    return (err != ESP_OK) ? err : ESP_FAIL;
}

const dro_transport_ops_t *transport_espnow_ops(void)
{
    static const dro_transport_ops_t ops = {
        .id = DRO_TR_ESPNOW,
        .name = "espnow",
        .ready = transport_espnow_ready,
        .send = transport_espnow_send,
        .set_channel = transport_espnow_set_channel,
        .set_baud = NULL,
    };
    return &ops;
}
