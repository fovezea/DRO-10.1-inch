#pragma once

#include "lvgl.h"

/**
 * @brief Initialize DRO axis helpers
 * 
 * Adds click-to-rename functionality to axis labels.
 * Call this after the DRO screen is created.
 */
void dro_axis_helper_init(void);

/**
 * @brief Refresh E-Screw dropdowns with updated axis names
 * 
 * Call this when axis names are changed to update the E-Screw screen dropdowns
 */
void dro_refresh_escrew_dropdowns(void);
