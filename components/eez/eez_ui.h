#pragma once

#include "lvgl.h"
#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EEZ UI initialization structure
 */
typedef struct {
    lv_obj_t *screen;           // Main screen object
    lv_obj_t *container;        // Main container
    bool auto_scan;             // Enable automatic WiFi scanning
    uint32_t scan_interval_ms;  // Scan interval in milliseconds
} eez_ui_config_t;

/**
 * @brief Default EEZ UI configuration
 */
#define EEZ_UI_CONFIG_DEFAULT() { \
    .screen = NULL, \
    .container = NULL, \
    .auto_scan = true, \
    .scan_interval_ms = 30000 \
}

/**
 * @brief Initialize EEZ UI system
 * 
 * @param config Configuration structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t eez_ui_init(const eez_ui_config_t *config);

/**
 * @brief Create the main UI screen
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t eez_ui_create_screen(void);

/**
 * @brief Update WiFi scan results in the UI
 * 
 * @param networks Array of WiFi network information
 * @param count Number of networks
 * @return esp_err_t ESP_OK on success
 */
esp_err_t eez_ui_update_wifi_list(const wifi_ap_record_t *networks, uint16_t count);

/**
 * @brief Set status message in the UI
 * 
 * @param message Status message to display
 * @return esp_err_t ESP_OK on success
 */
esp_err_t eez_ui_set_status(const char *message);

/**
 * @brief Get the main screen object
 * 
 * @return lv_obj_t* Screen object or NULL if not initialized
 */
lv_obj_t* eez_ui_get_screen(void);

/**
 * @brief Register callback for scan button
 * 
 * @param callback Function to call when scan button is pressed
 * @return esp_err_t ESP_OK on success
 */
esp_err_t eez_ui_register_scan_callback(void (*callback)(void));

/**
 * @brief Register callback for network selection
 * 
 * @param callback Function to call when a network is selected
 * @return esp_err_t ESP_OK on success
 */
esp_err_t eez_ui_register_network_callback(void (*callback)(const char *ssid));

/**
 * @brief Deinitialize EEZ UI system
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t eez_ui_deinit(void);

#ifdef __cplusplus
}
#endif 