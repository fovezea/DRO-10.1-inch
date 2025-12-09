#pragma once

#include "esp_err.h"
#include <stdint.h>

#define AXIS_NAME_MAX_LEN 15
#define NUM_VIRTUAL_AXES 5

// Default axis names
#define DEFAULT_AXIS_0_NAME "X-Axis"
#define DEFAULT_AXIS_1_NAME "Y-Axis"
#define DEFAULT_AXIS_2_NAME "Z-Axis"
#define DEFAULT_AXIS_3_NAME "C-Axis"
#define DEFAULT_AXIS_4_NAME "W-Axis"

// Default mappings
#define DEFAULT_Z_AXIS_MAPPING 2  // Z-Axis
#define DEFAULT_X_AXIS_MAPPING 0  // X-Axis

/**
 * @brief Initialize axis mapping from NVS
 * 
 * Loads mappings and custom names from NVS, or uses defaults if not set
 * @return esp_err_t ESP_OK on success
 */
esp_err_t axis_mapping_init(void);

/**
 * @brief Get Z-axis (longitudinal) virtual axis mapping
 * @return uint8_t Virtual axis index (0-4)
 */
uint8_t axis_mapping_get_z_axis(void);

/**
 * @brief Set Z-axis (longitudinal) virtual axis mapping
 * @param virtual_axis Virtual axis index (0-4)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t axis_mapping_set_z_axis(uint8_t virtual_axis);

/**
 * @brief Get X-axis (cross-slide) virtual axis mapping
 * @return uint8_t Virtual axis index (0-4)
 */
uint8_t axis_mapping_get_x_axis(void);

/**
 * @brief Set X-axis (cross-slide) virtual axis mapping
 * @param virtual_axis Virtual axis index (0-4)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t axis_mapping_set_x_axis(uint8_t virtual_axis);

/**
 * @brief Get custom name for a virtual axis
 * @param virtual_axis Virtual axis index (0-4)
 * @return const char* Custom axis name (pointer to internal buffer)
 */
const char* axis_mapping_get_axis_name(uint8_t virtual_axis);

/**
 * @brief Set custom name for a virtual axis
 * @param virtual_axis Virtual axis index (0-4)
 * @param name Custom name (max 15 chars, will be truncated if longer)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t axis_mapping_set_axis_name(uint8_t virtual_axis, const char *name);
