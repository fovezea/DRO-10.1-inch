#include <string.h>
#include <stdlib.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "../e_screw_screen.h"
#include <math.h>
#include "dro_core.h"


static void event_handler_back_to_main(lv_event_t *e);


objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

static void event_handler_cb_main_obj1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_active_space_number(atoi(value));
        }
    }
}

static void event_handler_cb_main_active_tool_number(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_active_tool_number(atoi(value));
        }
    }
}

static void event_handler_cb_main_axis4_textarea(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_virtual_axis_4(atof(value));
        }
    }
}

static void event_handler_cb_main_axis1_textarea(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_virtual_axis_1(atof(value));
        }
    }
}

static void event_handler_cb_main_axis3_textarea(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_virtual_axis_3(atof(value));
        }
    }
}

static void event_handler_cb_main_axis5_textarea(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_virtual_axis_5(atof(value));
        }
    }
}

static void event_handler_cb_main_axis2_textarea(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            set_var_virtual_axis_2(atof(value));
        }
    }
}

static void tab_button_event_handler(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_CLICKED) {
        // lv_obj_t *button = lv_event_get_target(e); // Unused
        uint32_t tab_index = (uint32_t)lv_event_get_user_data(e);
        
        // Manually switch to the selected tab
        lv_tabview_set_active(objects.obj0, tab_index, LV_ANIM_ON);
    }
}



// --- Custom Axis Settings UI Helpers ---

typedef struct {
    lv_obj_t *sw_enable;
    lv_obj_t *dd_type;
    lv_obj_t *sw_invert;
    lv_obj_t *ta_ppu;
    lv_obj_t *ta_gear;
    lv_obj_t *ta_pitch;
    // Calib handles
    lv_obj_t *ta_dist;
    lv_obj_t *lbl_calib_status; // To show "Start Set" or result
    lv_obj_t *dd_resolution;
    lv_obj_t *sw_is_spindle;
} axis_settings_ui_t;

static axis_settings_ui_t axis_ui_handles[DRO_AXIS_COUNT];
static float axis_calib_start_counts[DRO_AXIS_COUNT];
static bool axis_dirty_flags[DRO_AXIS_COUNT] = {false};

static void event_handler_resolution_change(lv_event_t *e) {
    int axis_index = (int)(intptr_t)lv_event_get_user_data(e);
    if (axis_index >= DRO_AXIS_COUNT) return;
    
    lv_obj_t *dd = lv_event_get_target(e);
    uint32_t selected = lv_dropdown_get_selected(dd);
    
    float new_ppu = 0.0f;
    // Options: "1 um\n2 um\n5 um\n10 um\nCustom"
    // 1 um = 1000 pulses/mm
    // 2 um = 500 pulses/mm
    // 5 um = 200 pulses/mm
    // 10 um = 100 pulses/mm
    
    switch(selected) {
        case 0: new_ppu = 1000.0f; break;
        case 1: new_ppu = 500.0f; break;
        case 2: new_ppu = 200.0f; break;
        case 3: new_ppu = 100.0f; break;
        default: return; // Custom or match failed
    }
    
    if (new_ppu > 0.0f) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", new_ppu);
        lv_textarea_set_text(axis_ui_handles[axis_index].ta_ppu, buf);
        
        // Mark dirty since we changed a value programmatically
        axis_dirty_flags[axis_index] = true;
    }
}

static void event_handler_high_precision_toggle(lv_event_t *e) {
    lv_obj_t *sw = lv_event_get_target(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    dro_set_high_precision(enabled);
}

static void event_handler_machine_type_change(lv_event_t *e) {
    lv_obj_t *dropdown = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    dro_set_machine_type((dro_machine_type_t)selected);
}

static void event_handler_axis_count_change(lv_event_t *e) {
    lv_obj_t *dropdown = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    // Selected 0 => 1 Axis, 1 => 2 Axes, etc.
    dro_set_active_axis_count((uint8_t)(selected + 1));
}

static void msgbox_discard_event_handler(lv_event_t * e) {
    lv_obj_t * mbox = (lv_obj_t *)lv_event_get_user_data(e);
    
    if (mbox) {
        int axis_index = (int)(intptr_t)lv_obj_get_user_data(mbox);
        if (axis_index >= 0 && axis_index < DRO_AXIS_COUNT) {
            axis_dirty_flags[axis_index] = false;
        }
        lv_msgbox_close(mbox);
    }
    
    lv_scr_load(objects.main);
}

static void msgbox_cancel_event_handler(lv_event_t * e) {
    lv_obj_t * mbox = (lv_obj_t *)lv_event_get_user_data(e);
    if (mbox) lv_msgbox_close(mbox);
}

static void event_handler_return_check(lv_event_t *e) {
    int axis_index = (int)(intptr_t)lv_event_get_user_data(e);
    if (axis_index >= DRO_AXIS_COUNT) return;

    if (axis_dirty_flags[axis_index]) {
        // LVGL v9 API
        lv_obj_t * mbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(mbox, "Unsaved Changes");
        lv_msgbox_add_text(mbox, "You have unsaved changes. Do you want to discard them?");
        
        // Store axis_index in mbox user data
        lv_obj_set_user_data(mbox, (void*)(intptr_t)axis_index);

        lv_obj_t * btn_discard = lv_msgbox_add_footer_button(mbox, "Discard");
        lv_obj_t * btn_cancel = lv_msgbox_add_footer_button(mbox, "Cancel");

        // Pass mbox as user_data to buttons so we can delete it
        lv_obj_add_event_cb(btn_discard, msgbox_discard_event_handler, LV_EVENT_CLICKED, mbox);
        lv_obj_add_event_cb(btn_cancel, msgbox_cancel_event_handler, LV_EVENT_CLICKED, mbox);

        lv_obj_center(mbox);
    } else {
        lv_scr_load(objects.main);
    }
}

static void mark_axis_dirty(lv_event_t *e) {
    int axis_index = (int)(intptr_t)lv_event_get_user_data(e);
    if (axis_index >= 0 && axis_index < DRO_AXIS_COUNT) {
        // Only mark dirty if it wasn't already (optional optimization)
        if (!axis_dirty_flags[axis_index]) {
            axis_dirty_flags[axis_index] = true;
            // Optional: Update UI to show "Modified" state?
        }
    }
}

static void event_handler_calib_start(lv_event_t *e) {
    int axis_index = (int)(intptr_t)lv_event_get_user_data(e);
    if (axis_index >= DRO_AXIS_COUNT) return;
    
    const dro_system_state_t *state = dro_get_state();
    axis_calib_start_counts[axis_index] = state->axes[axis_index].raw_counts;
    
    lv_label_set_text(axis_ui_handles[axis_index].lbl_calib_status, "Start Point Set. Move axis now.");
}

static void event_handler_calib_finish(lv_event_t *e) {
    int axis_index = (int)(intptr_t)lv_event_get_user_data(e);
    if (axis_index >= DRO_AXIS_COUNT) return;

    const dro_system_state_t *state = dro_get_state();
    float current_counts = state->axes[axis_index].raw_counts;
    float start_counts = axis_calib_start_counts[axis_index];
    float delta_counts = current_counts - start_counts;
    
    // Get distance
    const char* txt_dist = lv_textarea_get_text(axis_ui_handles[axis_index].ta_dist);
    float dist = atof(txt_dist);
    
    if (dist == 0.0f) {
        lv_label_set_text(axis_ui_handles[axis_index].lbl_calib_status, "Error: Distance is 0");
        return;
    }
    
    // Calculate PPU = Counts / Dist
    // We take absolute value of PPU (direction handled by Invert)
    // Actually, delta might be negative if moved backwards. Dist is usually positive.
    float new_ppu = fabsf(delta_counts / dist);
    
    // Update UI
    char buf[16];
    snprintf(buf, sizeof(buf), "%.4f", new_ppu);
    lv_textarea_set_text(axis_ui_handles[axis_index].ta_ppu, buf);
    
    lv_label_set_text_fmt(axis_ui_handles[axis_index].lbl_calib_status, "Calc: %.0f/%.1f = %.4f", delta_counts, dist, new_ppu);
}

static void event_handler_save_axis_settings(lv_event_t *e) {
    int axis_index = (int)(intptr_t)lv_event_get_user_data(e);
    if (axis_index >= DRO_AXIS_COUNT) return;
    
    // Read values from UI
    bool enabled = lv_obj_has_state(axis_ui_handles[axis_index].sw_enable, LV_STATE_CHECKED);
    uint32_t type = lv_dropdown_get_selected(axis_ui_handles[axis_index].dd_type);
    bool inverted = lv_obj_has_state(axis_ui_handles[axis_index].sw_invert, LV_STATE_CHECKED);
    bool is_spindle = lv_obj_has_state(axis_ui_handles[axis_index].sw_is_spindle, LV_STATE_CHECKED);
    
    // Parse floats
    const char* txt_ppu = lv_textarea_get_text(axis_ui_handles[axis_index].ta_ppu);
    const char* txt_gear = lv_textarea_get_text(axis_ui_handles[axis_index].ta_gear);
    const char* txt_pitch = lv_textarea_get_text(axis_ui_handles[axis_index].ta_pitch);
    
    float ppu = atof(txt_ppu);
    float gear = atof(txt_gear);
    float pitch = atof(txt_pitch);
    
    if (ppu == 0.0f) ppu = 200.0f; // Safety
    
    // Update System State directly (dirty way, but quick for now)
    // Ideally we should have a setter in dro_core
    // But direct access is fine since we are "System UI"
    // Wait, system_state is static in dro_core.c, so we can't access it directly unless exposed.
    // dro_get_state() returns const pointer.
    
    // We need dro_save_axis_config(index, config) in dro_core exposed? 
    // Or just construct the struct and save to NVS, and reload?
    
    dro_axis_config_t config;
    config.enabled = enabled;
    config.type = (dro_axis_type_t)type;
    config.pulses_per_unit = ppu;
    config.gear_ratio = gear;
    config.inverted = inverted;
    config.is_spindle_readout = is_spindle;
    config.leadscrew_pitch = pitch;
    
    // Save to NVS and Update Runtime
    dro_set_axis_config(axis_index, config);
    
    // Clear Dirty Flag
    axis_dirty_flags[axis_index] = false;

    // LOG
    LV_LOG_USER("Saved Axis %d: PPU=%.2f, Gear=%.2f, Type=%d", axis_index, ppu, gear, (int)type);
    
    // Optional: Visual Feedback
   lv_obj_t *btn = lv_event_get_target(e);
   lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), 0); // Green feedback
}

