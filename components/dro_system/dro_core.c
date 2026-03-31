#include "dro_core.h"
#include "dro_nvs.h"
#include "nvs.h" // Required for ESP_ERR_NVS_NOT_FOUND
#include "esp_log.h"
#include <string.h>

static const char *TAG = "DRO_CORE";
static dro_system_state_t system_state;

esp_err_t dro_init(void) {
    ESP_LOGI(TAG, "Initializing DRO System...");
    
    // Initialize NVS
    esp_err_t err = dro_nvs_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init NVS: %d", err);
        return err;
    }

    // Load Global Settings
    int32_t val;
    dro_nvs_load_param(DRO_KEY_UNITS, &val, DRO_UNIT_MM);
    system_state.current_unit = (dro_unit_t)val;

    dro_nvs_load_param(DRO_KEY_MODE, &val, DRO_MODE_ABS);
    system_state.current_mode = (dro_mode_t)val;

    // Load Axis States
    for (int i = 0; i < DRO_AXIS_COUNT; i++) {
        system_state.axes[i].raw_counts = 0.0f;
        system_state.axes[i].position_mm = 0.0f;
        
        // Load Config from NVS
        esp_err_t err = dro_nvs_load_axis_config(i, &system_state.axis_configs[i], sizeof(dro_axis_config_t));
        
        if (err != ESP_OK) {
            // Default Config (Safety defaults if not found)
            ESP_LOGW(TAG, "Axis %d config not found, using defaults", i);
            system_state.axis_configs[i].enabled = true;
            system_state.axis_configs[i].type = DRO_AXIS_TYPE_LINEAR;
            system_state.axis_configs[i].pulses_per_unit = 200.0f; 
            system_state.axis_configs[i].gear_ratio = 1.0f;
            system_state.axis_configs[i].inverted = false;
            system_state.axis_configs[i].is_spindle_readout = false;
            
            // Save default to NVS so it exists next time
            dro_nvs_save_axis_config(i, &system_state.axis_configs[i], sizeof(dro_axis_config_t));
        }
    }

    // Load and Apply Active Tool
    int32_t active_tool_idx = 0;
    dro_nvs_load_param(DRO_KEY_TOOL, &active_tool_idx, 0);
    dro_tool_apply(active_tool_idx);

    // Load and Apply Active Workspace (Space)
    int32_t active_space_idx = 0;
    dro_nvs_load_param(DRO_KEY_SPACE, &active_space_idx, 0);
    dro_workspace_apply(active_space_idx);

    // Initial precision settings
    system_state.high_precision = false;
    
    // Default Machine settings
    system_state.machine_type = DRO_MACHINE_TYPE_MILL;
    system_state.active_axis_count = 3;

    system_state.is_initialized = true;
    ESP_LOGI(TAG, "DRO System Initialized. Units: %d, Mode: %d", system_state.current_unit, system_state.current_mode);
    return ESP_OK;
}

const dro_system_state_t* dro_get_state(void) {
    return &system_state;
}

int dro_get_precision(void) {
    if (system_state.current_unit == DRO_UNIT_MM) {
        return system_state.high_precision ? 4 : 3;
    } else {

        return system_state.high_precision ? 5 : 4;
    }
}

void dro_set_high_precision(bool enabled) {
    system_state.high_precision = enabled;
    // dro_nvs_save_param(DRO_KEY_PRECISION, enabled ? 1 : 0); // TODO: Add key if persistence needed
    ESP_LOGI(TAG, "High Precision Set: %d", enabled);
}

void dro_set_machine_type(dro_machine_type_t type) {
    system_state.machine_type = type;
    // dro_nvs_save_param(DRO_KEY_MACHINE_TYPE, (int32_t)type); // TODO: Add key
    ESP_LOGI(TAG, "Machine Type Set: %d", type);
}

void dro_set_active_axis_count(uint8_t count) {
    if (count > DRO_AXIS_COUNT) count = DRO_AXIS_COUNT;
    if (count < 1) count = 1;
    
    system_state.active_axis_count = count;
    // dro_nvs_save_param(DRO_KEY_AXIS_COUNT, (int32_t)count); // TODO: Add key
    ESP_LOGI(TAG, "Active Axis Count Set: %d", count);
}

