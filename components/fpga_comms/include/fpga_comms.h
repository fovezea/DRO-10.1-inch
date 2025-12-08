#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// Helper to access members
typedef struct {
    uint8_t work_mode;  // 0=Screw, 1=Follow, 2=Conical
    bool is_enabled;
    uint32_t current_rpm;
    int32_t current_position;
    bool is_connected;  // Heartbeat received recently
    uint8_t status_flags;
} fpga_state_t;

/**
 * @brief Initialize the FPGA communication component
 * 
 * Configures UART and starts the communication task.
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fpga_comms_init(void);

/**
 * @brief Send a command to the FPGA
 * 
 * Queues a command to be sent.
 * @param cmd_id Command ID
 * @param payload Pointer to payload data (can be NULL)
 * @param len Length of payload
 * @return esp_err_t ESP_OK if queued
 */
esp_err_t fpga_comms_send_cmd(uint8_t cmd_id, const uint8_t *payload, uint8_t len);

/**
 * @brief Get a copy of the current FPGA state
 * 
 * Thread-safe access to the state store.
 * @param state_out Pointer to struct to fill
 */
void fpga_comms_get_state(fpga_state_t *state_out);