static void create_axis_settings_ui(lv_obj_t *parent, int axis_index) {
    if (axis_index >= DRO_AXIS_COUNT) return;
    
    // Get current config
    const dro_system_state_t *state = dro_get_state();
    dro_axis_config_t config = state->axis_configs[axis_index];

    // 1. Enable Switch
    lv_obj_t *sw_enable = lv_switch_create(parent);
    lv_obj_set_pos(sw_enable, 20, 20);
    lv_obj_set_size(sw_enable, 50, 25);
    if (config.enabled) lv_obj_add_state(sw_enable, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_enable, mark_axis_dirty, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)axis_index);
    axis_ui_handles[axis_index].sw_enable = sw_enable;
    
    lv_obj_t *lbl_enable = lv_label_create(parent);
    lv_obj_set_pos(lbl_enable, 80, 22);
    lv_obj_set_style_text_font(lbl_enable, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_enable, "Enable Axis");

    // 2. Type Dropdown
    lv_obj_t *dd_type = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd_type, "Linear\nRotary");
    lv_dropdown_set_selected(dd_type, (uint16_t)config.type);
    lv_obj_set_pos(dd_type, 20, 80);
    lv_obj_set_width(dd_type, 180);
    lv_obj_set_style_text_font(dd_type, &lv_font_montserrat_22, 0);
    lv_obj_add_event_cb(dd_type, mark_axis_dirty, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)axis_index);
    axis_ui_handles[axis_index].dd_type = dd_type;

    // 3. Invert Switch (Moved right)
    lv_obj_t *sw_invert = lv_switch_create(parent);
    lv_obj_set_pos(sw_invert, 350, 20);
    lv_obj_set_size(sw_invert, 50, 25);
    if (config.inverted) lv_obj_add_state(sw_invert, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_invert, mark_axis_dirty, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)axis_index);
    axis_ui_handles[axis_index].sw_invert = sw_invert;
    
    lv_obj_t *lbl_invert = lv_label_create(parent);
    lv_obj_set_pos(lbl_invert, 410, 22);
    lv_obj_set_style_text_font(lbl_invert, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_invert, "Invert Direction");

    // 3.5 Is Spindle Readout
    lv_obj_t *sw_spindle = lv_switch_create(parent);
    lv_obj_set_pos(sw_spindle, 600, 20);
    lv_obj_set_size(sw_spindle, 50, 25);
    if (config.is_spindle_readout) lv_obj_add_state(sw_spindle, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_spindle, mark_axis_dirty, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)axis_index);
    axis_ui_handles[axis_index].sw_is_spindle = sw_spindle;
    
    lv_obj_t *lbl_spindle = lv_label_create(parent);
    lv_obj_set_pos(lbl_spindle, 660, 22);
    lv_obj_set_style_text_font(lbl_spindle, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_spindle, "Track Spindle Data");

    // 4. Pulses Per Unit (Moved down)
    lv_obj_t *ta_ppu = lv_textarea_create(parent);
    lv_obj_set_pos(ta_ppu, 20, 200);
    lv_obj_set_size(ta_ppu, 250, 50);
    lv_obj_set_style_text_font(ta_ppu, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_ppu, true);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", config.pulses_per_unit);
    lv_textarea_set_text(ta_ppu, buf);
    lv_obj_add_event_cb(ta_ppu, mark_axis_dirty, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)axis_index);
    axis_ui_handles[axis_index].ta_ppu = ta_ppu;
    
    lv_obj_t *lbl_ppu = lv_label_create(parent);
    lv_obj_set_pos(lbl_ppu, 20, 160);
    lv_obj_set_style_text_font(lbl_ppu, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_ppu, "Pulses Per Unit");
    
    // Resolution Dropdown (Moved down)
    lv_obj_t *dd_res = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd_res, "1 um\n2 um\n5 um\n10 um\nCustom");
    lv_obj_set_pos(dd_res, 20, 300); 
    lv_obj_set_width(dd_res, 150);
    lv_obj_set_style_text_font(dd_res, &lv_font_montserrat_22, 0);
    lv_obj_add_event_cb(dd_res, event_handler_resolution_change, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)axis_index);
    axis_ui_handles[axis_index].dd_resolution = dd_res;
    
    // Auto-select resolution
    int res_idx = 4; // Custom
    if (fabsf(config.pulses_per_unit - 1000.0f) < 0.01f) res_idx = 0;
    else if (fabsf(config.pulses_per_unit - 500.0f) < 0.01f) res_idx = 1;
    else if (fabsf(config.pulses_per_unit - 200.0f) < 0.01f) res_idx = 2;
    else if (fabsf(config.pulses_per_unit - 100.0f) < 0.01f) res_idx = 3;
    lv_dropdown_set_selected(dd_res, res_idx);

    lv_obj_t *lbl_res = lv_label_create(parent);
    lv_obj_set_pos(lbl_res, 20, 270);
    lv_obj_set_style_text_font(lbl_res, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_res, "Resolution");

    // 5. Gear Ratio (Moved Right & Down)
    lv_obj_t *ta_gear = lv_textarea_create(parent);
    lv_obj_set_pos(ta_gear, 350, 200);
    lv_obj_set_size(ta_gear, 150, 50);
    lv_obj_set_style_text_font(ta_gear, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_gear, true);
    snprintf(buf, sizeof(buf), "%.2f", config.gear_ratio);
    lv_textarea_set_text(ta_gear, buf);
    lv_obj_add_event_cb(ta_gear, mark_axis_dirty, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)axis_index);
    axis_ui_handles[axis_index].ta_gear = ta_gear;

    lv_obj_t *lbl_gear = lv_label_create(parent);
    lv_obj_set_pos(lbl_gear, 350, 160);
    lv_obj_set_style_text_font(lbl_gear, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_gear, "Gear Ratio");

    // 6. Lead Screw Pitch (Moved Right & Down)
    lv_obj_t *ta_pitch = lv_textarea_create(parent);
    lv_obj_set_pos(ta_pitch, 600, 200);
    lv_obj_set_size(ta_pitch, 150, 50);
    lv_obj_set_style_text_font(ta_pitch, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_pitch, true);
    snprintf(buf, sizeof(buf), "%.2f", config.leadscrew_pitch);
    lv_textarea_set_text(ta_pitch, buf);
    lv_obj_add_event_cb(ta_pitch, mark_axis_dirty, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)axis_index);
    axis_ui_handles[axis_index].ta_pitch = ta_pitch;

    lv_obj_t *lbl_pitch = lv_label_create(parent);
    lv_obj_set_pos(lbl_pitch, 600, 160);
    lv_obj_set_style_text_font(lbl_pitch, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_pitch, "Pitch (mm/rev)");
    
    // 7. Save Button (Moved Down)
    lv_obj_t *btn_save = lv_btn_create(parent);
    lv_obj_set_pos(btn_save, 20, 380);
    lv_obj_set_size(btn_save, 140, 50);
    lv_obj_add_event_cb(btn_save, event_handler_save_axis_settings, LV_EVENT_CLICKED, (void*)(intptr_t)axis_index);
    
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_obj_center(lbl_save);
    lv_obj_set_style_text_font(lbl_save, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_save, "SAVE");

    // 8. Return Button (Inside Tab)
    lv_obj_t *btn_back = lv_btn_create(parent);
    lv_obj_set_pos(btn_back, 1150, 10); // Top right corner of the tab
    lv_obj_set_size(btn_back, 100, 40);
    lv_obj_add_event_cb(btn_back, event_handler_return_check, LV_EVENT_CLICKED, (void*)(intptr_t)axis_index);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xFF0000), 0); // Red
    
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_obj_center(lbl_back);
    lv_label_set_text(lbl_back, "RETURN");


    // --- Calibration Wizard Section ---
    lv_obj_t *cont_calib = lv_obj_create(parent);
    lv_obj_set_pos(cont_calib, 600, 320); // Moved down 300px (was 400)
    lv_obj_set_size(cont_calib, 600, 300); // Increased Size
    lv_obj_set_style_bg_opa(cont_calib, LV_OPA_20, 0); 
    
    lv_obj_t *lbl_calib_title = lv_label_create(cont_calib);
    lv_obj_set_pos(lbl_calib_title, 10, 5);
    lv_label_set_text(lbl_calib_title, "Calibration Wizard");
    lv_obj_set_style_text_font(lbl_calib_title, &lv_font_montserrat_22, 0);

    // Status Label
    lv_obj_t *lbl_status = lv_label_create(cont_calib);
    lv_obj_set_pos(lbl_status, 10, 220); // Moved down due to larger spacing
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_status, "Status: Ready");
    axis_ui_handles[axis_index].lbl_calib_status = lbl_status;

    // 1. Set Start
    lv_obj_t *btn_start = lv_btn_create(cont_calib);
    lv_obj_set_pos(btn_start, 10, 50);
    lv_obj_set_size(btn_start, 140, 50);
    lv_obj_add_event_cb(btn_start, event_handler_calib_start, LV_EVENT_CLICKED, (void*)(intptr_t)axis_index);
    lv_obj_t *lbl_btn_start = lv_label_create(btn_start);
    lv_obj_center(lbl_btn_start);
    lv_obj_set_style_text_font(lbl_btn_start, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_btn_start, "Set Start");

    // 2. Known Distance Input
    lv_obj_t *ta_dist = lv_textarea_create(cont_calib);
    lv_obj_set_pos(ta_dist, 180, 50);
    lv_obj_set_size(ta_dist, 180, 50);
    lv_obj_set_style_text_font(ta_dist, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_dist, true);
    lv_textarea_set_placeholder_text(ta_dist, "Dist (e.g. 10.0)");
    axis_ui_handles[axis_index].ta_dist = ta_dist;

    // 3. Button Calibrate
    lv_obj_t *btn_calib = lv_btn_create(cont_calib);
    lv_obj_set_pos(btn_calib, 380, 50);
    lv_obj_set_size(btn_calib, 160, 50);
    lv_obj_add_event_cb(btn_calib, event_handler_calib_finish, LV_EVENT_CLICKED, (void*)(intptr_t)axis_index);
    lv_obj_t *lbl_btn_calib = lv_label_create(btn_calib);
    lv_obj_center(lbl_btn_calib);
    lv_obj_set_style_text_font(lbl_btn_calib, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_btn_calib, "CALCULATE");
    
    lv_obj_t *lbl_instr = lv_label_create(cont_calib);
    lv_obj_set_pos(lbl_instr, 10, 120);
    lv_obj_set_style_text_font(lbl_instr, &lv_font_montserrat_22, 0);
    lv_label_set_text(lbl_instr, "1. Click 'Set Start'.\n2. Move axis by known distance.\n3. Enter distance & Click 'Calculate'.");
}

