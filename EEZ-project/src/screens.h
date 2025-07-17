#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *setings_page;
    lv_obj_t *obj0;
    lv_obj_t *dro_tab;
    lv_obj_t *set_zero_axis5_button;
    lv_obj_t *set_zero_axis4_button;
    lv_obj_t *set_zero_axis_3_button;
    lv_obj_t *set_zero_axis1_button;
    lv_obj_t *set_value_axis5_button;
    lv_obj_t *set_zero_axis2_button;
    lv_obj_t *set_value_axis4_button;
    lv_obj_t *set_value_axis3_button;
    lv_obj_t *set_value_axis2_button;
    lv_obj_t *set_value_axis1_button;
    lv_obj_t *mm_x_axis5_label;
    lv_obj_t *mm_x_axis4_label;
    lv_obj_t *mm_x_axis3_label;
    lv_obj_t *mm_x_axis1_label;
    lv_obj_t *settings_button;
    lv_obj_t *settings;
    lv_obj_t *obj1;
    lv_obj_t *active_tool_number;
    lv_obj_t *half_button;
    lv_obj_t *set_space_number_button;
    lv_obj_t *set_tool_number_button;
    lv_obj_t *toggle_inch_mm_button;
    lv_obj_t *toggle_abs_incr_button;
    lv_obj_t *set_zero_global_button;
    lv_obj_t *mm_x_axis2_label;
    lv_obj_t *axis4_textarea;
    lv_obj_t *info_text_area;
    lv_obj_t *axis1_textarea;
    lv_obj_t *axis3_textarea;
    lv_obj_t *axis5_textarea;
    lv_obj_t *axis2_textarea;
    lv_obj_t *axis5_label;
    lv_obj_t *axis4_label;
    lv_obj_t *axis3_label;
    lv_obj_t *axis2_label;
    lv_obj_t *axis1_label;
    lv_obj_t *numpad;
    lv_obj_t *setting_page;
    lv_obj_t *axis1_tab;
    lv_obj_t *axis2;
    lv_obj_t *axis3_tab;
    lv_obj_t *axis4_tab;
    lv_obj_t *axis5_tab;
    lv_obj_t *general_settings_tab;
    lv_obj_t *settings_mm_switch;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_SETINGS_PAGE = 2,
};

void create_screen_main();
void tick_screen_main();

void create_screen_setings_page();
void tick_screen_setings_page();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/