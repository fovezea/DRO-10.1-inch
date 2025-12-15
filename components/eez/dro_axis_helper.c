#include "dro_axis_helper.h"
#include "generated/screens.h"
#include "axis_mapping.h"
#include "e_screw_screen.h"
#include <string.h>

// Rename dialog objects
static lv_obj_t *rename_dialog = NULL;
static lv_obj_t *rename_textarea = NULL;
static uint8_t current_editing_axis = 0;

// Forward declarations
static void axis_label_click_event(lv_event_t *e);
static void rename_dialog_ok_event(lv_event_t *e);
static void rename_dialog_cancel_event(lv_event_t *e);

void dro_axis_helper_init(void) {
    // Add click handlers to the 5 axis labels
    lv_obj_t *axis_labels[] = {
        objects.axis1_label,  // Virtual Axis 0 (X)
        objects.axis2_label,  // Virtual Axis 1 (Y)
        objects.axis3_label,  // Virtual Axis 2 (Z)
        objects.axis4_label,  // Virtual Axis 3 (C)
        objects.axis5_label   // Virtual Axis 4 (W)
    };
    
    for (uint8_t i = 0; i < 5; i++) {
        if (axis_labels[i]) {
            lv_obj_add_flag(axis_labels[i], LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(axis_labels[i], axis_label_click_event, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        }
    }
    
    // Initialize axis labels with custom names
    for (uint8_t i = 0; i < 5; i++) {
        const char *name = axis_mapping_get_axis_name(i);
        if (axis_labels[i] && name) {
            lv_label_set_text(axis_labels[i], name);
        }
    }
}

static void axis_label_click_event(lv_event_t *e) {
    uint8_t axis_index = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    current_editing_axis = axis_index;
    
    // Create rename dialog (larger to fit keyboard)
    rename_dialog = lv_obj_create(lv_screen_active());
    lv_obj_set_size(rename_dialog, 500, 580);
    lv_obj_center(rename_dialog);
    lv_obj_set_style_bg_color(rename_dialog, lv_color_hex(0x303030), 0);
    lv_obj_set_style_border_color(rename_dialog, lv_color_hex(0x00BFFF), 0);
    lv_obj_set_style_border_width(rename_dialog, 2, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(rename_dialog);
    lv_label_set_text_fmt(title, "Rename Axis %d", axis_index);
    lv_obj_set_pos(title, 20, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    
    // Current name label
    lv_obj_t *current_label = lv_label_create(rename_dialog);
    lv_label_set_text_fmt(current_label, "Current: %s", axis_mapping_get_axis_name(axis_index));
    lv_obj_set_pos(current_label, 20, 55);
    lv_obj_set_style_text_color(current_label, lv_color_hex(0xAAAAAA), 0);
    
    // Text input
    rename_textarea = lv_textarea_create(rename_dialog);
    lv_obj_set_pos(rename_textarea, 20, 90);
    lv_obj_set_size(rename_textarea, 460, 50);
    lv_textarea_set_one_line(rename_textarea, true);
    lv_textarea_set_max_length(rename_textarea, AXIS_NAME_MAX_LEN);
    // Start with empty text field, show current name as placeholder
    lv_textarea_set_text(rename_textarea, "");
    lv_textarea_set_placeholder_text(rename_textarea, axis_mapping_get_axis_name(axis_index));
    
    // Keyboard (positioned directly below the input field)
    lv_obj_t *keyboard = lv_keyboard_create(rename_dialog);
    lv_obj_set_pos(keyboard, 20, -100);  // 
    lv_obj_set_size(keyboard, 460, 270);  // Adjusted height to fit buttons below
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_UPPER);
    lv_keyboard_set_textarea(keyboard, rename_textarea);
    
    // OK Button (below keyboard with spacing)
    lv_obj_t *ok_btn = lv_button_create(rename_dialog);
    lv_obj_set_pos(ok_btn, 270, 428);  // 16px gap below keyboard (142 + 270 + 16)
    lv_obj_set_size(ok_btn, 200, 60);
    lv_obj_add_event_cb(ok_btn, rename_dialog_ok_event, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x00AA00), 0);
    
    lv_obj_t *ok_label = lv_label_create(ok_btn);
    lv_label_set_text(ok_label, "OK");
    lv_obj_center(ok_label);
    lv_obj_set_style_text_font(ok_label, &lv_font_montserrat_24, 0);
    
    // Cancel Button (below keyboard with spacing)
    lv_obj_t *cancel_btn = lv_button_create(rename_dialog);
    lv_obj_set_pos(cancel_btn, 30, 428);  // 16px gap below keyboard (142 + 270 + 16)
    lv_obj_set_size(cancel_btn, 200, 60);
    lv_obj_add_event_cb(cancel_btn, rename_dialog_cancel_event, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xAA0000), 0);
    
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
    lv_obj_set_style_text_font(cancel_label, &lv_font_montserrat_24, 0);
}

static void rename_dialog_ok_event(lv_event_t *e) {
    const char *new_name = lv_textarea_get_text(rename_textarea);
    
    if (new_name && strlen(new_name) > 0) {
        // Save new name
        axis_mapping_set_axis_name(current_editing_axis, new_name);
        
        // Update DRO label
        lv_obj_t *axis_labels[] = {
            objects.axis1_label,
            objects.axis2_label,
            objects.axis3_label,
            objects.axis4_label,
            objects.axis5_label
        };
        
        if (axis_labels[current_editing_axis]) {
            lv_label_set_text(axis_labels[current_editing_axis], new_name);
        }
        
        // Refresh E-Screw dropdowns
        dro_refresh_escrew_dropdowns();
    }
    
    // Close dialog
    if (rename_dialog) {
        lv_obj_delete(rename_dialog);
        rename_dialog = NULL;
        rename_textarea = NULL;
    }
}

static void rename_dialog_cancel_event(lv_event_t *e) {
    // Close dialog without saving
    if (rename_dialog) {
        lv_obj_delete(rename_dialog);
        rename_dialog = NULL;
        rename_textarea = NULL;
    }
}

void dro_refresh_escrew_dropdowns(void) {
    // This function will be called from e_screw_screen.c
    // to refresh the dropdowns when axis names change
    refresh_escrew_axis_dropdowns();
}