const char* dro_get_axis_unit_name(uint8_t axis_index) {
    if (axis_index >= DRO_AXIS_COUNT) return "";
    
    if (system_state.axis_configs[axis_index].type == DRO_AXIS_TYPE_ROTARY) {
        return "deg";
    }
    
    return (system_state.current_unit == DRO_UNIT_INCH) ? "inch" : "mm";
}

void dro_update(void) {
    if (!system_state.is_initialized) return;

    for (int i = 0; i < DRO_AXIS_COUNT; i++) {
        // Skip disabled axes
        if (!system_state.axis_configs[i].enabled) {
            system_state.axes[i].displayed_value = 0.0f;
            continue;
        }

        // 1. Calculate Physical Position (MM) from Raw Counts
        // Formula: Pos = (Counts / PPU) * Gear * Sign
        float ppu = system_state.axis_configs[i].pulses_per_unit;
        if (ppu == 0.0f) ppu = 1.0f; // Safety
        
        float base_pos = (system_state.axes[i].raw_counts / ppu) * system_state.axis_configs[i].gear_ratio;
        
        if (system_state.axis_configs[i].inverted) {
            base_pos = -base_pos;
        }
        
        system_state.axes[i].position_mm = base_pos;

        // 2. Apply Offsets (Work Coordinates)
        float result_mm = 0.0f;
        
        if (system_state.current_mode == DRO_MODE_ABS) {
            // Absolute: Raw Machine Coords
            result_mm = system_state.axes[i].position_mm;
        } else {
            // Incremental: Apply Work & Tool Offsets
            // Result = Raw - WorkOffset - ToolOffset
            result_mm = system_state.axes[i].position_mm - 
                       system_state.axes[i].work_offset - 
                       system_state.axes[i].tool_offset;
        }

        // 3. Unit Conversion (for Display)
        // If Rotary, we usually stay in Degrees (base unit for rotary is assumed Deg not mm if configured properly?)
        // Let's assume for Rotary, the "MM" position is actually "Degrees".
        if (system_state.axis_configs[i].type == DRO_AXIS_TYPE_ROTARY) {
             system_state.axes[i].displayed_value = result_mm;
        } else {
            // Linear Axis: Convert MM/Inch
            if (system_state.current_unit == DRO_UNIT_INCH) {
                system_state.axes[i].displayed_value = result_mm / 25.4f;
            } else {
                system_state.axes[i].displayed_value = result_mm;
            }
        }
    }
}

void dro_toggle_units(void) {
    if (system_state.current_unit == DRO_UNIT_MM) {
        system_state.current_unit = DRO_UNIT_INCH;
    } else {
        system_state.current_unit = DRO_UNIT_MM;
    }
    dro_nvs_save_param(DRO_KEY_UNITS, system_state.current_unit);
    ESP_LOGI(TAG, "Units toggled to %d", system_state.current_unit);
}

void dro_toggle_mode(void) {
    if (system_state.current_mode == DRO_MODE_ABS) {
        system_state.current_mode = DRO_MODE_INC;
    } else {
        system_state.current_mode = DRO_MODE_ABS;
    }
    dro_nvs_save_param(DRO_KEY_MODE, system_state.current_mode);
    ESP_LOGI(TAG, "Mode toggled to %d", system_state.current_mode);
}

void dro_set_raw_counts(uint8_t axis_index, int32_t counts) {
    if (axis_index >= DRO_AXIS_COUNT) return;
    system_state.axes[axis_index].raw_counts = (float)counts;
    // dro_update() will handle the recalculation on next tick
}

void dro_set_spindle_telemetry(int32_t counts, float rpm) {
    system_state.current_spindle_counts = counts;
    system_state.current_spindle_rpm = rpm;
    
    // Temporarily hardwire to Axis C (5th Axis) so the user doesn't need to wipe NVS
    dro_set_raw_counts(DRO_AXIS_C, counts);
    
    // Route to any user-configured axes
    for (int i = 0; i < DRO_AXIS_COUNT; i++) {
        if (system_state.axis_configs[i].is_spindle_readout && i != DRO_AXIS_C) {
            dro_set_raw_counts((uint8_t)i, counts);
        }
    }
}

