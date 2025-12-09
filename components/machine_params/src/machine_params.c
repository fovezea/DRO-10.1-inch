#include "machine_params.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

#define TAG "MACHINE_PARAMS"
#define NVS_NAMESPACE "machine"

// Cached parameters
static float g_leadscrew_pitch = DEFAULT_LEADSCREW_PITCH_MM;
static uint32_t g_motor_steps = DEFAULT_MOTOR_STEPS_PER_REV;
static uint32_t g_encoder_counts = DEFAULT_ENCODER_COUNTS_PER_REV;

esp_err_t machine_params_init(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read leadscrew pitch
    uint32_t pitch_int;  // Store as integer (pitch * 1000)
    err = nvs_get_u32(nvs_handle, "ls_pitch", &pitch_int);
    if (err == ESP_OK) {
        g_leadscrew_pitch = pitch_int / 1000.0f;
    } else {
        ESP_LOGW(TAG, "Leadscrew pitch not found, using default: %.2f mm", DEFAULT_LEADSCREW_PITCH_MM);
        g_leadscrew_pitch = DEFAULT_LEADSCREW_PITCH_MM;
    }
    
    // Read motor steps
    err = nvs_get_u32(nvs_handle, "motor_steps", &g_motor_steps);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Motor steps not found, using default: %lu", DEFAULT_MOTOR_STEPS_PER_REV);
        g_motor_steps = DEFAULT_MOTOR_STEPS_PER_REV;
    }
    
    // Read encoder counts
    err = nvs_get_u32(nvs_handle, "enc_counts", &g_encoder_counts);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Encoder counts not found, using default: %lu", DEFAULT_ENCODER_COUNTS_PER_REV);
        g_encoder_counts = DEFAULT_ENCODER_COUNTS_PER_REV;
    }
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Machine params loaded: Pitch=%.2fmm, Steps=%lu, Encoder=%lu", 
             g_leadscrew_pitch, g_motor_steps, g_encoder_counts);
    
    return ESP_OK;
}

float machine_params_get_leadscrew_pitch(void) {
    return g_leadscrew_pitch;
}

esp_err_t machine_params_set_leadscrew_pitch(float pitch_mm) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    
    uint32_t pitch_int = (uint32_t)(pitch_mm * 1000.0f);
    err = nvs_set_u32(nvs_handle, "ls_pitch", pitch_int);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            g_leadscrew_pitch = pitch_mm;
            ESP_LOGI(TAG, "Leadscrew pitch set to %.2f mm", pitch_mm);
        }
    }
    
    nvs_close(nvs_handle);
    return err;
}

uint32_t machine_params_get_motor_steps(void) {
    return g_motor_steps;
}

esp_err_t machine_params_set_motor_steps(uint32_t steps) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    
    err = nvs_set_u32(nvs_handle, "motor_steps", steps);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            g_motor_steps = steps;
            ESP_LOGI(TAG, "Motor steps set to %lu", steps);
        }
    }
    
    nvs_close(nvs_handle);
    return err;
}

uint32_t machine_params_get_encoder_counts(void) {
    return g_encoder_counts;
}

esp_err_t machine_params_set_encoder_counts(uint32_t counts) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    
    err = nvs_set_u32(nvs_handle, "enc_counts", counts);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            g_encoder_counts = counts;
            ESP_LOGI(TAG, "Encoder counts set to %lu", counts);
        }
    }
    
    nvs_close(nvs_handle);
    return err;
}
