#pragma once

#include "lvgl.h"

/**
 * @brief Create E-Screw settings screen content for a tab
 * 
 * @param parent_obj Parent tab object to add content to
 */
void create_e_screw_screen(lv_obj_t *parent_obj);

/**
 * @brief Update E-Screw screen with current FPGA state
 */
void tick_e_screw_screen(void);

// Refresh axis dropdowns when names change
void refresh_escrew_axis_dropdowns(void);
