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
        system_state.axes[i].raw_position = 0.0f; // Reset on boot
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

void dro_update(void) {
    if (!system_state.is_initialized) return;

    for (int i = 0; i < DRO_AXIS_COUNT; i++) {
        float result_mm = 0.0f;
        
        if (system_state.current_mode == DRO_MODE_ABS) {
            // Absolute Mode: Show scaled absolute position
            // (Typically Machine Zero, but simplified here as just raw)
            // TODO: Clarify "Machine Zero" vs "Work Zero" concept mapping
            // For now: ABS = Raw (Machine Coords)
            result_mm = system_state.axes[i].raw_position;
        } else {
            // Incremental Mode: Work Offset applied
            // Result = Raw - WorkOffset - ToolOffset
            result_mm = system_state.axes[i].raw_position - 
                       system_state.axes[i].work_offset - 
                       system_state.axes[i].tool_offset;
        }

        // Apply Unit Conversion
        if (system_state.current_unit == DRO_UNIT_INCH) {
            system_state.axes[i].displayed_value = result_mm / 25.4f;
        } else {
            system_state.axes[i].displayed_value = result_mm;
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

void dro_set_raw_position(uint8_t axis_index, float position_mm) {
    if (axis_index >= DRO_AXIS_COUNT) return;
    system_state.axes[axis_index].raw_position = position_mm;
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
    
    float new_offset = system_state.axes[axis_index].raw_position - system_state.axes[axis_index].tool_offset;
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
    float new_offset = system_state.axes[axis_index].raw_position - 
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
    float current_rel_mm = system_state.axes[axis_index].raw_position - 
                           system_state.axes[axis_index].work_offset - 
                           system_state.axes[axis_index].tool_offset;
    
    // New Target = Current / 2
    float target_mm = current_rel_mm / 2.0f;
    
    // WorkOffset = Raw - Tool - Target
    float new_offset = system_state.axes[axis_index].raw_position - 
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
