#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: label_axis
lv_style_t *get_style_label_axis_MAIN_DEFAULT();
void add_style_label_axis(lv_obj_t *obj);
void remove_style_label_axis(lv_obj_t *obj);

// Style: textarea_axis_value
lv_style_t *get_style_textarea_axis_value_MAIN_DEFAULT();
void add_style_textarea_axis_value(lv_obj_t *obj);
void remove_style_textarea_axis_value(lv_obj_t *obj);

// Style: label_mm
lv_style_t *get_style_label_mm_MAIN_DEFAULT();
void add_style_label_mm(lv_obj_t *obj);
void remove_style_label_mm(lv_obj_t *obj);

// Style: button_tyle1
lv_style_t *get_style_button_tyle1_MAIN_DEFAULT();
void add_style_button_tyle1(lv_obj_t *obj);
void remove_style_button_tyle1(lv_obj_t *obj);

// Style: button_label
lv_style_t *get_style_button_label_MAIN_DEFAULT();
void add_style_button_label(lv_obj_t *obj);
void remove_style_button_label(lv_obj_t *obj);



#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/