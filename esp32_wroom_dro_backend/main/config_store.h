/**
 * @file config_store.h
 * @brief Persistent backend settings (NVS) with debounced auto-save.
 *
 * Only settings live here.  Positions are deliberately not persisted: the
 * frontend owns the coordinate system, the backend owns the hardware.
 */

#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <stdbool.h>
#include <stdint.h>

#include "dro_link_protocol.h"
#include "esp_err.h"

typedef struct {
    uint8_t  enabled_mask;      /* bit i = axis present           */
    uint8_t  invert_mask;       /* bit i = counting direction -1  */
    uint8_t  index_mask;        /* bit i = Z pulse capture on     */
    uint8_t  link_mode;         /* DRO_LINK_MODE_*                */
    uint8_t  espnow_channel;    /* 1..13                          */
    uint8_t  peer_locked;       /* 1 = only talk to peer_mac      */
    uint8_t  peer_mac[6];
    uint16_t stream_hz;         /* default telemetry rate         */
    uint32_t uart_baud;
    char     name[DRO_LINK_NAME_LEN];
} dro_cfg_t;

/** Load from NVS (or create defaults) and start the auto-save task. */
esp_err_t config_store_init(void);

/** Thread-safe snapshot. */
void config_store_get(dro_cfg_t *out);

/** Replace the whole configuration and mark it dirty. */
void config_store_set(const dro_cfg_t *in);

/** Fill `out` with the factory defaults. */
void config_store_defaults(dro_cfg_t *out);

bool config_store_dirty(void);

/** Force a write to NVS now. */
esp_err_t config_store_save(void);

/** Restore defaults and write them. */
esp_err_t config_store_reset(void);

/** Auto-save worker, created by config_store_init(). */
void config_store_task(void *arg);

#endif /* CONFIG_STORE_H */
