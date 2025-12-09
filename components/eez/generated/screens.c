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
        lv_obj_t *button = lv_event_get_target(e);
        uint32_t tab_index = (uint32_t)lv_event_get_user_data(e);
        
        // Manually switch to the selected tab
        lv_tabview_set_active(objects.obj0, tab_index, LV_ANIM_ON);
    }
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
            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
                    
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
                            lv_obj_set_pos(obj, 387, 534);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // mm_X_axis4_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.mm_x_axis4_label = obj;
                            lv_obj_set_pos(obj, 387, 418);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // mm_X_axis3_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.mm_x_axis3_label = obj;
                            lv_obj_set_pos(obj, 387, 302);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // mm_X_axis1_label
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.mm_x_axis1_label = obj;
                            lv_obj_set_pos(obj, 387, 70);
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
                            lv_obj_set_pos(obj, 387, 186);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_label_mm(obj);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // Axis4_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis4_textarea = obj;
                            lv_obj_set_pos(obj, 78, 378);
                            lv_obj_set_size(obj, 290, 80);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_placeholder_text(obj, "8888.0000");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis4_textarea, LV_EVENT_ALL, 0);
                            add_style_text_area_axis_value(obj);
                        }
                        {
                            // info_text_area
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.info_text_area = obj;
                            lv_obj_set_pos(obj, 797, -5);
                            lv_obj_set_size(obj, 442, 97);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_one_line(obj, false);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_set_style_min_height(obj, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_max_height(obj, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // Axis1_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis1_textarea = obj;
                            lv_obj_set_pos(obj, 78, 30);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_placeholder_text(obj, "8888.0000");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis1_textarea, LV_EVENT_ALL, 0);
                            lv_obj_add_state(obj, LV_STATE_FOCUSED);
                            add_style_text_area_axis_value(obj);
                        }
                        {
                            // Axis3_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis3_textarea = obj;
                            lv_obj_set_pos(obj, 78, 262);
                            lv_obj_set_size(obj, 290, 80);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_placeholder_text(obj, "8888.0000");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis3_textarea, LV_EVENT_ALL, 0);
                            add_style_text_area_axis_value(obj);
                        }
                        {
                            // Axis5_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis5_textarea = obj;
                            lv_obj_set_pos(obj, 78, 494);
                            lv_obj_set_size(obj, 290, 80);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_placeholder_text(obj, "8888.0000");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis5_textarea, LV_EVENT_ALL, 0);
                            add_style_text_area_axis_value(obj);
                        }
                        {
                            // Axis2_textarea
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.axis2_textarea = obj;
                            lv_obj_set_pos(obj, 78, 146);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_textarea_set_max_length(obj, 9);
                            lv_textarea_set_placeholder_text(obj, "8888.8888");
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, event_handler_cb_main_axis2_textarea, LV_EVENT_ALL, 0);
                            add_style_text_area_axis_value(obj);
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
                    // Tab 2
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Tab 2");
                    add_style_tab_style(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
                    
                    // Get the tab button and add explicit event handler
                    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.obj0);
                    lv_obj_t *tab_button = lv_obj_get_child_by_type(tab_bar, 1, &lv_button_class);
                    if (tab_button) {
                        lv_obj_add_event_cb(tab_button, tab_button_event_handler, LV_EVENT_CLICKED, (void*)1);
                    }
                    
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // Add a label to make the tab visible
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 50, 50);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "Tab 2 Content");
                        }
                    }
                }
                {
                    // Tab 3
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Tab 3");
                    add_style_tab_style(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
                    
                    // Get the tab button and add explicit event handler
                    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.obj0);
                    lv_obj_t *tab_button = lv_obj_get_child_by_type(tab_bar, 2, &lv_button_class);
                    if (tab_button) {
                        lv_obj_add_event_cb(tab_button, tab_button_event_handler, LV_EVENT_CLICKED, (void*)2);
                    }
                    
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // Add a label to make the tab visible
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 50, 50);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "Tab 3 Content");
                        }
                    }
                }
                {
                    // E-Screw Tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "E-Screw");
                    add_style_tab_style(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
                    
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
    
    tick_screen_main();
}

void tick_screen_main() {
    // Update E-Screw screen
    tick_e_screw_screen();
    
    {
        char new_val_str[32];
        snprintf(new_val_str, sizeof(new_val_str), "%ld", get_var_active_space_number());
        const char *cur_val = lv_textarea_get_text(objects.obj1);
        uint32_t max_length = lv_textarea_get_max_length(objects.obj1);
        if (strncmp(new_val_str, cur_val, max_length) != 0) {
            tick_value_change_obj = objects.obj1;
            lv_textarea_set_text(objects.obj1, new_val_str);
            tick_value_change_obj = NULL;
        }
    }
    {
        char new_val_str[32];
        snprintf(new_val_str, sizeof(new_val_str), "%ld", get_var_active_tool_number());
        const char *cur_val = lv_textarea_get_text(objects.active_tool_number);
        uint32_t max_length = lv_textarea_get_max_length(objects.active_tool_number);
        if (strncmp(new_val_str, cur_val, max_length) != 0) {
            tick_value_change_obj = objects.active_tool_number;
            lv_textarea_set_text(objects.active_tool_number, new_val_str);
            tick_value_change_obj = NULL;
        }
    }
    {
        char new_val_str[32];
        snprintf(new_val_str, sizeof(new_val_str), "%.4f", get_var_virtual_axis_4());
        const char *cur_val = lv_textarea_get_text(objects.axis4_textarea);
        uint32_t max_length = lv_textarea_get_max_length(objects.axis4_textarea);
        if (strncmp(new_val_str, cur_val, max_length) != 0) {
            tick_value_change_obj = objects.axis4_textarea;
            lv_textarea_set_text(objects.axis4_textarea, new_val_str);
            tick_value_change_obj = NULL;
        }
    }
    {
        char new_val_str[32];
        snprintf(new_val_str, sizeof(new_val_str), "%.4f", get_var_virtual_axis_1());
        const char *cur_val = lv_textarea_get_text(objects.axis1_textarea);
        uint32_t max_length = lv_textarea_get_max_length(objects.axis1_textarea);
        if (strncmp(new_val_str, cur_val, max_length) != 0) {
            tick_value_change_obj = objects.axis1_textarea;
            lv_textarea_set_text(objects.axis1_textarea, new_val_str);
            tick_value_change_obj = NULL;
        }
    }
    {
        char new_val_str[32];
        snprintf(new_val_str, sizeof(new_val_str), "%.4f", get_var_virtual_axis_3());
        const char *cur_val = lv_textarea_get_text(objects.axis3_textarea);
        uint32_t max_length = lv_textarea_get_max_length(objects.axis3_textarea);
        if (strncmp(new_val_str, cur_val, max_length) != 0) {
            tick_value_change_obj = objects.axis3_textarea;
            lv_textarea_set_text(objects.axis3_textarea, new_val_str);
            tick_value_change_obj = NULL;
        }
    }
    {
        char new_val_str[32];
        snprintf(new_val_str, sizeof(new_val_str), "%.4f", get_var_virtual_axis_5());
        const char *cur_val = lv_textarea_get_text(objects.axis5_textarea);
        uint32_t max_length = lv_textarea_get_max_length(objects.axis5_textarea);
        if (strncmp(new_val_str, cur_val, max_length) != 0) {
            tick_value_change_obj = objects.axis5_textarea;
            lv_textarea_set_text(objects.axis5_textarea, new_val_str);
            tick_value_change_obj = NULL;
        }
    }
    {
        char new_val_str[32];
        snprintf(new_val_str, sizeof(new_val_str), "%.4f", get_var_virtual_axis_2());
        const char *cur_val = lv_textarea_get_text(objects.axis2_textarea);
        uint32_t max_length = lv_textarea_get_max_length(objects.axis2_textarea);
        if (strncmp(new_val_str, cur_val, max_length) != 0) {
            tick_value_change_obj = objects.axis2_textarea;
            lv_textarea_set_text(objects.axis2_textarea, new_val_str);
            tick_value_change_obj = NULL;
        }
    }
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
            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff9f600), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // axis1_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 1");
                    objects.axis1_tab = obj;
                    add_style_tab_style(obj);
                }
                {
                    // axis2
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 2");
                    objects.axis2 = obj;
                }
                {
                    // axis3_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 3");
                    objects.axis3_tab = obj;
                }
                {
                    // axis4_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 4");
                    objects.axis4_tab = obj;
                }
                {
                    // axis5_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Axis 5");
                    objects.axis5_tab = obj;
                }
                {
                    // general_settings_tab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "General");
                    objects.general_settings_tab = obj;
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 43, 96);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_setting_page_label(obj);
            lv_label_set_text(obj, "Liniar axes in mm");
        }
        {
            // settings_mm_switch
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.settings_mm_switch = obj;
            lv_obj_set_pos(obj, 934, 106);
            lv_obj_set_size(obj, 50, 25);
            lv_obj_add_event_cb(obj, action_settings_mm_switch_pressed, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_state(obj, LV_STATE_CHECKED);
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
