/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_hosted.h"
#include "esp_hosted_api_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
// LVGL with esp_lvgl_port and BSP
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_disp.h"
#include "driver/gpio.h"
#include "eez_ui.h"
#include "ui.h"  // Include EEZ Studio generated UI
#include "fpga_comms.h"
#include "machine_params.h"
#include "axis_mapping.h"
#include "dro_axis_helper.h"

// Initialize the USB Frontend Host Component
extern void init_frontend_host(void);


// Include BSP for ESP32-P4 board
#include "bsp/esp32_p4_function_ev_board.h"

// Does anyone use these? (LCD_H_RES, LCD_V_RES)
// #define LCD_H_RES              1280
// #define LCD_V_RES              800

// Forward declarations for callbacks
static void wifi_scan_callback(void);
static void network_selected_callback(const char *ssid);

// Forward declaration for WiFi initialization  
static esp_err_t wifi_init(void);
static void wifi_init_task(void *pvParameters);

// Forward declaration for UI tick task
static void ui_tick_task(void *pvParameters);
static void dro_update_task(void *pvParameters);
#include "dro_core.h"
#include "vars.h" // For set_var_virtual_axis_X

static const char *TAG = "WIFI_DEBUG";

/* WiFi scan result storage */
#define MAX_AP_COUNT 20
static wifi_ap_record_t ap_info[MAX_AP_COUNT];
static uint16_t ap_count = 0;

/* Event group for WiFi events */
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* Display handles */
static lv_display_t *lvgl_disp = NULL;


/**
 * @brief WiFi event handler with detailed logging
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                break;
            case WIFI_EVENT_STA_STOP:
                break;
            case WIFI_EVENT_STA_CONNECTED:
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                break;
            case WIFI_EVENT_SCAN_DONE:
                // NOTE: UI update is handled by scan task instead of event handler
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                break;
            case IP_EVENT_STA_LOST_IP:
                break;
            default:
                break;
        }
    }
}



/**
 * @brief Initialize WiFi with detailed logging and slave device validation
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
 */
static esp_err_t wifi_init(void)
{
    // Create event group
    wifi_event_group = xEventGroupCreate();
    
    // Initialize network interface
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Create default event loop
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Create default WiFi station
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    if (netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi station");
        return ESP_FAIL;
    }
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s (ESP-Hosted slave may not be responding)", esp_err_to_name(ret));
        ESP_LOGW(TAG, "WiFi unavailable - ESP32-C6 slave device not responding");
        return ESP_FAIL;
    }
    
    // Register event handler
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_handler_register (WIFI_EVENT) failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_handler_register (IP_EVENT) failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Set WiFi mode
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Start WiFi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "WiFi initialized successfully");
    return ESP_OK;
}

/**
 * @brief Print WiFi scan results to console
 */
static void print_wifi_scan_results(void)
{
    ESP_LOGI(TAG, "========== WiFi Scan Results ==========");
    ESP_LOGI(TAG, "Found %d access point(s):", ap_count);
    
    if (ap_count == 0) {
        ESP_LOGW(TAG, "No access points found");
        ESP_LOGI(TAG, "==========================================");
        return;
    }
    
    ESP_LOGI(TAG, "%-4s  %-32s  %-18s  %-6s  %s", 
             "No.", "SSID", "BSSID", "RSSI", "Channel");
    ESP_LOGI(TAG, "------------------------------------------------------------");
    
    for (uint16_t i = 0; i < ap_count; i++) {
        char ssid[33] = {0};
        char bssid[18] = {0};
        
        // Copy SSID (may be empty for hidden networks)
        if (ap_info[i].ssid[0] == 0) {
            snprintf(ssid, sizeof(ssid), "<hidden>");
        } else {
            snprintf(ssid, sizeof(ssid), "%s", (char *)ap_info[i].ssid);
        }
        
        // Format BSSID as MAC address
        snprintf(bssid, sizeof(bssid), "%02X:%02X:%02X:%02X:%02X:%02X",
                 ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
                 ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5]);
        
        ESP_LOGI(TAG, "%-4d  %-32s  %-18s  %-6d  %d", 
                 i + 1, ssid, bssid, ap_info[i].rssi, ap_info[i].primary);
    }
    
    ESP_LOGI(TAG, "==========================================");
}

/**
 * @brief Perform WiFi scan with detailed logging
 */
