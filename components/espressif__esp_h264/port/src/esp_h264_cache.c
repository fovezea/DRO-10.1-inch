/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_cache.h"
#include "esp_h264_cache.h"
#include "esp_h264_check.h"
#include "esp_log.h"

static const char *TAG = "H264_CACHE";

void esp_h264_cache_check_and_writeback(uint8_t *addr, uint32_t length)
{
    if (addr == NULL || length == 0) {
        return;
    }
    esp_err_t ret = esp_cache_msync(addr, length, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    // Error 0x67 (103/ESP_ERR_INVALID_ARG) means invalid addr or null pointer - non-fatal
    // Suppress logging for this non-fatal error to reduce log spam
    if (ret != ESP_OK && ret != 0x67) {
        ESP_H264_LOGE(TAG, "esp_cache_msync failed for writeback: addr=%p, length=%u, ret=0x%x", addr, length, ret);
    }
}

void esp_h264_cache_check_and_invalidate(uint8_t *addr, uint32_t length)
{
    if (addr == NULL || length == 0) {
        return;
    }
    esp_err_t ret = esp_cache_msync(addr, length, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    // Error 0x67 (103/ESP_ERR_INVALID_ARG) means invalid addr or null pointer - non-fatal
    // Suppress logging for this non-fatal error to reduce log spam
    if (ret != ESP_OK && ret != 0x67) {
        ESP_H264_LOGE(TAG, "esp_cache_msync failed for invalidate: addr=%p, length=%u, ret=0x%x", addr, length, ret);
    }
}
