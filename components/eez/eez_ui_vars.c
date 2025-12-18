#include "generated/vars.h"
#include "dro_core.h"
#include <string.h>

// Global variable storage
static float virtual_axis_1 = 0.0f;
static float virtual_axis_2 = 0.0f;
static float virtual_axis_3 = 0.0f;
static float virtual_axis_4 = 0.0f;
static float virtual_axis_5 = 0.0f;
static int32_t active_tool_number = 0;
static int32_t active_space_number = 0;

// Getter functions for float variables
float get_var_virtual_axis_1() {
    return virtual_axis_1;
}

float get_var_virtual_axis_2() {
    return virtual_axis_2;
}

float get_var_virtual_axis_3() {
    return virtual_axis_3;
}

float get_var_virtual_axis_4() {
    return virtual_axis_4;
}

float get_var_virtual_axis_5() {
    return virtual_axis_5;
}

// Setter functions for float variables
void set_var_virtual_axis_1(float value) {
    virtual_axis_1 = value;
}

void set_var_virtual_axis_2(float value) {
    virtual_axis_2 = value;
}

void set_var_virtual_axis_3(float value) {
    virtual_axis_3 = value;
}

void set_var_virtual_axis_4(float value) {
    virtual_axis_4 = value;
}

void set_var_virtual_axis_5(float value) {
    virtual_axis_5 = value;
}

// Getter functions for integer variables
int32_t get_var_active_tool_number() {
    return active_tool_number;
}

int32_t get_var_active_space_number() {
    return active_space_number;
}

// Setter functions for integer variables
void set_var_active_tool_number(int32_t value) {
    active_tool_number = value;
}

void set_var_active_space_number(int32_t value) {
    active_space_number = value;
}

int get_var_axis_precision() {
    return dro_get_precision();
}
