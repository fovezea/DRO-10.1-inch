#include "calculator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

static lv_obj_t *calc_modal = NULL;
static lv_obj_t *calc_ta = NULL;
static lv_obj_t *target_ta = NULL;

static double operand1 = 0;
static char current_op = 0;
static bool last_was_op = false;
static bool has_result = false;

static void close_calculator(void) {
    if (calc_modal) {
        lv_obj_del(calc_modal);
        calc_modal = NULL;
        calc_ta = NULL;
        target_ta = NULL;
        operand1 = 0;
        current_op = 0;
        last_was_op = false;
        has_result = false;
    }
}

static void calc_event_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    uint32_t id = lv_buttonmatrix_get_selected_button(obj);
    const char * txt = lv_buttonmatrix_get_button_text(obj, id);

    if (strcmp(txt, "C") == 0) {
        lv_textarea_set_text(calc_ta, "0");
        operand1 = 0;
        current_op = 0;
        last_was_op = false;
        has_result = false;
    } else if (strcmp(txt, "Apply") == 0) {
        if (target_ta) {
            lv_textarea_set_text(target_ta, lv_textarea_get_text(calc_ta));
            // Trigger value changed event on target
            lv_obj_send_event(target_ta, LV_EVENT_VALUE_CHANGED, NULL);
        }
        close_calculator();
    } else if (strcmp(txt, "Cancel") == 0) {
        close_calculator();
    } else if ((txt[0] >= '0' && txt[0] <= '9') || txt[0] == '.') {
        if (last_was_op || has_result) {
            lv_textarea_set_text(calc_ta, "");
            last_was_op = false;
            has_result = false;
        }
        
        const char * cur = lv_textarea_get_text(calc_ta);
        if (strcmp(cur, "0") == 0 && txt[0] != '.') {
            lv_textarea_set_text(calc_ta, txt);
        } else {
            lv_textarea_add_text(calc_ta, txt);
        }
    } else if (strcmp(txt, "=") == 0) {
        if (current_op) {
            double operand2 = atof(lv_textarea_get_text(calc_ta));
            double res = 0;
            switch(current_op) {
                case '+': res = operand1 + operand2; break;
                case '-': res = operand1 - operand2; break;
                case '*': res = operand1 * operand2; break;
                case '/': if (operand2 != 0) res = operand1 / operand2; break;
            }
            char buf[32];
            snprintf(buf, sizeof(buf), "%.4f", res);
            // Remove trailing zeros
            for (int i = strlen(buf)-1; i > 0 && (buf[i] == '0' || buf[i] == '.'); i--) {
                char c = buf[i];
                buf[i] = '\0';
                if (c == '.') break;
            }
            lv_textarea_set_text(calc_ta, buf);
            current_op = 0;
            has_result = true;
        }
    } else {
        // Operators +, -, *, /
        operand1 = atof(lv_textarea_get_text(calc_ta));
        current_op = txt[0];
        last_was_op = true;
    }
}

void calculator_open(lv_obj_t *target) {
    if (calc_modal) return;
    target_ta = target;

    // Gray out background
    calc_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(calc_modal, 1280, 800);
    lv_obj_set_style_bg_color(calc_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(calc_modal, LV_OPA_50, 0);
    lv_obj_set_style_border_width(calc_modal, 0, 0);
    lv_obj_set_style_radius(calc_modal, 0, 0);

    // Main window
    lv_obj_t * win = lv_obj_create(calc_modal);
    lv_obj_set_size(win, 460, 600);
    lv_obj_center(win);
    lv_obj_set_style_bg_color(win, lv_color_hex(0x282b30), 0);
    lv_obj_set_style_border_color(win, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(win, 2, 0);
    lv_obj_clear_flag(win, LV_OBJ_FLAG_SCROLLABLE);

    // Display
    calc_ta = lv_textarea_create(win);
    lv_obj_set_size(calc_ta, 420, 70);
    lv_obj_align(calc_ta, LV_ALIGN_TOP_MID, 0, 0);
    lv_textarea_set_text(calc_ta, "0");
    lv_obj_set_style_text_font(calc_ta, &lv_font_montserrat_32, 0);
    lv_textarea_set_one_line(calc_ta, true);
    lv_obj_set_style_text_align(calc_ta, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_clear_flag(calc_ta, LV_OBJ_FLAG_CLICKABLE);

    // Buttons
    static const char * btnm_map[] = {
        "7", "8", "9", "/", "\n",
        "4", "5", "6", "*", "\n",
        "1", "2", "3", "-", "\n",
        "C", "0", "=", "+", "\n",
        "Cancel", "Apply", ""
    };

    lv_obj_t * btnm = lv_buttonmatrix_create(win);
    lv_obj_set_size(btnm, 420, 480);
    lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_buttonmatrix_set_map(btnm, btnm_map);
    lv_obj_add_event_cb(btnm, calc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Styling the matrix
    lv_obj_set_style_bg_opa(btnm, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btnm, 0, 0);
    lv_obj_set_style_text_font(btnm, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(btnm, lv_color_hex(0xffffff), 0);
    
    // Make Apply button stand out
    lv_buttonmatrix_set_button_ctrl(btnm, 17, LV_BUTTONMATRIX_CTRL_CHECKED); // Apply
}
