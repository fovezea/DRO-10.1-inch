#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// NVS Namespace
#define DRO_NVS_NAMESPACE "dro_config"

// Keys
#define DRO_KEY_UNITS "units"
#define DRO_KEY_MODE  "mode"
#define DRO_KEY_TOOL  "active_tool"
#define DRO_KEY_SPACE "active_space"
#define DRO_KEY_OFFSET_PREFIX "off_" // + axis_index

/**
 * @brief Initialize NVS for DRO
 */
esp_err_t dro_nvs_init(void);

/**
 * @brief Save a system parameter (units, mode)
 */
esp_err_t dro_nvs_save_param(const char* key, int32_t value);

/**
 * @brief Load a system parameter
 */
esp_err_t dro_nvs_load_param(const char* key, int32_t* value, int32_t default_value);

/**
 * @brief Save axis workspace offset
 */
esp_err_t dro_nvs_save_axis_offset(uint8_t axis_index, float offset);

/**
 * @brief Load axis workspace offset
 */
esp_err_t dro_nvs_load_axis_offset(uint8_t axis_index, float* offset);

/**
 * @brief Save tool data (blob)
 */
esp_err_t dro_nvs_save_tool(uint8_t tool_index, const void* data, size_t size);

/**
 * @brief Load tool data (blob)
 */
esp_err_t dro_nvs_load_tool(uint8_t tool_index, void* data, size_t size);

/**
 * @brief Save workspace data (blob)
 */
esp_err_t dro_nvs_save_workspace(uint8_t space_index, const void* data, size_t size);

/**
 * @brief Load workspace data (blob)
 */
esp_err_t dro_nvs_load_workspace(uint8_t space_index, void* data, size_t size);

#ifdef __cplusplus
}
#endif
