#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_VIRTUAL_AXIS_1 = 0,
    FLOW_GLOBAL_VARIABLE_VIRTUAL_AXIS_2 = 1,
    FLOW_GLOBAL_VARIABLE_VIRTUAL_AXIS_3 = 2,
    FLOW_GLOBAL_VARIABLE_VIRTUAL_AXIS_4 = 3,
    FLOW_GLOBAL_VARIABLE_VIRTUAL_AXIS_5 = 4,
    FLOW_GLOBAL_VARIABLE_ACTIVE_TOOL_NUMBER = 5,
    FLOW_GLOBAL_VARIABLE_ACTIVE_SPACE_NUMBER = 6
};

// Native global variables

extern float get_var_virtual_axis_1();
extern void set_var_virtual_axis_1(float value);
extern float get_var_virtual_axis_2();
extern void set_var_virtual_axis_2(float value);
extern float get_var_virtual_axis_3();
extern void set_var_virtual_axis_3(float value);
extern float get_var_virtual_axis_4();
extern void set_var_virtual_axis_4(float value);
extern float get_var_virtual_axis_5();
extern void set_var_virtual_axis_5(float value);
extern int32_t get_var_active_tool_number();
extern void set_var_active_tool_number(int32_t value);
extern int32_t get_var_active_space_number();
extern void set_var_active_space_number(int32_t value);
extern int get_var_axis_precision();
extern const char* get_var_mode_text();
extern const char* get_var_conn_status_text();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/