void create_functions_tab_ui(lv_obj_t *parent) {
    // Disable scrolling on the tab page itself
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // --- TOP SECTION: DRO VALUES ---
    lv_obj_t *cont_top = lv_obj_create(parent);
    lv_obj_set_pos(cont_top, 0, 0);
    lv_obj_set_size(cont_top, 1280, 100);
    lv_obj_set_style_bg_opa(cont_top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_top, 0, 0);

    // X Value
    lv_obj_t *lbl_x = lv_label_create(cont_top);
    lv_obj_set_pos(lbl_x, 50, 30);
    lv_obj_set_style_text_font(lbl_x, &ui_font_jet_brains_mono_bold_64, 0); // Corrected Font
    lv_label_set_text(lbl_x, "X 0.000"); // Removed + sign

    // Y Value
    lv_obj_t *lbl_y = lv_label_create(cont_top);
    lv_obj_set_pos(lbl_y, 450, 30);
    lv_obj_set_style_text_font(lbl_y, &ui_font_jet_brains_mono_bold_64, 0); // Corrected Font
    lv_label_set_text(lbl_y, "Y 0.000"); // Removed + sign

    // Z Value
    lv_obj_t *lbl_z = lv_label_create(cont_top);
    lv_obj_set_pos(lbl_z, 850, 30);
    lv_obj_set_style_text_font(lbl_z, &ui_font_jet_brains_mono_bold_64, 0); // Corrected Font
    lv_label_set_text(lbl_z, "Z 0.000"); // Removed + sign

    // --- MIDDLE SECTION: SPLIT PANELS ---
    // Left Panel: Linear
    lv_obj_t *cont_left = lv_obj_create(parent);
    lv_obj_set_pos(cont_left, 0, 100);
    lv_obj_set_size(cont_left, 620, 500); 
    lv_obj_set_style_border_width(cont_left, 2, 0);
    lv_obj_set_style_border_color(cont_left, lv_color_hex(0xf9f600), 0); 

    lv_obj_t *lbl_lin_title = lv_label_create(cont_left);
    lv_obj_set_pos(lbl_lin_title, 200, 10);
    lv_obj_set_style_text_font(lbl_lin_title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_lin_title, lv_color_hex(0xf9f600), 0); 
    lv_label_set_text(lbl_lin_title, "LINEAR PATTERN");

    // --- GRAPHIC MOCKUP (Top Area) ---
    // Mockup: 4 holes, 25 deg angle. 
    // Start approx (200, 200). Vector (+54, -25) per hole.

    // Line (Axis)
    static lv_point_precise_t line_points[] = { {200, 200}, {362, 125} };
    lv_obj_t * line1 = lv_line_create(cont_left);
    lv_line_set_points(line1, line_points, 2);
    lv_obj_set_style_line_width(line1, 2, 0);
    lv_obj_set_style_line_color(line1, lv_color_hex(0x808080), 0); // Grey axis line

    // Hole 0
    lv_obj_t *h0 = lv_obj_create(cont_left);
    lv_obj_set_size(h0, 12, 12);
    lv_obj_set_style_radius(h0, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(h0, lv_color_hex(0x00FF00), 0); // Green
    lv_obj_set_style_border_width(h0, 0, 0);
    lv_obj_set_pos(h0, 194, 194); // Centered on (200, 200)

    // Hole 1 (+54, -25) -> (254, 175)
    lv_obj_t *h1 = lv_obj_create(cont_left);
    lv_obj_set_size(h1, 12, 12);
    lv_obj_set_style_radius(h1, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(h1, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_border_width(h1, 0, 0);
    lv_obj_set_pos(h1, 248, 169);

    // Hole 2 (+108, -50) -> (308, 150)
    lv_obj_t *h2 = lv_obj_create(cont_left);
    lv_obj_set_size(h2, 12, 12);
    lv_obj_set_style_radius(h2, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(h2, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_border_width(h2, 0, 0);
    lv_obj_set_pos(h2, 302, 144);

    // Hole 3 (+162, -75) -> (362, 125)
    lv_obj_t *h3 = lv_obj_create(cont_left);
    lv_obj_set_size(h3, 12, 12);
    lv_obj_set_style_radius(h3, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(h3, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_border_width(h3, 0, 0);
    lv_obj_set_pos(h3, 356, 119);


    // Row 1: Holes
    lv_obj_t *lbl_lin_holes = lv_label_create(cont_left);
    lv_obj_set_pos(lbl_lin_holes, 50, 300); 
    lv_obj_set_style_text_font(lbl_lin_holes, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_lin_holes, lv_color_hex(0xf9f600), 0);
    lv_label_set_text(lbl_lin_holes, "Number of Holes:");

    lv_obj_t *ta_lin_holes = lv_textarea_create(cont_left);
    lv_obj_set_pos(ta_lin_holes, 300, 290);
    lv_obj_set_size(ta_lin_holes, 150, 50);
    lv_obj_set_style_text_font(ta_lin_holes, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_lin_holes, true);
    lv_textarea_set_text(ta_lin_holes, "4"); // Updated default to 4

    // Row 2: Spacing (NEW)
    lv_obj_t *lbl_lin_space = lv_label_create(cont_left);
    lv_obj_set_pos(lbl_lin_space, 50, 370); 
    lv_obj_set_style_text_font(lbl_lin_space, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_lin_space, lv_color_hex(0xf9f600), 0);
    lv_label_set_text(lbl_lin_space, "Hole Spacing:");

    lv_obj_t *ta_lin_space = lv_textarea_create(cont_left);
    lv_obj_set_pos(ta_lin_space, 300, 360);
    lv_obj_set_size(ta_lin_space, 150, 50);
    lv_obj_set_style_text_font(ta_lin_space, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_lin_space, true);
    lv_textarea_set_text(ta_lin_space, "15.0"); // Updated default to 15.0

    // Row 3: Angle
    lv_obj_t *lbl_lin_angle = lv_label_create(cont_left);
    lv_obj_set_pos(lbl_lin_angle, 50, 440); // Shifted down
    lv_obj_set_style_text_font(lbl_lin_angle, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_lin_angle, lv_color_hex(0xf9f600), 0);
    lv_label_set_text(lbl_lin_angle, "Angle:");

    lv_obj_t *ta_lin_angle = lv_textarea_create(cont_left);
    lv_obj_set_pos(ta_lin_angle, 300, 430); // Shifted down
    lv_obj_set_size(ta_lin_angle, 150, 50);
    lv_obj_set_style_text_font(ta_lin_angle, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_lin_angle, true);
    lv_textarea_set_text(ta_lin_angle, "25.0"); // Updated default to 25.0

    // Right Panel: Circular
    lv_obj_t *cont_right = lv_obj_create(parent);
    lv_obj_set_pos(cont_right, 620, 100); 
    lv_obj_set_size(cont_right, 620, 500); 
    lv_obj_set_style_border_width(cont_right, 2, 0);
    lv_obj_set_style_border_color(cont_right, lv_color_hex(0xf9f600), 0); 

    lv_obj_t *lbl_circ_title = lv_label_create(cont_right);
    lv_obj_set_pos(lbl_circ_title, 200, 10);
    lv_obj_set_style_text_font(lbl_circ_title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_circ_title, lv_color_hex(0xf9f600), 0); 
    lv_label_set_text(lbl_circ_title, "CIRCULAR PATTERN");

    // Row 1: Holes
    lv_obj_t *lbl_circ_holes = lv_label_create(cont_right);
    lv_obj_set_pos(lbl_circ_holes, 50, 300);
    lv_obj_set_style_text_font(lbl_circ_holes, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_circ_holes, lv_color_hex(0xf9f600), 0);
    lv_label_set_text(lbl_circ_holes, "Number of Holes:");

    lv_obj_t *ta_circ_holes = lv_textarea_create(cont_right);
    lv_obj_set_pos(ta_circ_holes, 300, 290);
    lv_obj_set_size(ta_circ_holes, 150, 50);
    lv_obj_set_style_text_font(ta_circ_holes, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_circ_holes, true);
    lv_textarea_set_text(ta_circ_holes, "6");

    // Row 2: Radius
    lv_obj_t *lbl_circ_radius = lv_label_create(cont_right);
    lv_obj_set_pos(lbl_circ_radius, 50, 370);
    lv_obj_set_style_text_font(lbl_circ_radius, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_circ_radius, lv_color_hex(0xf9f600), 0);
    lv_label_set_text(lbl_circ_radius, "Radius:");

    lv_obj_t *ta_circ_radius = lv_textarea_create(cont_right);
    lv_obj_set_pos(ta_circ_radius, 300, 360);
    lv_obj_set_size(ta_circ_radius, 150, 50);
    lv_obj_set_style_text_font(ta_circ_radius, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_circ_radius, true);
    lv_textarea_set_text(ta_circ_radius, "50.0");

    // Row 3: Start Angle (NEW)
    lv_obj_t *lbl_circ_angle = lv_label_create(cont_right);
    lv_obj_set_pos(lbl_circ_angle, 50, 440); // Shifted down
    lv_obj_set_style_text_font(lbl_circ_angle, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_circ_angle, lv_color_hex(0xf9f600), 0);
    lv_label_set_text(lbl_circ_angle, "Start Angle:");

    lv_obj_t *ta_circ_angle = lv_textarea_create(cont_right);
    lv_obj_set_pos(ta_circ_angle, 300, 430); // Shifted down
    lv_obj_set_size(ta_circ_angle, 150, 50);
    lv_obj_set_style_text_font(ta_circ_angle, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_circ_angle, true);
    lv_textarea_set_text(ta_circ_angle, "0.0");


    // --- BOTTOM SECTION: CONTROLS ---
    lv_obj_t *cont_bot = lv_obj_create(parent);
    lv_obj_set_pos(cont_bot, 0, 590); // Corrected to 590 (580 + 10)original 550 abs pos
    lv_obj_set_size(cont_bot, 1280, 150); 
    lv_obj_set_style_bg_opa(cont_bot, LV_OPA_TRANSP, 0); 
    lv_obj_set_style_border_width(cont_bot, 0, 0);

    // Apply Button
    lv_obj_t *btn_apply = lv_btn_create(cont_bot);
    lv_obj_set_pos(btn_apply, 30, 0); // Relative Y=0 in container
    lv_obj_set_size(btn_apply, 200, 80);
    add_style_button_tyle1(btn_apply);
    
    lv_obj_t *lbl_apply = lv_label_create(btn_apply);
    lv_obj_center(lbl_apply);
    add_style_button_label(lbl_apply); // Use DRO style label
    lv_label_set_text(lbl_apply, "APPLY");

    // Cancel Button
    lv_obj_t *btn_cancel = lv_btn_create(cont_bot);
    lv_obj_set_pos(btn_cancel, 280, 0);
    lv_obj_set_size(btn_cancel, 200, 80);
    add_style_button_tyle1(btn_cancel);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x606060), 0); // Override for Grey
    
    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_obj_center(lbl_cancel);
    add_style_button_label(lbl_cancel);
    lv_label_set_text(lbl_cancel, "CANCEL");

    // Increment Buttons (Center)
    lv_obj_t *btn_prev = lv_btn_create(cont_bot);
    lv_obj_set_pos(btn_prev, 580, 0);
    lv_obj_set_size(btn_prev, 150, 80);
    add_style_button_tyle1(btn_prev);

    lv_obj_t *lbl_prev = lv_label_create(btn_prev);
    lv_obj_center(lbl_prev);
    add_style_button_label(lbl_prev);
    lv_label_set_text(lbl_prev, "< PREV");

    lv_obj_t *btn_next = lv_btn_create(cont_bot);
    lv_obj_set_pos(btn_next, 760, 0);
    lv_obj_set_size(btn_next, 150, 80);
    add_style_button_tyle1(btn_next);

    lv_obj_t *lbl_next = lv_label_create(btn_next);
    lv_obj_center(lbl_next);
    add_style_button_label(lbl_next);
    lv_label_set_text(lbl_next, "NEXT >");

    // Back Button (Right)
    lv_obj_t *btn_back = lv_btn_create(cont_bot);
    lv_obj_set_pos(btn_back, 1010, 0);
    lv_obj_set_size(btn_back, 200, 80);
    add_style_button_tyle1(btn_back);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xFF0000), 0); // Override for Red
    lv_obj_add_event_cb(btn_back, event_handler_back_to_main, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_obj_center(lbl_back);
    add_style_button_label(lbl_back);
    lv_label_set_text(lbl_back, "BACK");
}

void create_tool_library_tab_ui(lv_obj_t *parent) {
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // --- TOP SECTION: GRAPHIC & DETAILS (0-400px) ---
    lv_obj_t *cont_top = lv_obj_create(parent);
    lv_obj_set_pos(cont_top, 0, 0);
    lv_obj_set_size(cont_top, 1280, 400); 
    lv_obj_set_style_border_width(cont_top, 0, 0);
    lv_obj_set_style_bg_opa(cont_top, LV_OPA_TRANSP, 0);

    // 1. Tool Graphic (Center)
    // Placeholder Box for now
    lv_obj_t *rect_tool = lv_obj_create(cont_top);
    lv_obj_set_pos(rect_tool, 400, 50);
    lv_obj_set_size(rect_tool, 480, 300);
    lv_obj_set_style_bg_color(rect_tool, lv_color_hex(0x202020), 0);
    lv_obj_set_style_border_color(rect_tool, lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_width(rect_tool, 2, 0);
    
    // Label for Graphic
    lv_obj_t *lbl_graphic = lv_label_create(rect_tool);
    lv_obj_center(lbl_graphic);
    lv_label_set_text(lbl_graphic, "Tool Graphic Placeholder");
    lv_obj_set_style_text_color(lbl_graphic, lv_color_hex(0xFFFFFF), 0);

    // 2. Left Side Inputs
    // Tool Number
    lv_obj_t *lbl_tool_no = lv_label_create(cont_top);
    lv_obj_set_pos(lbl_tool_no, 50, 50);
    lv_label_set_text(lbl_tool_no, "Tool Number:");
    lv_obj_set_style_text_font(lbl_tool_no, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_tool_no, lv_color_hex(0xf9f600), 0);

    lv_obj_t *ta_tool_no = lv_textarea_create(cont_top);
    lv_obj_set_pos(ta_tool_no, 50, 90);
    lv_obj_set_size(ta_tool_no, 200, 50);
    lv_obj_set_style_text_font(ta_tool_no, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_tool_no, true);
    lv_textarea_set_text(ta_tool_no, "1");

    // Tool Type Dropdown
    lv_obj_t *lbl_type = lv_label_create(cont_top);
    lv_obj_set_pos(lbl_type, 50, 160);
    lv_label_set_text(lbl_type, "Tool Type:");
    lv_obj_set_style_text_font(lbl_type, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_type, lv_color_hex(0xf9f600), 0);

    lv_obj_t *dd_type = lv_dropdown_create(cont_top);
    lv_obj_set_pos(dd_type, 50, 200);
    lv_obj_set_width(dd_type, 250);
    lv_dropdown_set_options(dd_type, "End Mill\nBall End Mill\nDrill\nCenter Drill\nReamer\nTap\nFace Mill\nChamfer Mill\nCountersink\nBoring Bar\nThread Mill\nSlot Cutter\nFly Cutter");
    lv_obj_set_style_text_font(dd_type, &lv_font_montserrat_22, 0);

    // 3. Right Side Inputs
    // Diameter
    lv_obj_t *lbl_dia = lv_label_create(cont_top);
    lv_obj_set_pos(lbl_dia, 950, 50);
    lv_label_set_text(lbl_dia, "Diameter:");
    lv_obj_set_style_text_font(lbl_dia, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_dia, lv_color_hex(0xf9f600), 0);

    lv_obj_t *ta_dia = lv_textarea_create(cont_top);
    lv_obj_set_pos(ta_dia, 950, 90);
    lv_obj_set_size(ta_dia, 200, 50);
    lv_obj_set_style_text_font(ta_dia, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_dia, true);
    lv_textarea_set_text(ta_dia, "10.0");

    // Length
    lv_obj_t *lbl_len = lv_label_create(cont_top);
    lv_obj_set_pos(lbl_len, 950, 160);
    lv_label_set_text(lbl_len, "Length:");
    lv_obj_set_style_text_font(lbl_len, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_len, lv_color_hex(0xf9f600), 0);

    lv_obj_t *ta_len = lv_textarea_create(cont_top);
    lv_obj_set_pos(ta_len, 950, 200);
    lv_obj_set_size(ta_len, 200, 50);
    lv_obj_set_style_text_font(ta_len, &lv_font_montserrat_22, 0);
    lv_textarea_set_one_line(ta_len, true);
    lv_textarea_set_text(ta_len, "50.0");


    // --- BOTTOM SECTION: TABLE (410-800px) ---
    lv_obj_t *cont_bot = lv_obj_create(parent);
    lv_obj_set_pos(cont_bot, 0, 410);
    lv_obj_set_size(cont_bot, 1280, 390);
    lv_obj_set_style_bg_opa(cont_bot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_bot, 0, 0);

    // Table
    lv_obj_t *table = lv_table_create(cont_bot);
    lv_obj_set_pos(table, 50, 0);
    lv_obj_set_size(table, 1180, 300);
    lv_table_set_col_cnt(table, 5);
    lv_table_set_col_width(table, 0, 100); // ID
    lv_table_set_col_width(table, 1, 300); // Type
    lv_table_set_col_width(table, 2, 200); // Dia
    lv_table_set_col_width(table, 3, 200); // Len
    lv_table_set_col_width(table, 4, 300); // Material

    lv_table_set_cell_value(table, 0, 0, "ID");
    lv_table_set_cell_value(table, 0, 1, "Type");
    lv_table_set_cell_value(table, 0, 2, "Diameter");
    lv_table_set_cell_value(table, 0, 3, "Length");
    lv_table_set_cell_value(table, 0, 4, "Material");

    // Dummy Data
    lv_table_set_cell_value(table, 1, 0, "1");
    lv_table_set_cell_value(table, 1, 1, "End Mill");
    lv_table_set_cell_value(table, 1, 2, "10.0");
    lv_table_set_cell_value(table, 1, 3, "50.0");
    lv_table_set_cell_value(table, 1, 4, "HSS");

    // Controls Bottom
    lv_obj_t *btn_add = lv_btn_create(cont_bot);
    lv_obj_set_pos(btn_add, 50, 310);
    lv_obj_set_size(btn_add, 150, 60);
    lv_obj_t *lbl_add = lv_label_create(btn_add);
    lv_label_set_text(lbl_add, "ADD");
    lv_obj_center(lbl_add);

    lv_obj_t *btn_save = lv_btn_create(cont_bot);
    lv_obj_set_pos(btn_save, 220, 310);
    lv_obj_set_size(btn_save, 150, 60);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x00FF00), 0);
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, "SAVE");
    lv_obj_center(lbl_save);
}

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 1280, 800);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_tabview_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 1280, 800);
            lv_tabview_set_tab_bar_position(obj, LV_DIR_TOP);
            lv_tabview_set_tab_bar_size(obj, 60);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xf9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
            // Enable clickable tabs and ensure proper touch handling
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                        lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            
            // Ensure tabview is properly configured for touch input
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
            
            {
                lv_obj_t *parent_obj = obj;
                {
                    // DRO_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "DRO");
                    objects.dro_tab = obj;
                    add_style_tab_style(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xf9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
                    
                    // Get the tab button and add explicit event handler
                    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.obj0);
                    lv_obj_t *tab_button = lv_obj_get_child_by_type(tab_bar, 0, &lv_button_class);
                    if (tab_button) {
                        lv_obj_add_event_cb(tab_button, tab_button_event_handler, LV_EVENT_CLICKED, (void*)0);
                    }
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // Set_zero_axis5_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_zero_axis5_button = obj;
                            lv_obj_set_pos(obj, 638, 506);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "  SET\nZERO");
                                }
                            }
                        }
                        {
                            // Set_zero_axis4_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_zero_axis4_button = obj;
                            lv_obj_set_pos(obj, 638, 390);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "  SET\nZERO");
                                }
                            }
                        }
                        {
                            // Set_zero_axis_3_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_zero_axis_3_button = obj;
                            lv_obj_set_pos(obj, 638, 274);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "  SET\nZERO");
                                }
                            }
                        }
                        {
                            // Set_zero_axis1_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_zero_axis1_button = obj;
                            lv_obj_set_pos(obj, 638, 42);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "  SET\nZERO");
                                }
                            }
                        }
                        {
                            // Set_value_axis5_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_value_axis5_button = obj;
                            lv_obj_set_pos(obj, 476, 506);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "SET");
                                }
                            }
                        }
                        {
                            // Set_zero_axis2_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_zero_axis2_button = obj;
                            lv_obj_set_pos(obj, 638, 158);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "  SET\nZERO");
                                }
                            }
                        }
                        {
                            // Set_value_axis4_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_value_axis4_button = obj;
                            lv_obj_set_pos(obj, 476, 390);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "SET");
                                }
                            }
                        }
                        {
                            // Set_value_axis3_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_value_axis3_button = obj;
                            lv_obj_set_pos(obj, 476, 274);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "SET");
                                }
                            }
                        }
                        {
                            // Set_value_axis2_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_value_axis2_button = obj;
                            lv_obj_set_pos(obj, 476, 158);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "SET\n  ");
                                }
                            }
                        }
                        {
                            // Set_value_axis1_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_value_axis1_button = obj;
                            lv_obj_set_pos(obj, 476, 42);
                            lv_obj_set_size(obj, 100, 60);
                            add_style_button_tyle1(obj);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_PRESSED);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, -2);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "SET");
                                }
                            }
                        }
                        {
                            // mm_X_axis5_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.mm_x_axis5_label = obj;
                            lv_obj_set_pos(obj, 417, 534);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // mm_X_axis4_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.mm_x_axis4_label = obj;
                            lv_obj_set_pos(obj, 417, 418);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // mm_X_axis3_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.mm_x_axis3_label = obj;
                            lv_obj_set_pos(obj, 417, 302);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // mm_X_axis1_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.mm_x_axis1_label = obj;
                            lv_obj_set_pos(obj, 417, 70);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // settings_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.settings_button = obj;
                            lv_obj_set_pos(obj, 1075, 638);
                            lv_obj_set_size(obj, 150, 50);
                            lv_obj_add_event_cb(obj, action_change_to_setting_page, LV_EVENT_PRESSED, (void *)0);
                            add_style_button_tyle1(obj);
                            lv_obj_set_style_min_width(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // Settings
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.settings = obj;
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, 130, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Settings ->");
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.obj1 = obj;
                            lv_obj_set_pos(obj, 747, 638);
                            lv_obj_set_size(obj, 100, 60);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_placeholder_text(obj, "200");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_obj1, LV_EVENT_ALL, 0);
                            add_style_active_tool_and_space(obj);
                        }
                        {
                            // Active_tool_number
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.active_tool_number = obj;
                            lv_obj_set_pos(obj, 503, 638);
                            lv_obj_set_size(obj, 100, 60);
                            lv_textarea_set_max_length(obj, 3);
                            lv_textarea_set_placeholder_text(obj, "200");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_active_tool_number, LV_EVENT_ALL, 0);
                            add_style_active_tool_and_space(obj);
                        }
                        {
                            // half_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.half_button = obj;
                            lv_obj_set_pos(obj, 902, 638);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "1/2");
                                }
                            }
                        }
                        {
                            // Set_space_number_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_space_number_button = obj;
                            lv_obj_set_pos(obj, 638, 638);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);

                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "   set\nspace");
                                }
                            }
                        }
                        {
                            // set_tool_number_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_tool_number_button = obj;
                            lv_obj_set_pos(obj, 387, 638);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " set\ntool");
                                }
                            }
                        }
                        {
                            // Toggle_inch_mm_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.toggle_inch_mm_button = obj;
                            lv_obj_set_pos(obj, 268, 638);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "inch\nmm");
                                }
                            }
                        }
                        {
                            // toggle_abs_incr_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.toggle_abs_incr_button = obj;
                            lv_obj_set_pos(obj, 140, 638);
                            lv_obj_set_size(obj, 100, 60);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "abs\nincr");
                                }
                            }
                        }
                        {
                            // Set_zero_global_button
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.set_zero_global_button = obj;
                            lv_obj_set_pos(obj, 18, 638);
                            lv_obj_set_size(obj, 100, 50);
                            add_style_button_tyle1(obj);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    add_style_button_label(obj);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "  SET\nZERO");
                                }
                            }
                        }
                        {
                            // mm_X_axis2_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.mm_x_axis2_label = obj;
                            lv_obj_set_pos(obj, 417, 186);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // Axis4_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis4_textarea = obj;
                            lv_obj_set_pos(obj, 70, 378);
                            lv_obj_set_size(obj, 335, 80);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis4_textarea, LV_EVENT_ALL, 0);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            add_style_text_area_axis_value(obj);
                            lv_textarea_set_text(obj, "");
                        }
                        {
                            // Status bar labels
                            objects.mode_status_label = lv_label_create(parent_obj);
                            lv_obj_set_pos(objects.mode_status_label, 807, 10);
                            lv_obj_set_style_text_font(objects.mode_status_label, &lv_font_montserrat_22, 0);
                            lv_obj_set_style_text_color(objects.mode_status_label, lv_color_hex(0xfff9f600), 0);
                            lv_label_set_text(objects.mode_status_label, "ABSOLUTE");

                            objects.conn_status_label = lv_label_create(parent_obj);
                            lv_obj_set_pos(objects.conn_status_label, 1020, 10);
                            lv_obj_set_style_text_font(objects.conn_status_label, &lv_font_montserrat_22, 0);
                            lv_obj_set_style_text_color(objects.conn_status_label, lv_color_hex(0xfff9f600), 0);
                            lv_label_set_text(objects.conn_status_label, "DISCONNECTED");

                            // info_text_area (Input field)
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.info_text_area = obj;
                            lv_obj_set_pos(obj, 797, 125);
                            lv_obj_set_size(obj, 442, 55);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, 0);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);
                        }
                        {
                            // Axis1_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis1_textarea = obj;
                            lv_obj_set_pos(obj, 70, 30);
                            lv_obj_set_size(obj, 335, 80);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis1_textarea, LV_EVENT_ALL, 0);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            add_style_text_area_axis_value(obj);
                            lv_textarea_set_text(obj, "");
                        }
                        {
                            // Axis3_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis3_textarea = obj;
                            lv_obj_set_pos(obj, 70, 262);
                            lv_obj_set_size(obj, 335, 80);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis3_textarea, LV_EVENT_ALL, 0);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            add_style_text_area_axis_value(obj);
                            lv_textarea_set_text(obj, "");
                        }
                        {
                            // Axis5_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis5_textarea = obj;
                            lv_obj_set_pos(obj, 70, 494);
                            lv_obj_set_size(obj, 335, 80);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis5_textarea, LV_EVENT_ALL, 0);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            add_style_text_area_axis_value(obj);
                            lv_textarea_set_text(obj, "");
                        }
                        {
                            // Axis2_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis2_textarea = obj;
                            lv_obj_set_pos(obj, 70, 146);
                            lv_obj_set_size(obj, 335, 80);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis2_textarea, LV_EVENT_ALL, 0);
                            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
                            add_style_text_area_axis_value(obj);
                            lv_textarea_set_text(obj, "");
                        }
                        {
                            // axis5_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.axis5_label = obj;
                            lv_obj_set_pos(obj, 3, 504);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_axis(obj);
                            lv_label_set_text(obj, "C");
                        }
                        {
                            // Axis4_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.axis4_label = obj;
                            lv_obj_set_pos(obj, 3, 388);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_axis(obj);
                            lv_label_set_text(obj, "W");
                        }
                        {
                            // Axis3_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.axis3_label = obj;
                            lv_obj_set_pos(obj, 3, 272);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_axis(obj);
                            lv_label_set_text(obj, "Z");
                        }
                        {
                            // axis2_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.axis2_label = obj;
                            lv_obj_set_pos(obj, 5, 156);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_axis(obj);
                            lv_label_set_text(obj, "Y");
                        }
                        {
                            // axis1_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.axis1_label = obj;
                            lv_obj_set_pos(obj, 5, 40);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_axis(obj);
                            lv_label_set_text(obj, "X");
                        }
                        {
                            // numpad
                            lv_obj_t *obj = lv_keyboard_create(parent_obj);
                            objects.numpad = obj;
                            lv_obj_set_pos(obj, 797, 194);
                            lv_obj_set_size(obj, 442, 414);
                            lv_keyboard_set_mode(obj, LV_KEYBOARD_MODE_NUMBER);
                            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
                {
                    // Functions Tab (Was Tab 2)
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Functions");
                    add_style_tab_style(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xf9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
                    
                    // Get the tab button and add explicit event handler
                    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.obj0);
                    lv_obj_t *tab_button = lv_obj_get_child_by_type(tab_bar, 1, &lv_button_class);
                    if (tab_button) {
                        lv_obj_add_event_cb(tab_button, tab_button_event_handler, LV_EVENT_CLICKED, (void*)1);
                    }
                    
                    create_functions_tab_ui(obj);
                }
                {
                    // Tool Library Tab (Was Tab 3)
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Tools");
                    add_style_tab_style(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xf9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
                    
                    // Get the tab button and add explicit event handler
                    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.obj0);
                    lv_obj_t *tab_button = lv_obj_get_child_by_type(tab_bar, 2, &lv_button_class);
                    if (tab_button) {
                        lv_obj_add_event_cb(tab_button, tab_button_event_handler, LV_EVENT_CLICKED, (void*)2);
                    }
                    
                    create_tool_library_tab_ui(obj);
                }
                {
                    // E-Screw Tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "E-Screw");
                    add_style_tab_style(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xf9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
                    
                    // Get the tab button and add explicit event handler
                    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.obj0);
                    lv_obj_t *tab_button = lv_obj_get_child_by_type(tab_bar, 3, &lv_button_class);
                    if (tab_button) {
                        lv_obj_add_event_cb(tab_button, tab_button_event_handler, LV_EVENT_CLICKED, (void*)3);
                    }
                    
                    // Create E-Screw screen content
                    create_e_screw_screen(obj);
                }
            }
        }
    }
    
    // tick_screen_main(); // logic moved to ui_tick_task
}