static void wifi_scan_task(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for WiFi to be ready
    
    // Verify WiFi is in correct state before scanning
    ESP_LOGI(TAG, "Ensuring WiFi is in Station Mode and Started...");
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_err_t start_ret = esp_wifi_start();
    if (start_ret != ESP_OK && start_ret != ESP_ERR_WIFI_MODE) {
        // ESP_ERR_WIFI_MODE means it's already started/active, which is fine
        ESP_LOGI(TAG, "esp_wifi_start result: %s", esp_err_to_name(start_ret));
    }

    // Configure scan
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 120,
                .max = 150
            }
        }
    };
    
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan start failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }
    
    // Get scan results
    // Get scan results
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    
    if (ap_count == 0) {
        ESP_LOGI(TAG, "No APs found during scan.");
    } else {
        if (ap_count > MAX_AP_COUNT) {
            ap_count = MAX_AP_COUNT;
        }

        err = esp_wifi_scan_get_ap_records(&ap_count, ap_info);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get scan results: %s", esp_err_to_name(err));
            ap_count = 0;
        }
    }
    
    // Print results
    print_wifi_scan_results();
    
    // Update UI with scan results using EEZ component
    eez_ui_update_wifi_list(ap_info, ap_count);
    
    vTaskDelete(NULL);
}

/**
 * @brief Task to create UI elements after LVGL is ready
 */
static void ui_creation_task(void *pvParameters)
{
    // Wait for LVGL to be fully initialized
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Lock LVGL for thread safety
    lvgl_port_lock(0);
    
    // Set display rotation to 270° landscape
    lv_display_set_rotation(lvgl_disp, LV_DISPLAY_ROTATION_270);
    
    // Initialize EEZ Studio generated UI
    ui_init();
    
    // Initialize DRO axis helpers after screens are created
    dro_axis_helper_init();
    
    // Initialize UI Events mappings (must be after ui_init)
    extern void dro_ui_events_init(void);
    dro_ui_events_init();
    
    // UI creation complete
    ESP_LOGI(TAG, "UI creation complete");
    
    // Unlock LVGL after all UI creation is complete
    lvgl_port_unlock();
    
    vTaskDelete(NULL);
}

/**
 * @brief Task to handle EEZ Studio UI tick
 */
static void ui_tick_task(void *pvParameters)
{
    // Wait for UI creation to complete
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    while (1) {
        // Lock LVGL for thread safety
        lvgl_port_lock(0);
        
        // Call EEZ Studio UI tick function
        ui_tick();
        
        // Unlock LVGL
        lvgl_port_unlock();
        
        // Tick every 8ms (125Hz) - stable rate
        vTaskDelay(pdMS_TO_TICKS(8));
    }
}

/**
 * @brief Monitor task to print system information
 */
static void system_monitor_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "=== System Monitor ===");
        ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "Min free heap: %lu bytes", esp_get_minimum_free_heap_size());
        ESP_LOGI(TAG, "Free internal heap: %lu bytes", esp_get_free_internal_heap_size());
        ESP_LOGI(TAG, "Task count: %d", uxTaskGetNumberOfTasks());
        
        // Display rotation is handled by BSP - no need to check or correct
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // Print every 10 seconds
    }
}

/**
 * @brief Initialize physical display using BSP
 */
static esp_err_t app_display_init(void)
{
    // Initialize BSP display - this handles LVGL port init and display creation
    lv_display_t *lv_disp = bsp_display_start();
    if (lv_disp != NULL) {
        lvgl_disp = lv_disp;
        
        // Turn on the display backlight
        esp_err_t backlight_ret = bsp_display_backlight_on();
        if (backlight_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to turn on display backlight: %s", esp_err_to_name(backlight_ret));
        }
        
        // Lock LVGL for thread safety if the port is running
        lvgl_port_lock(0);
        // Set initial rotation to 270° landscape
        lv_display_set_rotation(lvgl_disp, LV_DISPLAY_ROTATION_270);
        lvgl_port_unlock();
        
        ESP_LOGI(TAG, "Display initialized with LVGL software rotation");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "BSP display initialization failed");
        return ESP_FAIL;
    }
}

/**
 * @brief Initialize LVGL - BSP already created the display
 */