void dro_set_raw_position(uint8_t axis_index, float position_mm) {
    if (axis_index >= DRO_AXIS_COUNT) return;
    
    // Reverse engineer counts for compatibility
    // Pos = (Counts / PPU) * Gear
    // Counts = (Pos / Gear) * PPU
    float ppu = system_state.axis_configs[axis_index].pulses_per_unit;
    if (ppu == 0.0f) ppu = 200.0f; 
    float gear = system_state.axis_configs[axis_index].gear_ratio;
    if (gear == 0.0f) gear = 1.0f;

    float counts = (position_mm / gear) * ppu;
    if (system_state.axis_configs[axis_index].inverted) counts = -counts;
    
    system_state.axes[axis_index].raw_counts = counts;
}



void dro_set_axis_config(uint8_t axis_index, dro_axis_config_t config) {
    if (axis_index >= DRO_AXIS_COUNT) return;
    
    // Update runtime state
    system_state.axis_configs[axis_index] = config;
    
    // Save to NVS
    dro_nvs_save_axis_config(axis_index, &config, sizeof(dro_axis_config_t));
    
    ESP_LOGI(TAG, "Axis %d config updated and saved.", axis_index);
}

void dro_axis_zero(uint8_t axis_index) {
    if (axis_index >= DRO_AXIS_COUNT) return;
    
    // In ABS mode, we usually don't zero Machine Coords? 
    // Usually "Set Zero" modifies the Work Offset to make Current Display = 0.
    // Display = Raw - WorkOffset - Tool
    // 0 = Raw - WorkOffset - Tool
    // WorkOffset = Raw - Tool
    
    // NOTE: If in ABS mode, standard practice is often to NOT allow zeroing 
    // or to treat it as setting G54 (Work) zero anyway.
    // Let's assume the user wants to zero the current viewing coordinate system.
    // So we update Work Offset regardless of mode, but effect is visible in INC.
    
    float new_offset = system_state.axes[axis_index].position_mm - system_state.axes[axis_index].tool_offset;
    system_state.axes[axis_index].work_offset = new_offset;
    
    // Save to active workspace
    dro_workspace_t space;
    if (dro_workspace_get(system_state.active_space_index, &space) == ESP_OK) {
        space.offsets[axis_index] = new_offset;
        dro_workspace_set(system_state.active_space_index, &space);
    }

    ESP_LOGI(TAG, "Axis %d Zeroed in Space %ld. New Offset: %f", axis_index, (long)system_state.active_space_index, new_offset);
    
    // Force mode to INC so they see the Zero? 
    // Or just leave it? Usually DROs switch to INC or stay in INC.
    if (system_state.current_mode == DRO_MODE_ABS) {
        dro_toggle_mode(); // Switch to INC to show the zero
    }
}

void dro_axis_set_value(uint8_t axis_index, float value) {
    if (axis_index >= DRO_AXIS_COUNT) return;

    // The user wants Display = Value.
    // Value (in current units) -> convert to mm first
    float value_mm = value;
    if (system_state.current_unit == DRO_UNIT_INCH) {
        value_mm = value * 25.4f;
    }

    // Display_mm = Raw - WorkOffset - Tool
    // WorkOffset = Raw - Tool - Display_mm
    float new_offset = system_state.axes[axis_index].position_mm - 
                       system_state.axes[axis_index].tool_offset - 
                       value_mm;

    system_state.axes[axis_index].work_offset = new_offset;
    
    // Save to active workspace
    dro_workspace_t space;
    if (dro_workspace_get(system_state.active_space_index, &space) == ESP_OK) {
        space.offsets[axis_index] = new_offset;
        dro_workspace_set(system_state.active_space_index, &space);
    }
    
    if (system_state.current_mode == DRO_MODE_ABS) {
        dro_toggle_mode(); // Switch to INC
    }
}

