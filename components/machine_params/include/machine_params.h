#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// Default machine parameters
#define DEFAULT_LEADSCREW_PITCH_MM      2.0f    // 2mm pitch leadscrew
#define DEFAULT_MOTOR_STEPS_PER_REV     1600    // 200 steps * 8 microsteps
#define DEFAULT_ENCODER_COUNTS_PER_REV  2400    // 600 PPR * 4 (quadrature)

/**
 * @brief Initialize machine parameters from NVS
 * 
 * Loads parameters from NVS or uses defaults if not set
 * @return esp_err_t ESP_OK on success
 */
esp_err_t machine_params_init(void);

/**
 * @brief Get leadscrew pitch in mm
 */
float machine_params_get_leadscrew_pitch(void);

/**
 * @brief Set leadscrew pitch in mm
 */
esp_err_t machine_params_set_leadscrew_pitch(float pitch_mm);

/**
 * @brief Get motor steps per revolution
 */
uint32_t machine_params_get_motor_steps(void);

/**
 * @brief Set motor steps per revolution
 */
esp_err_t machine_params_set_motor_steps(uint32_t steps);

/**
 * @brief Get encoder counts per revolution
 */
uint32_t machine_params_get_encoder_counts(void);

/**
 * @brief Set encoder counts per revolution
 */
esp_err_t machine_params_set_encoder_counts(uint32_t counts);

/**
 * @brief Get whether Electronic Leadscrew (ELS) hardware is present
 */
bool machine_params_get_is_els_present(void);

/**
 * @brief Set whether Electronic Leadscrew (ELS) hardware is present
 */
esp_err_t machine_params_set_is_els_present(bool present);
