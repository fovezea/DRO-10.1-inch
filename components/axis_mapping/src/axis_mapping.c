#include "axis_mapping.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

#define TAG "AXIS_MAPPING"
#define NVS_NAMESPACE "axis_map"

// Cached values
static uint8_t g_z_axis_mapping = DEFAULT_Z_AXIS_MAPPING;
static uint8_t g_x_axis_mapping = DEFAULT_X_AXIS_MAPPING;
static char g_axis_names[NUM_VIRTUAL_AXES][AXIS_NAME_MAX_LEN + 1];

// Default names array
static const char *default_names[NUM_VIRTUAL_AXES] = {
    DEFAULT_AXIS_0_NAME,
    DEFAULT_AXIS_1_NAME,
    DEFAULT_AXIS_2_NAME,
    DEFAULT_AXIS_3_NAME,
    DEFAULT_AXIS_4_NAME
};

esp_err_t axis_mapping_init(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read Z-axis mapping
    err = nvs_get_u8(nvs_handle, "z_axis", &g_z_axis_mapping);
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "Z-axis mapping not found, using default: %d", DEFAULT_Z_AXIS_MAPPING);
        g_z_axis_mapping = DEFAULT_Z_AXIS_MAPPING;
    }
    
    // Read X-axis mapping
    err = nvs_get_u8(nvs_handle, "x_axis", &g_x_axis_mapping);
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "X-axis mapping not found, using default: %d", DEFAULT_X_AXIS_MAPPING);
        g_x_axis_mapping = DEFAULT_X_AXIS_MAPPING;
    }
    
    // Read axis names
    for (uint8_t i = 0; i < NUM_VIRTUAL_AXES; i++) {
        char key[8];
        snprintf(key, sizeof(key), "name_%d", i);
        
        size_t required_size = AXIS_NAME_MAX_LEN + 1;
        err = nvs_get_str(nvs_handle, key, g_axis_names[i], &required_size);
        
        if (err != ESP_OK) {
            // Use default name
            strncpy(g_axis_names[i], default_names[i], AXIS_NAME_MAX_LEN);
            g_axis_names[i][AXIS_NAME_MAX_LEN] = '\0';
            ESP_LOGD(TAG, "Axis %d name not found, using default: %s", i, g_axis_names[i]);
        }
    }
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Axis mapping loaded - Z:%d, X:%d", g_z_axis_mapping, g_x_axis_mapping);
    for (uint8_t i = 0; i < NUM_VIRTUAL_AXES; i++) {
        ESP_LOGI(TAG, "Axis %d name: %s", i, g_axis_names[i]);
    }
    
    return ESP_OK;
}

uint8_t axis_mapping_get_z_axis(void) {
    return g_z_axis_mapping;
}

esp_err_t axis_mapping_set_z_axis(uint8_t virtual_axis) {
    if (virtual_axis >= NUM_VIRTUAL_AXES) {
        ESP_LOGE(TAG, "Invalid virtual axis: %d", virtual_axis);
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    
    err = nvs_set_u8(nvs_handle, "z_axis", virtual_axis);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            g_z_axis_mapping = virtual_axis;
            ESP_LOGI(TAG, "Z-axis mapping set to %d", virtual_axis);
        }
    }
    
    nvs_close(nvs_handle);
    return err;
}

uint8_t axis_mapping_get_x_axis(void) {
    return g_x_axis_mapping;
}

esp_err_t axis_mapping_set_x_axis(uint8_t virtual_axis) {
    if (virtual_axis >= NUM_VIRTUAL_AXES) {
        ESP_LOGE(TAG, "Invalid virtual axis: %d", virtual_axis);
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    
    err = nvs_set_u8(nvs_handle, "x_axis", virtual_axis);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            g_x_axis_mapping = virtual_axis;
            ESP_LOGI(TAG, "X-axis mapping set to %d", virtual_axis);
        }
    }
    
    nvs_close(nvs_handle);
    return err;
}

const char* axis_mapping_get_axis_name(uint8_t virtual_axis) {
    if (virtual_axis >= NUM_VIRTUAL_AXES) {
        ESP_LOGE(TAG, "Invalid virtual axis: %d", virtual_axis);
        return "Invalid";
    }
    
    return g_axis_names[virtual_axis];
}

esp_err_t axis_mapping_set_axis_name(uint8_t virtual_axis, const char *name) {
    if (virtual_axis >= NUM_VIRTUAL_AXES) {
        ESP_LOGE(TAG, "Invalid virtual axis: %d", virtual_axis);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (name == NULL || strlen(name) == 0) {
        ESP_LOGE(TAG, "Invalid axis name");
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    
    char key[8];
    snprintf(key, sizeof(key), "name_%d", virtual_axis);
    
    err = nvs_set_str(nvs_handle, key, name);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            strncpy(g_axis_names[virtual_axis], name, AXIS_NAME_MAX_LEN);
            g_axis_names[virtual_axis][AXIS_NAME_MAX_LEN] = '\0';
            ESP_LOGI(TAG, "Axis %d name set to: %s", virtual_axis, g_axis_names[virtual_axis]);
        }
    }
    
    nvs_close(nvs_handle);
    return err;
}