void tick_screen_main() {
    // Get state for precision checks
    const dro_system_state_t* state = dro_get_state();

    // Update status labels
    if (objects.mode_status_label) {
        lv_label_set_text(objects.mode_status_label, get_var_mode_text());
    }
    if (objects.conn_status_label) {
        lv_label_set_text(objects.conn_status_label, get_var_conn_status_text());
    }
    
    // Update Unit Labels (Dynamic based on settings)
    if (objects.mm_x_axis1_label) lv_label_set_text(objects.mm_x_axis1_label, dro_get_axis_unit_name(0));
    if (objects.mm_x_axis2_label) lv_label_set_text(objects.mm_x_axis2_label, dro_get_axis_unit_name(1));
    if (objects.mm_x_axis3_label) lv_label_set_text(objects.mm_x_axis3_label, dro_get_axis_unit_name(2));
    if (objects.mm_x_axis4_label) lv_label_set_text(objects.mm_x_axis4_label, dro_get_axis_unit_name(3));
    if (objects.mm_x_axis5_label) lv_label_set_text(objects.mm_x_axis5_label, dro_get_axis_unit_name(4));

    // Update E-Screw screen
    tick_e_screw_screen();
    
    {
        if (objects.obj1) {
            char new_val_str[16];
            snprintf(new_val_str, sizeof(new_val_str), "%ld", (long)get_var_active_space_number());
            const char *cur_val = lv_textarea_get_text(objects.obj1);
            uint32_t max_length = lv_textarea_get_max_length(objects.obj1);
            if (cur_val && strncmp(new_val_str, cur_val, max_length) != 0) {
                tick_value_change_obj = objects.obj1;
                lv_textarea_set_text(objects.obj1, new_val_str);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.active_tool_number) {
            char new_val_str[16];
            snprintf(new_val_str, sizeof(new_val_str), "%ld", (long)get_var_active_tool_number());
            const char *cur_val = lv_textarea_get_text(objects.active_tool_number);
            uint32_t max_length = lv_textarea_get_max_length(objects.active_tool_number);
            if (cur_val && strncmp(new_val_str, cur_val, max_length) != 0) {
                tick_value_change_obj = objects.active_tool_number;
                lv_textarea_set_text(objects.active_tool_number, new_val_str);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.axis1_textarea) {
            int precision = (state && state->axis_configs[0].type == DRO_AXIS_TYPE_ROTARY) ? 4 : get_var_axis_precision();
            char new_val_str[32];
            snprintf(new_val_str, sizeof(new_val_str), "%.*f", precision, get_var_virtual_axis_1());
            const char *cur_val = lv_textarea_get_text(objects.axis1_textarea);
            uint32_t max_length = lv_textarea_get_max_length(objects.axis1_textarea);
            if (cur_val && strncmp(new_val_str, cur_val, max_length) != 0) {
                tick_value_change_obj = objects.axis1_textarea;
                lv_textarea_set_text(objects.axis1_textarea, new_val_str);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.axis2_textarea) {
            int precision = (state && state->axis_configs[1].type == DRO_AXIS_TYPE_ROTARY) ? 4 : get_var_axis_precision();
            char new_val_str[32];
            snprintf(new_val_str, sizeof(new_val_str), "%.*f", precision, get_var_virtual_axis_2());
            const char *cur_val = lv_textarea_get_text(objects.axis2_textarea);
            uint32_t max_length = lv_textarea_get_max_length(objects.axis2_textarea);
            if (cur_val && strncmp(new_val_str, cur_val, max_length) != 0) {
                tick_value_change_obj = objects.axis2_textarea;
                lv_textarea_set_text(objects.axis2_textarea, new_val_str);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.axis3_textarea) {
            int precision = (state && state->axis_configs[2].type == DRO_AXIS_TYPE_ROTARY) ? 4 : get_var_axis_precision();
            char new_val_str[32];
            snprintf(new_val_str, sizeof(new_val_str), "%.*f", precision, get_var_virtual_axis_3());
            const char *cur_val = lv_textarea_get_text(objects.axis3_textarea);
            uint32_t max_length = lv_textarea_get_max_length(objects.axis3_textarea);
            if (cur_val && strncmp(new_val_str, cur_val, max_length) != 0) {
                tick_value_change_obj = objects.axis3_textarea;
                lv_textarea_set_text(objects.axis3_textarea, new_val_str);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.axis4_textarea) {
            int precision = (state && state->axis_configs[3].type == DRO_AXIS_TYPE_ROTARY) ? 4 : get_var_axis_precision();
            char new_val_str[32];
            snprintf(new_val_str, sizeof(new_val_str), "%.*f", precision, get_var_virtual_axis_4());
            const char *cur_val = lv_textarea_get_text(objects.axis4_textarea);
            uint32_t max_length = lv_textarea_get_max_length(objects.axis4_textarea);
            if (cur_val && strncmp(new_val_str, cur_val, max_length) != 0) {
                tick_value_change_obj = objects.axis4_textarea;
                lv_textarea_set_text(objects.axis4_textarea, new_val_str);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (objects.axis5_textarea) {
            int precision = (state && state->axis_configs[4].type == DRO_AXIS_TYPE_ROTARY) ? 4 : get_var_axis_precision();
            char new_val_str[32];
            snprintf(new_val_str, sizeof(new_val_str), "%.*f", precision, get_var_virtual_axis_5());
            const char *cur_val = lv_textarea_get_text(objects.axis5_textarea);
            uint32_t max_length = lv_textarea_get_max_length(objects.axis5_textarea);
            if (cur_val && strncmp(new_val_str, cur_val, max_length) != 0) {
                tick_value_change_obj = objects.axis5_textarea;
                lv_textarea_set_text(objects.axis5_textarea, new_val_str);
                tick_value_change_obj = NULL;
            }
        }
    }
}

static void event_handler_back_to_main(lv_event_t *e) {
    lv_scr_load(objects.main);
}

void create_screen_setings_page() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.setings_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 1280, 800);
    {
        lv_obj_t *parent_obj = obj;
        {
            // Setting_page
            lv_obj_t *obj = lv_tabview_create(parent_obj);
            objects.setting_page = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 1280, 800);
            lv_tabview_set_tab_bar_position(obj, LV_DIR_TOP);
            lv_tabview_set_tab_bar_size(obj, 60);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xf9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // axis1_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 1");
                    objects.axis1_tab = obj;
                    add_style_tab_style(obj);
                    create_axis_settings_ui(obj, 0);
                }
                {
                    // axis2
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 2");
                    objects.axis2 = obj;
                    create_axis_settings_ui(obj, 1);
                }
                {
                    // axis3_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 3");
                    objects.axis3_tab = obj;
                    create_axis_settings_ui(obj, 2);
                }
                {
                    // axis4_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 4");
                    objects.axis4_tab = obj;
                    create_axis_settings_ui(obj, 3);
                }
                {
                    // axis5_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 5");
                    objects.axis5_tab = obj;
                    create_axis_settings_ui(obj, 4);
                }
                {
                    // general_settings_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "General");
                    objects.general_settings_tab = obj;
                    
                    // Return Button for General Tab
                    {
                        lv_obj_t *btn_back = lv_btn_create(obj);
                        lv_obj_set_pos(btn_back, 1150, 10); // Top right
                        lv_obj_set_size(btn_back, 100, 40);
                        lv_obj_add_event_cb(btn_back, event_handler_back_to_main, LV_EVENT_CLICKED, NULL);
                        lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xFF0000), 0);
                        
                        lv_obj_t *lbl = lv_label_create(btn_back);
                        lv_obj_center(lbl);
                        lv_label_set_text(lbl, "RETURN");
                    }
                    // Global Settings Controls (Moved here to be inside General Tab)
                    {
                        lv_obj_t *obj = lv_label_create(objects.general_settings_tab);
                        lv_obj_set_pos(obj, 43, 96);
                        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                        add_style_setting_page_label(obj);
                        lv_label_set_text(obj, "Liniar axes in mm");
                    }
                    {
                        // settings_mm_switch
                        lv_obj_t *obj = lv_switch_create(objects.general_settings_tab);
                        objects.settings_mm_switch = obj;
                        lv_obj_set_pos(obj, 934, 106);
                        lv_obj_set_size(obj, 50, 25);
                        lv_obj_add_event_cb(obj, action_settings_mm_switch_pressed, LV_EVENT_CLICKED, (void *)0);
                        lv_obj_add_state(obj, LV_STATE_CHECKED);
                    }
                    {
                        // High Precision Switch
                        lv_obj_t *obj = lv_switch_create(objects.general_settings_tab);
                        objects.settings_high_precision_switch = obj;
                        lv_obj_set_pos(obj, 934, 150); // Below MM switch
                        lv_obj_set_size(obj, 50, 25);
                        
                        // Set initial state
                        if (dro_get_state()->high_precision) {
                            lv_obj_add_state(obj, LV_STATE_CHECKED);
                        }

                        lv_obj_add_event_cb(obj, event_handler_high_precision_toggle, LV_EVENT_VALUE_CHANGED, NULL);
                        
                        lv_obj_t *lbl = lv_label_create(objects.general_settings_tab);
                        lv_obj_set_pos(lbl, 43, 155);
                        add_style_setting_page_label(lbl); // Reuse style
                        lv_label_set_text(lbl, "High Precision");
                    }
                    
                    {
                        // Machine Type Label
                        lv_obj_t *lbl = lv_label_create(objects.general_settings_tab);
                        lv_obj_set_pos(lbl, 43, 210);
                        add_style_setting_page_label(lbl);
                        lv_label_set_text(lbl, "Machine Type");
                        
                        // Machine Type Dropdown
                        lv_obj_t *dd = lv_dropdown_create(objects.general_settings_tab);
                        lv_dropdown_set_options(dd, "Lathe\nMill\nGrinder\nEDM");
                        lv_obj_set_pos(dd, 934, 205);
                        lv_obj_set_width(dd, 150);
                        lv_obj_set_style_text_font(dd, &lv_font_montserrat_22, 0);
                        
                        // Set current selection
                        lv_dropdown_set_selected(dd, (uint16_t)dro_get_state()->machine_type);
                        
                        lv_obj_add_event_cb(dd, event_handler_machine_type_change, LV_EVENT_VALUE_CHANGED, NULL);
                    }

                    {
                        // Axis Count Label
                        lv_obj_t *lbl = lv_label_create(objects.general_settings_tab);
                        lv_obj_set_pos(lbl, 43, 270);
                        add_style_setting_page_label(lbl);
                        lv_label_set_text(lbl, "Number of Axes");
                        
                        // Axis Count Dropdown
                        lv_obj_t *dd = lv_dropdown_create(objects.general_settings_tab);
                        lv_dropdown_set_options(dd, "1 Axis\n2 Axes\n3 Axes\n4 Axes\n5 Axes");
                        lv_obj_set_pos(dd, 934, 265);
                        lv_obj_set_width(dd, 150);
                        lv_obj_set_style_text_font(dd, &lv_font_montserrat_22, 0);
                        
                        // Set current selection (count - 1)
                        uint8_t count = dro_get_state()->active_axis_count;
                        if (count > 0) count--; 
                        lv_dropdown_set_selected(dd, count);
                        
                        lv_obj_add_event_cb(dd, event_handler_axis_count_change, LV_EVENT_VALUE_CHANGED, NULL);
                    }
                }
            }
        }
    }
    
    tick_screen_setings_page();
}

void tick_screen_setings_page() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_setings_page,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
    create_screen_setings_page();
}
