#include "e_screw_screen.h"
#include "frontend_host.h"
#include "dro_core.h"
#include "machine_params.h"
#include "screw_calc.h"
#include "cone_calc.h"
#include "axis_mapping.h"
#include <string.h>
#include <stdio.h>

// UI Objects - Per-axis ratio inputs
static lv_obj_t *z_numerator_input = NULL;
static lv_obj_t *z_denominator_input = NULL;
static lv_obj_t *x_numerator_input = NULL;
static lv_obj_t *x_denominator_input = NULL;
static lv_obj_t *enable_switch = NULL;
static lv_obj_t *work_mode_dropdown = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *rpm_label = NULL;
static lv_obj_t *position_label = NULL;
static lv_obj_t *enabled_label = NULL;

// Virtual axis selection dropdowns
static lv_obj_t *z_axis_dropdown = NULL;
static lv_obj_t *x_axis_dropdown = NULL;

// Machine parameter inputs
static lv_obj_t *leadscrew_input = NULL;
static lv_obj_t *motor_steps_input = NULL;
static lv_obj_t *encoder_counts_input = NULL;

// Calculation inputs
static lv_obj_t *pitch_input = NULL;  // For Screw mode
static lv_obj_t *angle_input = NULL;  // For Cone mode

// Result displays
static lv_obj_t *result_label = NULL;

// Event Handlers
static void send_ratio_button_event(lv_event_t *e) {
    bool track = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
    
    // Get Z-axis ratio from inputs
    const char *z_num_str = lv_textarea_get_text(z_numerator_input);
    const char *z_den_str = lv_textarea_get_text(z_denominator_input);
    
    uint32_t z_num = atoi(z_num_str);
    uint32_t z_den = atoi(z_den_str);
    
    if (z_num > 0 && z_den > 0) {
        float ratio = (float)z_num / (float)z_den;
        uint16_t axis_id = lv_dropdown_get_selected(z_axis_dropdown);
        
        // TODO: Map Accel and Max_Hz to actual UI inputs later
        frontend_usb_update_els_config(axis_id, ratio, track, 50, 30000);
    }
    
    // Get X-axis ratio from inputs (for cone mode)
    const char *x_num_str = lv_textarea_get_text(x_numerator_input);
    const char *x_den_str = lv_textarea_get_text(x_denominator_input);
    
    uint32_t x_num = atoi(x_num_str);
    uint32_t x_den = atoi(x_den_str);
    
    if (x_num > 0 && x_den > 0) {
        float ratio = (float)x_num / (float)x_den;
        uint16_t axis_id = lv_dropdown_get_selected(x_axis_dropdown);
        frontend_usb_update_els_config(axis_id, ratio, track, 50, 30000);
    }
}

static void enable_switch_event(lv_event_t *e) {
    // Re-trigger the send ratio event to push the new Enable boolean
    send_ratio_button_event(NULL);
}

static void work_mode_dropdown_event(lv_event_t *e) {
    // Work mode logic (calculators) does not need to send USB commands directly.
    // The user must click "Send Ratio" to commit.
}

static void z_axis_dropdown_event(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dd);
    axis_mapping_set_z_axis(selected);
}

static void x_axis_dropdown_event(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dd);
    axis_mapping_set_x_axis(selected);
}

static void calculate_button_event(lv_event_t *e) {
    // Get current work mode
    uint16_t mode = lv_dropdown_get_selected(work_mode_dropdown);
    
    if (mode == 0) {  // Screw mode
        // Get pitch from input
        const char *pitch_str = lv_textarea_get_text(pitch_input);
        float pitch_mm = atof(pitch_str);
        
        if (pitch_mm > 0) {
            uint32_t num, den;
            screw_calc_ratio(pitch_mm, &num, &den);
            
            // Update Z-axis inputs
            char num_buf[16], den_buf[16];
            snprintf(num_buf, sizeof(num_buf), "%lu", (unsigned long)num);
            snprintf(den_buf, sizeof(den_buf), "%lu", (unsigned long)den);
            lv_textarea_set_text(z_numerator_input, num_buf);
            lv_textarea_set_text(z_denominator_input, den_buf);
            
            // Update result label
            lv_label_set_text_fmt(result_label, "Calculated: %lu/%lu for %.3fmm pitch", (unsigned long)num, (unsigned long)den, pitch_mm);
        }
    } else if (mode == 2) {  // Conical mode
        // Get angle from input
        const char *angle_str = lv_textarea_get_text(angle_input);
        float angle_deg = atof(angle_str);
        
        if (angle_deg > 0 && angle_deg < 90) {
            uint32_t z_num, z_den, x_num, x_den;
            cone_calc_ratio(angle_deg, &z_num, &z_den, &x_num, &x_den);
            
            // Update Z-axis inputs
            char z_num_buf[16], z_den_buf[16];
            snprintf(z_num_buf, sizeof(z_num_buf), "%lu", (unsigned long)z_num);
            snprintf(z_den_buf, sizeof(z_den_buf), "%lu", (unsigned long)z_den);
            lv_textarea_set_text(z_numerator_input, z_num_buf);
            lv_textarea_set_text(z_denominator_input, z_den_buf);
            
            // Update X-axis inputs
            char x_num_buf[16], x_den_buf[16];
            snprintf(x_num_buf, sizeof(x_num_buf), "%lu", (unsigned long)x_num);
            snprintf(x_den_buf, sizeof(x_den_buf), "%lu", (unsigned long)x_den);
            lv_textarea_set_text(x_numerator_input, x_num_buf);
            lv_textarea_set_text(x_denominator_input, x_den_buf);
            
            // Update result label
            lv_label_set_text_fmt(result_label, "Z: %lu/%lu, X: %lu/%lu for %.1f° angle", 
                                  (unsigned long)z_num, (unsigned long)z_den, (unsigned long)x_num, (unsigned long)x_den, angle_deg);
        }
    }
}

