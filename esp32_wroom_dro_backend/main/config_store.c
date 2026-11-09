/**
 * @file config_store.c
 * @brief NVS-backed settings with a debounced writer (protects the flash).
 */

#include "config_store.h"

#include <inttypes.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

static const char *TAG = "config";

#define CFG_NAMESPACE   "dro"
#define CFG_KEY         "cfg"
#define CFG_MAGIC       0x434F5244u   /* 'DROC' */
#define CFG_VERSION     1
#define CFG_AUTOSAVE_MS 2500

#ifndef CONFIG_DRO_DEVICE_NAME
#define CONFIG_DRO_DEVICE_NAME "wroom-dro"
#endif
#ifndef CONFIG_DRO_ESPNOW_CHANNEL
#define CONFIG_DRO_ESPNOW_CHANNEL 6
#endif
#ifndef CONFIG_DRO_DEFAULT_STREAM_HZ
#define CONFIG_DRO_DEFAULT_STREAM_HZ 50
#endif
#ifndef CONFIG_DRO_LINK_UART_BAUD
#define CONFIG_DRO_LINK_UART_BAUD 460800
#endif

typedef struct {
    uint32_t  magic;
    uint16_t  version;
    uint16_t  size;
    dro_cfg_t cfg;
} cfg_blob_t;

static dro_cfg_t          s_cfg;
static SemaphoreHandle_t  s_lock;
static bool               s_loaded;
static volatile bool      s_dirty;
static volatile int64_t   s_dirty_since_ms;

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */
void config_store_defaults(dro_cfg_t *out)
{
    memset(out, 0, sizeof(*out));
    out->enabled_mask = 0x1F;                      /* all five axes present */
    out->invert_mask = 0x00;
    out->index_mask = 0x1F;                        /* capture Z on all axes */
    out->link_mode = DRO_LINK_MODE_AUTO;
    out->espnow_channel = (uint8_t)CONFIG_DRO_ESPNOW_CHANNEL;
    out->peer_locked = 0;
    out->stream_hz = (uint16_t)CONFIG_DRO_DEFAULT_STREAM_HZ;
    out->uart_baud = (uint32_t)CONFIG_DRO_LINK_UART_BAUD;
    strncpy(out->name, CONFIG_DRO_DEVICE_NAME, sizeof(out->name) - 1);
}

static void sanitize(dro_cfg_t *cfg)
{
    cfg->enabled_mask &= 0x1F;
    cfg->invert_mask &= 0x1F;
    cfg->index_mask &= 0x1F;

    if (cfg->link_mode > DRO_LINK_MODE_ESPNOW) {
        cfg->link_mode = DRO_LINK_MODE_AUTO;
    }
    if (cfg->espnow_channel < 1 || cfg->espnow_channel > 13) {
        cfg->espnow_channel = 6;
    }
    if (cfg->stream_hz < 1 || cfg->stream_hz > 200) {
        cfg->stream_hz = 50;
    }
    if (cfg->uart_baud < 9600 || cfg->uart_baud > 3000000) {
        cfg->uart_baud = 460800;
    }
    cfg->name[sizeof(cfg->name) - 1] = '\0';
}

/* NVS write, called without holding s_lock. */
static esp_err_t write_blob(const dro_cfg_t *cfg)
{
    cfg_blob_t blob = {
        .magic = CFG_MAGIC,
        .version = CFG_VERSION,
        .size = sizeof(dro_cfg_t),
        .cfg = *cfg,
    };

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(CFG_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(nvs, CFG_KEY, &blob, sizeof(blob));
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "settings saved");
    } else {
        ESP_LOGE(TAG, "nvs write: %s", esp_err_to_name(err));
    }
    return err;
}

/* ------------------------------------------------------------------------- */
/* API                                                                       */
/* ------------------------------------------------------------------------- */
esp_err_t config_store_init(void)
{
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
        if (s_lock == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    config_store_defaults(&s_cfg);

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(CFG_NAMESPACE, NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        cfg_blob_t blob;
        size_t len = sizeof(blob);
        err = nvs_get_blob(nvs, CFG_KEY, &blob, &len);
        nvs_close(nvs);

        if (err == ESP_OK && len == sizeof(blob) &&
            blob.magic == CFG_MAGIC && blob.version == CFG_VERSION &&
            blob.size == sizeof(dro_cfg_t)) {
            s_cfg = blob.cfg;
            sanitize(&s_cfg);
            s_loaded = true;
            ESP_LOGI(TAG, "loaded settings: mask=0x%02x inv=0x%02x idx=0x%02x "
                          "mode=%u ch=%u hz=%u baud=%" PRIu32,
                     (unsigned)s_cfg.enabled_mask, (unsigned)s_cfg.invert_mask,
                     (unsigned)s_cfg.index_mask, (unsigned)s_cfg.link_mode,
                     (unsigned)s_cfg.espnow_channel, (unsigned)s_cfg.stream_hz,
                     s_cfg.uart_baud);
        } else {
            ESP_LOGW(TAG, "no valid settings (%s), using defaults", esp_err_to_name(err));
            s_loaded = false;
        }
    } else {
        ESP_LOGW(TAG, "nvs_open failed: %s, using defaults", esp_err_to_name(err));
    }

    if (!s_loaded) {
        write_blob(&s_cfg);
    }

    if (xTaskCreate(config_store_task, "cfg_save", 3072, NULL, 3, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void config_store_get(dro_cfg_t *out)
{
    if (out == NULL) {
        return;
    }
    if (s_lock != NULL) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
    }
    *out = s_cfg;
    if (s_lock != NULL) {
        xSemaphoreGive(s_lock);
    }
}

void config_store_set(const dro_cfg_t *in)
{
    if (in == NULL || s_lock == NULL) {
        return;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_cfg = *in;
    sanitize(&s_cfg);
    s_dirty = true;
    s_dirty_since_ms = esp_timer_get_time() / 1000;
    xSemaphoreGive(s_lock);
}

bool config_store_dirty(void)
{
    return s_dirty;
}

esp_err_t config_store_save(void)
{
    if (s_lock == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    dro_cfg_t copy;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    copy = s_cfg;
    s_dirty = false;
    xSemaphoreGive(s_lock);

    esp_err_t err = write_blob(&copy);
    if (err != ESP_OK) {
        s_dirty = true;
        s_dirty_since_ms = esp_timer_get_time() / 1000;
    }
    return err;
}

esp_err_t config_store_reset(void)
{
    dro_cfg_t defaults;
    config_store_defaults(&defaults);

    if (s_lock != NULL) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_cfg = defaults;
        s_dirty = false;
        xSemaphoreGive(s_lock);
    }

    return write_blob(&defaults);
}

void config_store_task(void *arg)
{
    (void)arg;

    for (;;) {
        if (s_dirty) {
            int64_t now = esp_timer_get_time() / 1000;
            if (now - s_dirty_since_ms >= CFG_AUTOSAVE_MS) {
                config_store_save();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
