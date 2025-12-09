#pragma once

#include <stdint.h>

/**
 * @brief Calculate gear ratio for screw cutting
 * 
 * Formula: Ratio (N/D) = (Target_Pitch * Steps_Per_Rev) / (Leadscrew_Pitch * Encoder_Counts_Per_Rev)
 * The ratio is automatically reduced to lowest terms using GCD.
 * 
 * @param target_pitch_mm Target screw pitch in mm (e.g., 0.5 for M3)
 * @param numerator Output: Simplified numerator
 * @param denominator Output: Simplified denominator
 */
void screw_calc_ratio(float target_pitch_mm, uint32_t *numerator, uint32_t *denominator);
