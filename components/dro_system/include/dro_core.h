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
#define DRO_MAX_WORKSPACES 200

typedef enum {
    DRO_UNIT_MM = 0,
    DRO_UNIT_INCH
} dro_unit_t;

typedef enum {
    DRO_MODE_ABS = 0,   // Absolute (Machine Zero)
    DRO_MODE_INC        // Incremental (Work/Relative Zero)
} dro_mode_t;

typedef enum {
    DRO_AXIS_TYPE_LINEAR = 0,
    DRO_AXIS_TYPE_ROTARY
} dro_axis_type_t;

typedef enum {
    DRO_MACHINE_TYPE_LATHE = 0,
    DRO_MACHINE_TYPE_MILL,
    DRO_MACHINE_TYPE_GRINDER,
    DRO_MACHINE_TYPE_EDM
} dro_machine_type_t;

typedef struct {
    bool enabled;
    dro_axis_type_t type;
    float pulses_per_unit;  // Pulses per mm (or inch? Usually native unit is mm)
    float gear_ratio;       // 1.0 = direct
    float leadscrew_pitch;  // mm per rev
    bool inverted;
    bool is_spindle_readout; // NEW: If true, ignore USB axis counts and track spindle directly
    uint8_t reserved[7];    // Future proofing for NVS
} dro_axis_config_t;

typedef struct {
    float raw_counts;       // CHANGED: Now storing raw counts from encoder
    float position_mm;      // Caclulated position in mm (base unit)
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
    char name[16];
    float offsets[DRO_AXIS_COUNT];
} dro_workspace_t;

typedef struct {
    dro_unit_t current_unit;
    dro_mode_t current_mode;
    dro_axis_config_t axis_configs[DRO_AXIS_COUNT]; // Added config
    dro_axis_state_t axes[DRO_AXIS_COUNT];
    
    // SPINDLE STATE
    int32_t current_spindle_counts;
    float current_spindle_rpm;
    
    int32_t active_tool_index;
    int32_t active_space_index;
    bool high_precision;
    dro_machine_type_t machine_type; // New Setting
    uint8_t active_axis_count;       // New Setting (1 to DRO_AXIS_COUNT)
    bool is_initialized;
} dro_system_state_t;

/**
 * @brief Set and save axis configuration (runtime update)
 */
void dro_set_axis_config(uint8_t axis_index, dro_axis_config_t config);

/**
 * @brief Initialize the DRO system and load state from NVS
 */
esp_err_t dro_init(void);

/**
 * @brief Get current decimal precision based on units and settings
 */
int dro_get_precision(void);

/**
 * @brief Set high precision mode (4 decimals for MM, 5 for INCH)
 */
void dro_set_high_precision(bool enabled);

/**
 * @brief Set machine type (Lathe, Mill, etc.)
 */
void dro_set_machine_type(dro_machine_type_t type);

/**
 * @brief Set number of active axes
 */
void dro_set_active_axis_count(uint8_t count);

/**
 * @brief Main tick function to calculate positions
 * Used to update displayed values based on current raw inputs
 */
void dro_update(void);

// --- Global Actions ---

/**
 * @brief Get the unit name string for a specific axis (e.g., "mm", "inch", "deg")
 */
const char* dro_get_axis_unit_name(uint8_t axis_index);

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
 * @brief Update the raw physical counts of an axis (from encoders)
 * This will trigger a recalculation of the position based on axis config (puls/unit, gear, etc.)
 */
void dro_set_raw_counts(uint8_t axis_index, int32_t counts);

/**
 * @brief Push telemetry from the Backend Spindle
 * Will automatically route raw counts to any Axis configured with is_spindle_readout=true
 */
void dro_set_spindle_telemetry(int32_t counts, float rpm);

/**
 * @brief DEPRECATED: Use dro_set_raw_counts instead.
 * Kept for basic compatibility during migration.
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


// --- Workspace Management ---

/**
 * @brief Get workspace data from NVS
 */
esp_err_t dro_workspace_get(uint8_t space_index, dro_workspace_t* space_out);

/**
 * @brief Save workspace data to NVS
 */
esp_err_t dro_workspace_set(uint8_t space_index, const dro_workspace_t* space_in);

/**
 * @brief Apply a workspace's offsets to the current system
 */
esp_err_t dro_workspace_apply(int32_t space_index);

/**
 * @brief Get the currently active workspace index
 */
int32_t dro_workspace_get_active_index(void);


#ifdef __cplusplus
}
#endif
