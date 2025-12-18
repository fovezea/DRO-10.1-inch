#include "dro_nvs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "dro_core.h" // For AXIS_COUNT

static const char *TAG = "DRO_NVS";

esp_err_t dro_nvs_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static esp_err_t dro_nvs_open(nvs_handle_t *handle, nvs_open_mode_t mode) {
    return nvs_open(DRO_NVS_NAMESPACE, mode, handle);
}

esp_err_t dro_nvs_save_param(const char* key, int32_t value) {
    nvs_handle_t handle;
    esp_err_t err = dro_nvs_open(&handle, NVS_READWRITE);
    if (err != ESP_OK) return err;

    err = nvs_set_i32(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t dro_nvs_load_param(const char* key, int32_t* value, int32_t default_value) {
    nvs_handle_t handle;
    esp_err_t err = dro_nvs_open(&handle, NVS_READONLY);
    if (err != ESP_OK) {
        *value = default_value;
        return ESP_OK; // Return default if NVS failed to open (e.g. first run)
    }

    err = nvs_get_i32(handle, key, value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *value = default_value;
        err = ESP_OK;
    }
    nvs_close(handle);
    return err;
}

esp_err_t dro_nvs_save_axis_offset(uint8_t axis_index, float offset) {
    char key[16];
    snprintf(key, sizeof(key), "%s%d", DRO_KEY_OFFSET_PREFIX, axis_index);
    
    nvs_handle_t handle;
    esp_err_t err = dro_nvs_open(&handle, NVS_READWRITE);
    if (err != ESP_OK) return err;

    // NVS doesn't support float directly, save as blob or convert
    // We will save as blob for simplicity
    err = nvs_set_blob(handle, key, &offset, sizeof(float));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t dro_nvs_load_axis_offset(uint8_t axis_index, float* offset) {
    char key[16];
    snprintf(key, sizeof(key), "%s%d", DRO_KEY_OFFSET_PREFIX, axis_index);

    nvs_handle_t handle;
    esp_err_t err = dro_nvs_open(&handle, NVS_READONLY);
    if (err != ESP_OK) {
        *offset = 0.0f;
        return ESP_OK;
    }

    size_t required_size = sizeof(float);
    err = nvs_get_blob(handle, key, offset, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *offset = 0.0f;
        err = ESP_OK;
    }
    nvs_close(handle);
    return err;
}

esp_err_t dro_nvs_save_tool(uint8_t tool_index, const void* data, size_t size) {
    char key[16];
    // Key format: "tool_N"
    snprintf(key, sizeof(key), "tool_%d", tool_index);
    
    nvs_handle_t handle;
    esp_err_t err = dro_nvs_open(&handle, NVS_READWRITE);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, key, data, size);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t dro_nvs_load_tool(uint8_t tool_index, void* data, size_t size) {
    char key[16];
    snprintf(key, sizeof(key), "tool_%d", tool_index);

    nvs_handle_t handle;
    esp_err_t err = dro_nvs_open(&handle, NVS_READONLY);
    if (err != ESP_OK) return err; // Caller handles empty/default

    size_t required_size = size;
    err = nvs_get_blob(handle, key, data, &required_size);
    nvs_close(handle);
    return err;
}
