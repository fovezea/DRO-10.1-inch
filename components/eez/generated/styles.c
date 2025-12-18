#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: label_axis
//

void init_style_label_axis_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(0xf9f600));
    lv_style_set_text_font(style, &ui_font_jet_brains_mono_bold_64);
    lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(style, LV_ALIGN_DEFAULT);
    lv_style_set_min_width(style, 50);
    lv_style_set_min_height(style, 50);
    lv_style_set_max_height(style, 50);
    lv_style_set_layout(style, LV_LAYOUT_GRID);
    {
        static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
        lv_style_set_grid_row_dsc_array(style, dsc);
    }
    {
        static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
        lv_style_set_grid_column_dsc_array(style, dsc);
    }
};

lv_style_t *get_style_label_axis_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_label_axis_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_label_axis_MAIN_DISABLED(lv_style_t *style) {
    lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_font(style, &lv_font_montserrat_42);
    lv_style_set_text_color(style, lv_color_hex(0xf9f600));
};

lv_style_t *get_style_label_axis_MAIN_DISABLED() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_label_axis_MAIN_DISABLED(style);
    }
    return style;
};

void add_style_label_axis(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_label_axis_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_label_axis_MAIN_DISABLED(), LV_PART_MAIN | LV_STATE_DISABLED);
};

void remove_style_label_axis(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_label_axis_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_label_axis_MAIN_DISABLED(), LV_PART_MAIN | LV_STATE_DISABLED);
};

//
// Style: label_mm
//

void init_style_label_mm_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_min_width(style, 50);
    lv_style_set_max_width(style, 50);
    lv_style_set_min_height(style, 50);
    lv_style_set_max_height(style, 50);
    lv_style_set_text_color(style, lv_color_hex(0xf9f600));
    lv_style_set_align(style, LV_ALIGN_DEFAULT);
    lv_style_set_text_font(style, &lv_font_montserrat_22);
    lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);
};

lv_style_t *get_style_label_mm_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_label_mm_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_label_mm(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_label_mm_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_label_mm(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_label_mm_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: button_tyle1
//

void init_style_button_tyle1_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_min_width(style, 100);
    lv_style_set_max_width(style, 100);
    lv_style_set_min_height(style, 60);
    lv_style_set_max_height(style, 60);
    lv_style_set_radius(style, 6);
    lv_style_set_border_color(style, lv_color_hex(0xffffffff));
    lv_style_set_border_width(style, 2);
    lv_style_set_bg_color(style, lv_color_hex(0xff282b30));
    lv_style_set_clip_corner(style, true);
};

lv_style_t *get_style_button_tyle1_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_button_tyle1_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_button_tyle1_MAIN_CHECKED_PRESSED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xfff44336));
};

lv_style_t *get_style_button_tyle1_MAIN_CHECKED_PRESSED() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_button_tyle1_MAIN_CHECKED_PRESSED(style);
    }
    return style;
};

void add_style_button_tyle1(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_button_tyle1_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_button_tyle1_MAIN_CHECKED_PRESSED(), LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
};

void remove_style_button_tyle1(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_button_tyle1_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_button_tyle1_MAIN_CHECKED_PRESSED(), LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
};

//
// Style: button_label
//

void init_style_button_label_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(0xfffafafa));
    lv_style_set_text_font(style, &lv_font_montserrat_16);
};

lv_style_t *get_style_button_label_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_button_label_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_button_label(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_button_label_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_button_label(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_button_label_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: Tab_style
//

void init_style_tab_style_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_min_height(style, 80);
    lv_style_set_max_height(style, 1280);
    lv_style_set_text_font(style, &lv_font_montserrat_14);
    lv_style_set_text_color(style, lv_color_hex(0xfffafafa));
    lv_style_set_bg_color(style, lv_color_hex(0xff282b30));
};

lv_style_t *get_style_tab_style_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_tab_style_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_tab_style(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_tab_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_tab_style(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_tab_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: Text_area_axis_value
//

void init_style_text_area_axis_value_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_min_height(style, 80);
    lv_style_set_max_height(style, 80);
    lv_style_set_min_width(style, 335);
    lv_style_set_text_color(style, lv_color_hex(0xf9f600));
    lv_style_set_text_font(style, &ui_font_jet_brains_mono_bold_64);
    lv_style_set_text_align(style, LV_TEXT_ALIGN_RIGHT);
    lv_style_set_pad_right(style, 10);
    lv_style_set_bg_opa(style, LV_OPA_TRANSP);
    lv_style_set_border_width(style, 0);
};

lv_style_t *get_style_text_area_axis_value_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_text_area_axis_value_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_text_area_axis_value_SELECTED_FOCUSED(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(0xfff90000));
};

lv_style_t *get_style_text_area_axis_value_SELECTED_FOCUSED() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_text_area_axis_value_SELECTED_FOCUSED(style);
    }
    return style;
};

void add_style_text_area_axis_value(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_text_area_axis_value_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_text_area_axis_value_MAIN_DEFAULT(), LV_PART_TEXTAREA_PLACEHOLDER | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_text_area_axis_value_SELECTED_FOCUSED(), LV_PART_SELECTED | LV_STATE_FOCUSED);
};

void remove_style_text_area_axis_value(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_text_area_axis_value_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_text_area_axis_value_SELECTED_FOCUSED(), LV_PART_SELECTED | LV_STATE_FOCUSED);
};

//
// Style: active_tool_and_space
//

void init_style_active_tool_and_space_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_min_height(style, 60);
    lv_style_set_max_height(style, 60);
    lv_style_set_text_color(style, lv_color_hex(0xf9f600));
    lv_style_set_text_font(style, &lv_font_montserrat_40);
    lv_style_set_min_width(style, 100);
};

lv_style_t *get_style_active_tool_and_space_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_active_tool_and_space_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_active_tool_and_space(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_active_tool_and_space_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_active_tool_and_space_MAIN_DEFAULT(), LV_PART_TEXTAREA_PLACEHOLDER | LV_STATE_DEFAULT);
};

void remove_style_active_tool_and_space(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_active_tool_and_space_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: setting page label
//

void init_style_setting_page_label_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(0xf9f600));
    lv_style_set_text_font(style, &lv_font_montserrat_32);
};

lv_style_t *get_style_setting_page_label_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_setting_page_label_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_setting_page_label(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_setting_page_label_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_setting_page_label(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_setting_page_label_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_label_axis,
        add_style_label_mm,
        add_style_button_tyle1,
        add_style_button_label,
        add_style_tab_style,
        add_style_text_area_axis_value,
        add_style_active_tool_and_space,
        add_style_setting_page_label,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_label_axis,
        remove_style_label_mm,
        remove_style_button_tyle1,
        remove_style_button_label,
        remove_style_tab_style,
        remove_style_text_area_axis_value,
        remove_style_active_tool_and_space,
        remove_style_setting_page_label,
    };
    remove_style_funcs[styleIndex](obj);
}