void dro_axis_half(uint8_t axis_index) {
    if (axis_index >= DRO_AXIS_COUNT) return;

    // Use current displayed value (in current units)
    // We want Display_New = Display_Current / 2
    // It's effectively setting the value to (Current / 2)
    
    // Calculate current relative position in mm
    float current_rel_mm = system_state.axes[axis_index].position_mm - 
                           system_state.axes[axis_index].work_offset - 
                           system_state.axes[axis_index].tool_offset;
    
    // New Target = Current / 2
    float target_mm = current_rel_mm / 2.0f;
    
    // WorkOffset = Raw - Tool - Target
    float new_offset = system_state.axes[axis_index].position_mm - 
                       system_state.axes[axis_index].tool_offset - 
                       target_mm;
                       
    system_state.axes[axis_index].work_offset = new_offset;
    
    // Save to active workspace
    dro_workspace_t space;
    if (dro_workspace_get(system_state.active_space_index, &space) == ESP_OK) {
        space.offsets[axis_index] = new_offset;
        dro_workspace_set(system_state.active_space_index, &space);
    }
    
    if (system_state.current_mode == DRO_MODE_ABS) {
        dro_toggle_mode();
    }
}

esp_err_t dro_tool_get(uint8_t tool_index, dro_tool_t* tool_out) {
    if (tool_index >= DRO_MAX_TOOLS || !tool_out) return ESP_ERR_INVALID_ARG;
    
    esp_err_t err = dro_nvs_load_tool(tool_index, tool_out, sizeof(dro_tool_t));
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Return clear tool if not found
        memset(tool_out, 0, sizeof(dro_tool_t));
        snprintf(tool_out->name, sizeof(tool_out->name), "Tool %d", tool_index);
        return ESP_OK;
    }
    return err;
}

esp_err_t dro_tool_set(uint8_t tool_index, const dro_tool_t* tool_in) {
    if (tool_index >= DRO_MAX_TOOLS || !tool_in) return ESP_ERR_INVALID_ARG;
    return dro_nvs_save_tool(tool_index, tool_in, sizeof(dro_tool_t));
}

esp_err_t dro_tool_apply(int32_t tool_index) {
    if (tool_index < 0 || tool_index >= DRO_MAX_TOOLS) {
        return ESP_ERR_INVALID_ARG;
    }

    dro_tool_t tool;
    esp_err_t err = dro_tool_get((uint8_t)tool_index, &tool);
    if (err != ESP_OK) return err;

    // Apply offsets to all axes
    for (int i = 0; i < DRO_AXIS_COUNT; i++) {
        system_state.axes[i].tool_offset = tool.offsets[i];
    }
    
    system_state.active_tool_index = tool_index;
    dro_nvs_save_param(DRO_KEY_TOOL, tool_index);
    
    ESP_LOGI(TAG, "Applied Tool %ld: %s", (long)tool_index, tool.name);
    return ESP_OK;
}

int32_t dro_tool_get_active_index(void) {
    return system_state.active_tool_index;
}

esp_err_t dro_workspace_get(uint8_t space_index, dro_workspace_t* space_out) {
    if (space_index >= DRO_MAX_WORKSPACES || !space_out) return ESP_ERR_INVALID_ARG;
    
    esp_err_t err = dro_nvs_load_workspace(space_index, space_out, sizeof(dro_workspace_t));
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        memset(space_out, 0, sizeof(dro_workspace_t));
        snprintf(space_out->name, sizeof(space_out->name), "Space %d", space_index);
        return ESP_OK;
    }
    return err;
}

esp_err_t dro_workspace_set(uint8_t space_index, const dro_workspace_t* space_in) {
    if (space_index >= DRO_MAX_WORKSPACES || !space_in) return ESP_ERR_INVALID_ARG;
    return dro_nvs_save_workspace(space_index, space_in, sizeof(dro_workspace_t));
}

esp_err_t dro_workspace_apply(int32_t space_index) {
    if (space_index < 0 || space_index >= DRO_MAX_WORKSPACES) {
        return ESP_ERR_INVALID_ARG;
    }

    dro_workspace_t space;
    esp_err_t err = dro_workspace_get((uint8_t)space_index, &space);
    if (err != ESP_OK) return err;

    // Apply offsets to all live axis states
    for (int i = 0; i < DRO_AXIS_COUNT; i++) {
        system_state.axes[i].work_offset = space.offsets[i];
    }
    
    system_state.active_space_index = space_index;
    dro_nvs_save_param(DRO_KEY_SPACE, space_index);
    
    ESP_LOGI(TAG, "Applied Workspace %ld: %s", (long)space_index, space.name);
    return ESP_OK;
}

int32_t dro_workspace_get_active_index(void) {
    return system_state.active_space_index;
}
