#include "eez_ui.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <string.h>

static const char *TAG = "EEZ_UI";

// Global variables
static eez_ui_config_t s_config;
static lv_obj_t *s_main_screen = NULL;
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_wifi_list = NULL;
static lv_obj_t *s_scan_button = NULL;

// Callback functions
static void (*s_scan_callback)(void) = NULL;
static void (*s_network_callback)(const char *ssid) = NULL;

/**
 * @brief Scan button event handler
 */
static void scan_button_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Scan button clicked");
        if (s_scan_callback) {
            s_scan_callback();
        }
    }
}

/**
 * @brief Network button event handler
 */
static void network_button_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *btn = lv_event_get_target(e);
        const char *ssid = lv_obj_get_user_data(btn);
        if (ssid && s_network_callback) {
            ESP_LOGI(TAG, "Network selected: %s", ssid);
            s_network_callback(ssid);
        }
    }
}

esp_err_t eez_ui_init(const eez_ui_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&s_config, config, sizeof(eez_ui_config_t));
    ESP_LOGI(TAG, "EEZ UI initialized");
    
    return ESP_OK;
}

esp_err_t eez_ui_create_screen(void)
{
    ESP_LOGI(TAG, "Creating EEZ UI screen");
    
    // Create main screen - use lv_scr_act() for compatibility
    s_main_screen = lv_scr_act();
    if (!s_main_screen) {
        ESP_LOGE(TAG, "Failed to get active screen");
        return ESP_FAIL;
    }
    
    // Set screen background
    lv_obj_set_style_bg_color(s_main_screen, lv_color_hex(0x000000), 0);
    
    // Create main container
    s_config.container = lv_obj_create(s_main_screen);
    lv_obj_set_size(s_config.container, 800, 480);
    lv_obj_center(s_config.container);
    lv_obj_set_style_bg_color(s_config.container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(s_config.container, 0, 0);
    
    // Create title
    lv_obj_t *title = lv_label_create(s_config.container);
    lv_label_set_text(title, "WiFi Scanner - ESP32-P4");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create scan button
    s_scan_button = lv_btn_create(s_config.container);
    lv_obj_set_size(s_scan_button, 120, 40);
    lv_obj_align(s_scan_button, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(s_scan_button, scan_button_event_handler, LV_EVENT_ALL, NULL);
    
    lv_obj_t *scan_btn_label = lv_label_create(s_scan_button);
    lv_label_set_text(scan_btn_label, "Scan");
    lv_obj_set_style_text_color(scan_btn_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(scan_btn_label);
    
    // Create status label
    s_status_label = lv_label_create(s_config.container);
    lv_label_set_text(s_status_label, "Ready to scan");
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_status_label, LV_ALIGN_TOP_LEFT, 10, 60);
    
    // Create scrollable list for WiFi networks
    s_wifi_list = lv_obj_create(s_config.container);
    lv_obj_set_size(s_wifi_list, 780, 380);
    lv_obj_align(s_wifi_list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(s_wifi_list, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(s_wifi_list, lv_color_hex(0x444444), 0);
    lv_obj_set_scroll_dir(s_wifi_list, LV_DIR_VER);
    
    // Set list layout
    lv_obj_set_layout(s_wifi_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_wifi_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_wifi_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(s_wifi_list, 5, 0);
    
    ESP_LOGI(TAG, "EEZ UI screen created successfully");
    return ESP_OK;
}

esp_err_t eez_ui_update_wifi_list(const wifi_ap_record_t *networks, uint16_t count)
{
    if (!s_wifi_list) {
        ESP_LOGE(TAG, "WiFi list not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Clear existing items
    lv_obj_clean(s_wifi_list);
    
    // Update status
    if (s_status_label) {
        lv_label_set_text_fmt(s_status_label, "Found %d WiFi networks", count);
    }
    
    // Add WiFi networks to list
    for (int i = 0; i < count; i++) {
        // Skip networks with empty SSID
        if (strlen((char*)networks[i].ssid) == 0) {
            continue;
        }
        
        lv_obj_t *btn = lv_btn_create(s_wifi_list);
        lv_obj_set_size(btn, 760, 60);
        lv_obj_set_style_radius(btn, 8, 0);
        
        // Store SSID as user data
        lv_obj_set_user_data(btn, (void*)networks[i].ssid);
        lv_obj_add_event_cb(btn, network_button_event_handler, LV_EVENT_ALL, NULL);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        
        const char *security_str = "Open";
        switch (networks[i].authmode) {
            case WIFI_AUTH_OPEN: security_str = "Open"; break;
            case WIFI_AUTH_WEP: security_str = "WEP"; break;
            case WIFI_AUTH_WPA_PSK: security_str = "WPA"; break;
            case WIFI_AUTH_WPA2_PSK: security_str = "WPA2"; break;
            case WIFI_AUTH_WPA_WPA2_PSK: security_str = "WPA/WPA2"; break;
            case WIFI_AUTH_WPA3_PSK: security_str = "WPA3"; break;
            default: security_str = "Unknown"; break;
        }
        
        lv_label_set_text_fmt(label, "%s\nCh: %d | RSSI: %ddBm | %s", 
                              (char*)networks[i].ssid, networks[i].primary, networks[i].rssi, security_str);
        lv_obj_center(label);
    }
    
    ESP_LOGI(TAG, "WiFi list updated with %d networks", count);
    return ESP_OK;
}

esp_err_t eez_ui_set_status(const char *message)
{
    if (!s_status_label) {
        ESP_LOGE(TAG, "Status label not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    lv_label_set_text(s_status_label, message);
    ESP_LOGI(TAG, "Status updated: %s", message);
    return ESP_OK;
}

lv_obj_t* eez_ui_get_screen(void)
{
    return s_main_screen;
}

esp_err_t eez_ui_register_scan_callback(void (*callback)(void))
{
    s_scan_callback = callback;
    ESP_LOGI(TAG, "Scan callback registered");
    return ESP_OK;
}

esp_err_t eez_ui_register_network_callback(void (*callback)(const char *ssid))
{
    s_network_callback = callback;
    ESP_LOGI(TAG, "Network callback registered");
    return ESP_OK;
}

esp_err_t eez_ui_deinit(void)
{
    s_main_screen = NULL;
    s_status_label = NULL;
    s_wifi_list = NULL;
    s_scan_button = NULL;
    s_scan_callback = NULL;
    s_network_callback = NULL;
    
    ESP_LOGI(TAG, "EEZ UI deinitialized");
    return ESP_OK;
} 