static esp_err_t app_lvgl_init(void)
{
    if (lvgl_disp == NULL) {
        ESP_LOGE(TAG, "No LVGL display available from BSP");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

void app_main(void)
{
    // Suppress cache subsystem error logs for non-fatal esp_cache_msync errors (error 103)
    // These occur when ESP-Hosted slave is not responding and are non-fatal
    esp_log_level_set("cache", ESP_LOG_WARN);
    
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* Initialize Display */
    ESP_ERROR_CHECK(app_display_init());
    
    /* Initialize LVGL */
    ESP_ERROR_CHECK(app_lvgl_init());

    /* Initialize Machine Parameters */
    machine_params_init();

    /* Initialize Axis Mapping */
    axis_mapping_init();

    /* Initialize FPGA Comms */
    // if (fpga_comms_init() != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to initialize FPGA Comms");
    // }
    
    /* Initialize DRO System */
    if (dro_init() == ESP_OK) {
        ESP_LOGI(TAG, "DRO System Initialized");
    } else {
        ESP_LOGE(TAG, "DRO System Init Failed");
    }

    /* Initialize USB Frontend Host (Logging Only for now) */
    init_frontend_host();

    /* Create tasks */

    // Create UI creation task (higher priority to run first)
    xTaskCreate(ui_creation_task, "ui_creation", 4096, NULL, 5, NULL);
    
    // Create UI tick task for EEZ Studio (slightly higher priority)
    xTaskCreate(ui_tick_task, "ui_tick", 4096, NULL, 5, NULL);
    
    // Create system monitor task
    // xTaskCreate(system_monitor_task, "system_monitor", 4096, NULL, 1, NULL);
    
    // Create delayed WiFi initialization task with slave device health checks
    xTaskCreate(wifi_init_task, "wifi_init", 4096, NULL, 2, NULL);




    // Create DRO Update Task
    xTaskCreate(dro_update_task, "dro_update", 4096, NULL, 5, NULL);
}

/**
 * @brief Task to update DRO state and UI
 */
static void dro_update_task(void *pvParameters)
{
    // Wait for UI to be ready
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        // Update DRO Internal State
        dro_update();

        // Update UI Variables (Thread Safe)
        lvgl_port_lock(0);
        
        const dro_system_state_t* state = dro_get_state();
        if (state && state->is_initialized) {
            set_var_virtual_axis_1(state->axes[DRO_AXIS_X].displayed_value);
            set_var_virtual_axis_2(state->axes[DRO_AXIS_Y].displayed_value);
            set_var_virtual_axis_3(state->axes[DRO_AXIS_Z].displayed_value);
            set_var_virtual_axis_4(state->axes[DRO_AXIS_W].displayed_value);
            set_var_virtual_axis_5(state->axes[DRO_AXIS_C].displayed_value);
        
            // Update Unit Labels - REMOVED (Handled in screens.c tick_screen_main)
            // const char* unit_str = (state->current_unit == DRO_UNIT_MM) ? "mm" : "in";
            // if (objects.mm_x_axis1_label) lv_label_set_text(objects.mm_x_axis1_label, unit_str);
            // if (objects.mm_x_axis2_label) lv_label_set_text(objects.mm_x_axis2_label, unit_str);
            // if (objects.mm_x_axis3_label) lv_label_set_text(objects.mm_x_axis3_label, unit_str);
            // if (objects.mm_x_axis4_label) lv_label_set_text(objects.mm_x_axis4_label, unit_str);
            // if (objects.mm_x_axis5_label) lv_label_set_text(objects.mm_x_axis5_label, unit_str);
            
            // Optional: Update ABS/INC button label if it existed as a label, but it's a toggle button usually
            
            // Update Active Tool and Space Displays via variable setters (EEZ UI handles the sync)
            set_var_active_tool_number(state->active_tool_index);
            set_var_active_space_number(state->active_space_index);
        }
        
        lvgl_port_unlock();

        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz update rate
    }
}

/**
 * @brief Delayed WiFi initialization task with slave device health checks
 */
static void wifi_init_task(void *pvParameters)
{
    // Wait for other components to fully initialize (5 seconds delay)
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    ESP_LOGI(TAG, "Starting delayed WiFi initialization...");

    /* Initialize WiFi */
    esp_err_t ret = wifi_init();
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi initialized successfully, starting scan task...");
        // Wait a bit more before starting scan
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Create WiFi scan task only if WiFi init succeeded
        xTaskCreate(wifi_scan_task, "wifi_scan", 4096, NULL, 2, NULL);
    } else {
        ESP_LOGW(TAG, "WiFi initialization failed - ESP32-C6 slave may not be connected");
        ESP_LOGW(TAG, "System will continue to operate without WiFi functionality");
    }
    
    ESP_LOGI(TAG, "WiFi initialization task complete");
    vTaskDelete(NULL);
}

/**
 * @brief WiFi scan callback for EEZ UI
 */
static void wifi_scan_callback(void)
{
    eez_ui_set_status("Scanning...");
    esp_wifi_scan_start(NULL, false);
}

/**
 * @brief Network selection callback for EEZ UI
 */
static void network_selected_callback(const char *ssid)
{
    // TODO: Implement network connection logic
    eez_ui_set_status("Selected network - connection not implemented yet");
}
