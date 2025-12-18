#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define DRO_AXIS_COUNT 5
#define DRO_AXIS_X 0
#define DRO_AXIS_Y 1
#define DRO_AXIS_Z 2
#define DRO_AXIS_W 3
#define DRO_AXIS_C 4

#define DRO_MAX_TOOLS 200

typedef enum {
    DRO_UNIT_MM = 0,
    DRO_UNIT_INCH
} dro_unit_t;

typedef enum {
    DRO_MODE_ABS = 0,   // Absolute (Machine Zero)
    DRO_MODE_INC        // Incremental (Work/Relative Zero)
} dro_mode_t;

typedef struct {
    float raw_position;     // Position from encoders (always mm)
    float work_offset;      // Zero point offset
    float tool_offset;      // Tool length offset
    float displayed_value;  // Final calculated value for UI
} dro_axis_state_t;

typedef struct {
    char name[16];
    float offsets[DRO_AXIS_COUNT]; // Offsets for X, Y, Z, W, C
    uint8_t type;                  // Reserved for future use (Drill, Turn, etc.)
} dro_tool_t;

typedef struct {
    dro_unit_t current_unit;
    dro_mode_t current_mode;
    dro_axis_state_t axes[DRO_AXIS_COUNT];
    int32_t active_tool_index;
    bool is_initialized;
} dro_system_state_t;

/**
 * @brief Initialize the DRO system and load state from NVS
 */
esp_err_t dro_init(void);

/**
 * @brief Main tick function to calculate positions
 * Used to update displayed values based on current raw inputs
 */
void dro_update(void);

// --- Global Actions ---

/**
 * @brief Toggle between MM and INCH
 */
void dro_toggle_units(void);

/**
 * @brief Toggle between ABS and INC modes
 */
void dro_toggle_mode(void);

/**
 * @brief Get global system state
 */
const dro_system_state_t* dro_get_state(void);


// --- Axis Actions ---

/**
 * @brief Update the raw physical position of an axis (from encoders)
 */
void dro_set_raw_position(uint8_t axis_index, float position_mm);

/**
 * @brief Set the current position to 0 (modifies Work Offset)
 */
void dro_axis_zero(uint8_t axis_index);

/**
 * @brief Set the current position to a specific value
 */
void dro_axis_set_value(uint8_t axis_index, float value);

/**
 * @brief Halve the current value (Center finding)
 */
void dro_axis_half(uint8_t axis_index);


// --- Tool Management ---

/**
 * @brief Get tool data from library (cached or NVS)
 */
esp_err_t dro_tool_get(uint8_t tool_index, dro_tool_t* tool_out);

/**
 * @brief Save tool data to library
 */
esp_err_t dro_tool_set(uint8_t tool_index, const dro_tool_t* tool_in);

/**
 * @brief Apply a tool's offsets to the current system
 * This updates the 'tool_offset' of each axis based on the tool's definition.
 */
esp_err_t dro_tool_apply(int32_t tool_index);

/**
 * @brief Get the currently active tool index
 */
int32_t dro_tool_get_active_index(void);


#ifdef __cplusplus
}
#endif
