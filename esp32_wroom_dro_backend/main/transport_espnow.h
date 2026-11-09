/**
 * @file transport_espnow.h
 * @brief Wireless link transport over ESP-NOW (ESP32 station, no AP).
 */

#ifndef TRANSPORT_ESPNOW_H
#define TRANSPORT_ESPNOW_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "link.h"

esp_err_t transport_espnow_init(void);
bool transport_espnow_ready(void);
esp_err_t transport_espnow_send(const uint8_t *dst_mac, const uint8_t *data, size_t len);
esp_err_t transport_espnow_set_channel(uint8_t channel);
uint8_t transport_espnow_channel(void);
bool transport_espnow_wifi_error(void);
const dro_transport_ops_t *transport_espnow_ops(void);

#endif /* TRANSPORT_ESPNOW_H */
