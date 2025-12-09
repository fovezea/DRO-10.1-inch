#pragma once

#include <stdint.h>

/**
 * @brief Calculate gear ratios for cone/taper turning
 * 
 * For a given taper angle, calculates the feed rate ra tio between:
 * - Z-axis (longitudinal feed)
 * - X-axis (cross-slide feed)
 * 
 * Relationship: tan(angle) = X_feed / Z_feed
 * 
 * @param angle_deg Taper angle in degrees (half-angle of the cone)
 * @param z_numerator Output: Z-axis numerator
 * @param z_denominator Output: Z-axis denominator
 * @param x_numerator Output: X-axis numerator
 * @param x_denominator Output: X-axis denominator
 */
void cone_calc_ratio(float angle_deg, 
                     uint32_t *z_numerator, uint32_t *z_denominator,
                     uint32_t *x_numerator, uint32_t *x_denominator);