static void save_settings_event(lv_event_t *e) {
    // Save machine parameters to NVS
    const char *ls_str = lv_textarea_get_text(leadscrew_input);
    const char *ms_str = lv_textarea_get_text(motor_steps_input);
    const char *ec_str = lv_textarea_get_text(encoder_counts_input);
    
    float ls_pitch = atof(ls_str);
    uint32_t motor_steps = atoi(ms_str);
    uint32_t enc_counts = atoi(ec_str);
    
    if (ls_pitch > 0) machine_params_set_leadscrew_pitch(ls_pitch);
    if (motor_steps > 0) machine_params_set_motor_steps(motor_steps);
    if (enc_counts > 0) machine_params_set_encoder_counts(enc_counts);
}

void create_e_screw_screen(lv_obj_t *parent_obj) {
    // Title
    lv_obj_t *title = lv_label_create(parent_obj);
    lv_label_set_text(title, "Electronic Lead Screw Settings");
    lv_obj_set_pos(title, 20, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    
    // ELS Present Switch
    lv_obj_t *els_present_label = lv_label_create(parent_obj);
    lv_label_set_text(els_present_label, "ELS Hardware:");
    lv_obj_set_pos(els_present_label, 20, 70);
    lv_obj_set_style_text_font(els_present_label, &lv_font_montserrat_20, 0);

    lv_obj_t *els_present_switch = lv_switch_create(parent_obj);
    lv_obj_set_pos(els_present_switch, 180, 65);
    if (machine_params_get_is_els_present()) {
        lv_obj_add_state(els_present_switch, LV_STATE_CHECKED);
    }
    // TODO: Add callback to save setting and toggle visibility of other controls

    // Work Mode Dropdown
    lv_obj_t *mode_label = lv_label_create(parent_obj);
    lv_label_set_text(mode_label, "Work Mode:");
    lv_obj_set_pos(mode_label, 280, 70); // Shifted right
    lv_obj_set_style_text_font(mode_label, &lv_font_montserrat_20, 0);
    
    work_mode_dropdown = lv_dropdown_create(parent_obj);
    lv_dropdown_set_options(work_mode_dropdown, "Screw\\nFollow\\nConical");
    lv_obj_set_pos(work_mode_dropdown, 410, 65); // Shifted right
    lv_obj_set_size(work_mode_dropdown, 150, 40);
    lv_obj_add_event_cb(work_mode_dropdown, work_mode_dropdown_event, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Enable Switch
    lv_obj_t *enable_label = lv_label_create(parent_obj);
    lv_label_set_text(enable_label, "Enable:");
    lv_obj_set_pos(enable_label, 600, 70); // Shifted right
    lv_obj_set_style_text_font(enable_label, &lv_font_montserrat_20, 0);
    
    enable_switch = lv_switch_create(parent_obj);
    lv_obj_set_pos(enable_switch, 690, 65); // Shifted right
    lv_obj_add_event_cb(enable_switch, enable_switch_event, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Z-Axis Ratio Section
    lv_obj_t *z_ratio_title = lv_label_create(parent_obj);
    lv_label_set_text(z_ratio_title, "Z-Axis Ratio (Longitudinal)");
    lv_obj_set_pos(z_ratio_title, 20, 130);
    lv_obj_set_style_text_font(z_ratio_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(z_ratio_title, lv_color_hex(0x00BFFF), 0);
    
    // Z Virtual Axis Dropdown
    lv_obj_t *z_virt_label = lv_label_create(parent_obj);
    lv_label_set_text(z_virt_label, "Virtual Axis:");
    lv_obj_set_pos(z_virt_label, 300, 135);
    
    z_axis_dropdown = lv_dropdown_create(parent_obj);
    lv_obj_set_pos(z_axis_dropdown, 420, 130);
    lv_obj_set_size(z_axis_dropdown, 140, 40);
    
    // Build dropdown options from custom axis names
    char z_dropdown_opts[128] = "";
    for (uint8_t i = 0; i < 5; i++) {
        const char *name = axis_mapping_get_axis_name(i);
        strcat(z_dropdown_opts, name);
        if (i < 4) strcat(z_dropdown_opts, "\n");
    }
    lv_dropdown_set_options(z_axis_dropdown, z_dropdown_opts);
    lv_dropdown_set_selected(z_axis_dropdown, axis_mapping_get_z_axis());
    lv_obj_add_event_cb(z_axis_dropdown, z_axis_dropdown_event, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Z Numerator
    lv_obj_t *z_num_label = lv_label_create(parent_obj);
    lv_label_set_text(z_num_label, "Numerator:");
    lv_obj_set_pos(z_num_label, 20, 165);
    
    z_numerator_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(z_numerator_input, 130, 160);
    lv_obj_set_size(z_numerator_input, 100, 40);
    lv_textarea_set_one_line(z_numerator_input, true);
    lv_textarea_set_placeholder_text(z_numerator_input, "1");
    lv_textarea_set_text(z_numerator_input, "1");
    
    // Z Denominator
    lv_obj_t *z_den_label = lv_label_create(parent_obj);
    lv_label_set_text(z_den_label, "Denominator:");
    lv_obj_set_pos(z_den_label, 250, 165);
    
    z_denominator_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(z_denominator_input, 370, 160);
    lv_obj_set_size(z_denominator_input, 100, 40);
    lv_textarea_set_one_line(z_denominator_input, true);
    lv_textarea_set_placeholder_text(z_denominator_input, "1");
    lv_textarea_set_text(z_denominator_input, "1");
    
    // X-Axis Ratio Section
    lv_obj_t *x_ratio_title = lv_label_create(parent_obj);
    lv_label_set_text(x_ratio_title, "X-Axis Ratio (Cross-slide)");
    lv_obj_set_pos(x_ratio_title, 20, 220);
    lv_obj_set_style_text_font(x_ratio_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(x_ratio_title, lv_color_hex(0x00BFFF), 0);
    
    // X Virtual Axis Dropdown
    lv_obj_t *x_virt_label = lv_label_create(parent_obj);
    lv_label_set_text(x_virt_label, "Virtual Axis:");
    lv_obj_set_pos(x_virt_label, 300, 225);
    
    x_axis_dropdown = lv_dropdown_create(parent_obj);
    lv_obj_set_pos(x_axis_dropdown, 420, 220);
    lv_obj_set_size(x_axis_dropdown, 140, 40);
    
    // Build dropdown options from custom axis names
    char x_dropdown_opts[128] = "";
    for (uint8_t i = 0; i < 5; i++) {
        const char *name = axis_mapping_get_axis_name(i);
        strcat(x_dropdown_opts, name);
        if (i < 4) strcat(x_dropdown_opts, "\n");
    }
    lv_dropdown_set_options(x_axis_dropdown, x_dropdown_opts);
    lv_dropdown_set_selected(x_axis_dropdown, axis_mapping_get_x_axis());
    lv_obj_add_event_cb(x_axis_dropdown, x_axis_dropdown_event, LV_EVENT_VALUE_CHANGED, NULL);
    
    // X Numerator
    lv_obj_t *x_num_label = lv_label_create(parent_obj);
    lv_label_set_text(x_num_label, "Numerator:");
    lv_obj_set_pos(x_num_label, 20, 255);
    
    x_numerator_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(x_numerator_input, 130, 250);
    lv_obj_set_size(x_numerator_input, 100, 40);
    lv_textarea_set_one_line(x_numerator_input, true);
    lv_textarea_set_placeholder_text(x_numerator_input, "1");
    lv_textarea_set_text(x_numerator_input, "1");
    
    // X Denominator
    lv_obj_t *x_den_label = lv_label_create(parent_obj);
    lv_label_set_text(x_den_label, "Denominator:");
    lv_obj_set_pos(x_den_label, 250, 255);
    
    x_denominator_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(x_denominator_input, 370, 250);
    lv_obj_set_size(x_denominator_input, 100, 40);
    lv_textarea_set_one_line(x_denominator_input, true);
    lv_textarea_set_placeholder_text(x_denominator_input, "1");
    lv_textarea_set_text(x_denominator_input, "1");
    
    // Send Ratio Button
    lv_obj_t *send_ratio_btn = lv_button_create(parent_obj);
    lv_obj_set_pos(send_ratio_btn, 20, 310);
    lv_obj_set_size(send_ratio_btn, 200, 60);
    lv_obj_add_event_cb(send_ratio_btn, send_ratio_button_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *send_ratio_label = lv_label_create(send_ratio_btn);
    lv_label_set_text(send_ratio_label, "Send Ratio to FPGA");
    lv_obj_center(send_ratio_label);
    
    // Status Label
    status_label = lv_label_create(parent_obj);
    lv_label_set_text(status_label, "Status: Idle");
    lv_obj_set_pos(status_label, 20, 390);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
    
    // Current RPM Display
    rpm_label = lv_label_create(parent_obj);
    lv_label_set_text(rpm_label, "RPM: 0");
    lv_obj_set_pos(rpm_label, 20, 430);
    lv_obj_set_style_text_font(rpm_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(rpm_label, lv_color_hex(0xFFD700), 0);
    
    // Current Position Display
    position_label = lv_label_create(parent_obj);
    lv_label_set_text(position_label, "Position: 0");
    lv_obj_set_pos(position_label, 20, 460);
    lv_obj_set_style_text_font(position_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(position_label, lv_color_hex(0xFFD700), 0);
    
    // Enabled Status Display
    enabled_label = lv_label_create(parent_obj);
    lv_label_set_text(enabled_label, "Enabled: No");
    lv_obj_set_pos(enabled_label, 20, 490);
    lv_obj_set_style_text_font(enabled_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(enabled_label, lv_color_hex(0xFF6B6B), 0);
    
    // Machine Settings Panel (right side, column layout)
    lv_obj_t *settings_title = lv_label_create(parent_obj);
    lv_label_set_text(settings_title, "Machine Settings");
    lv_obj_set_pos(settings_title, 800, 10);
    lv_obj_set_style_text_font(settings_title, &lv_font_montserrat_20, 0);
    
    // Leadscrew Pitch
    lv_obj_t *ls_label = lv_label_create(parent_obj);
    lv_label_set_text(ls_label, "Leadscrew Pitch (mm):");
    lv_obj_set_pos(ls_label, 800, 50);
    
    leadscrew_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(leadscrew_input, 1000, 45);
    lv_obj_set_size(leadscrew_input, 100, 40);
    lv_textarea_set_one_line(leadscrew_input, true);
    char ls_buf[16];
    snprintf(ls_buf, sizeof(ls_buf), "%.2f", machine_params_get_leadscrew_pitch());
    lv_textarea_set_text(leadscrew_input, ls_buf);
    
    // Motor Steps
    lv_obj_t *ms_label = lv_label_create(parent_obj);
    lv_label_set_text(ms_label, "Motor Steps/Rev:");
    lv_obj_set_pos(ms_label, 800, 100);
    
    motor_steps_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(motor_steps_input, 1000, 95);
    lv_obj_set_size(motor_steps_input, 100, 40);
    lv_textarea_set_one_line(motor_steps_input, true);
    char ms_buf[16];
    snprintf(ms_buf, sizeof(ms_buf), "%lu", (unsigned long)machine_params_get_motor_steps());
    lv_textarea_set_text(motor_steps_input, ms_buf);
    
    // Encoder Counts
    lv_obj_t *ec_label = lv_label_create(parent_obj);
    lv_label_set_text(ec_label, "Encoder Counts/Rev:");
    lv_obj_set_pos(ec_label, 800, 150);
    
    encoder_counts_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(encoder_counts_input, 1000, 145);
    lv_obj_set_size(encoder_counts_input, 100, 40);
    lv_textarea_set_one_line(encoder_counts_input, true);
    char ec_buf[16];
    snprintf(ec_buf, sizeof(ec_buf), "%lu", (unsigned long)machine_params_get_encoder_counts());
    lv_textarea_set_text(encoder_counts_input, ec_buf);
    
    // Save Settings Button
    lv_obj_t *save_btn = lv_button_create(parent_obj);
    lv_obj_set_pos(save_btn, 1120, 45);
    lv_obj_set_size(save_btn, 120, 40);
    lv_obj_add_event_cb(save_btn, save_settings_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_center(save_label);
    
    // Calculation Section
    lv_obj_t *calc_title = lv_label_create(parent_obj);
    lv_label_set_text(calc_title, "Calculate Ratio");
    lv_obj_set_pos(calc_title, 800, 210);
    lv_obj_set_style_text_font(calc_title, &lv_font_montserrat_20, 0);
    
    // Pitch Input (for Screw mode)
    lv_obj_t *pitch_label = lv_label_create(parent_obj);
    lv_label_set_text(pitch_label, "Target Pitch (mm):");
    lv_obj_set_pos(pitch_label, 800, 250);
    
    pitch_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(pitch_input, 1000, 245);
    lv_obj_set_size(pitch_input, 100, 40);
    lv_textarea_set_one_line(pitch_input, true);
    lv_textarea_set_placeholder_text(pitch_input, "0.5");
    
    // Angle Input (for Cone mode)
    lv_obj_t *angle_label = lv_label_create(parent_obj);
    lv_label_set_text(angle_label, "Taper Angle (deg):");
    lv_obj_set_pos(angle_label, 800, 300);
    
    angle_input = lv_textarea_create(parent_obj);
    lv_obj_set_pos(angle_input, 1000, 295);
    lv_obj_set_size(angle_input, 100, 40);
    lv_textarea_set_one_line(angle_input, true);
    lv_textarea_set_placeholder_text(angle_input, "30.0");
    
    // Calculate Button
    lv_obj_t *calc_btn = lv_button_create(parent_obj);
    lv_obj_set_pos(calc_btn, 1120, 245);
    lv_obj_set_size(calc_btn, 120, 90);
    lv_obj_add_event_cb(calc_btn, calculate_button_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *calc_btn_label = lv_label_create(calc_btn);
    lv_label_set_text(calc_btn_label, "Calculate");
    lv_obj_center(calc_btn_label);
    
    // Result Label
    result_label = lv_label_create(parent_obj);
    lv_label_set_text(result_label, "Enter pitch or angle and click Calculate");
    lv_obj_set_pos(result_label, 800, 350);
    lv_obj_set_style_text_font(result_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(result_label, lv_color_hex(0x00BFFF), 0);
}

void tick_e_screw_screen(void) {
    if (!status_label) return;  // Not initialized yet
    
    // Update status based on Native USB connection
    if (frontend_usb_is_connected()) {
        uint16_t mode = lv_dropdown_get_selected(work_mode_dropdown);
        const char *mode_names[] = {"Screw", "Follow", "Conical"};
        const char *mode_name = (mode < 3) ? mode_names[mode] : "Unknown";
        
        lv_label_set_text_fmt(status_label, "Status: Connected (USB) | %s", mode_name);
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(status_label, "Status: Disconnected");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0);
    }
    
    // Fetch Spindle Metrics from DRO core
    const dro_system_state_t *dr_state = dro_get_state();
    
    // Update RPM
    if (rpm_label) {
        lv_label_set_text_fmt(rpm_label, "Spindle RPM: %.1f", dr_state->current_spindle_rpm);
    }
    
    // Update Position
    if (position_label) {
        lv_label_set_text_fmt(position_label, "Spindle (Counts): %ld", (long)dr_state->current_spindle_counts);
    }
    
    // Update Enabled Status
    if (enabled_label) {
        if (lv_obj_has_state(enable_switch, LV_STATE_CHECKED)) {
            lv_label_set_text(enabled_label, "Enabled: Yes (Tracking)");
            lv_obj_set_style_text_color(enabled_label, lv_color_hex(0x00FF00), 0);
        } else {
            lv_label_set_text(enabled_label, "Enabled: No (Idling)");
            lv_obj_set_style_text_color(enabled_label, lv_color_hex(0xFF6B6B), 0);
        }
    }
}

void refresh_escrew_axis_dropdowns(void) {
    if (!z_axis_dropdown || !x_axis_dropdown) return;
    
    // Rebuild dropdown options from updated axis names
    char dropdown_opts[128] = "";
    for (uint8_t i = 0; i < 5; i++) {
        const char *name = axis_mapping_get_axis_name(i);
        strcat(dropdown_opts, name);
        if (i < 4) strcat(dropdown_opts, "\n");
    }
    
    // Get current selections
    uint16_t z_selected = lv_dropdown_get_selected(z_axis_dropdown);
    uint16_t x_selected = lv_dropdown_get_selected(x_axis_dropdown);
    
    // Update dropdown options
    lv_dropdown_set_options(z_axis_dropdown, dropdown_opts);
    lv_dropdown_set_options(x_axis_dropdown, dropdown_opts);
    
    // Restore selections
    lv_dropdown_set_selected(z_axis_dropdown, z_selected);
    lv_dropdown_set_selected(x_axis_dropdown, x_selected);
}
