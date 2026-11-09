/**
 * @file axis_reader.h
 * @brief Five-axis quadrature decoder built on the ESP32 PCNT peripheral.
 *
 * Responsibilities (and deliberately nothing else):
 *   - decode A/B quadrature from the five scale ports at 4x resolution
 *   - extend the 16-bit PCNT counter to 64 bits with overflow watch points
 *   - capture scale index (Z) pulses
 *   - apply zero / preset / invert, which the frontend asks for
 *
 * The backend is a "dumb pipe": it reports raw counts.  Coordinate systems,
 * tool offsets and unit conversion belong to the frontend.
 */

#ifndef AXIS_READER_H
#define AXIS_READER_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define AXIS_READER_COUNT 5

/**
 * @brief Called from the index task when a Z pulse is latched.
 *
 * Runs in task context (not an ISR), so it may take locks and log.
 *
 * @param axis  0..4
 * @param pos   reported position at the moment of the pulse
 * @param t_ms  uptime in milliseconds
 */
typedef void (*axis_index_cb_t)(int axis, int32_t pos, uint32_t t_ms);

typedef struct {
    int64_t  raw;             /* absolute raw count                     */
    int32_t  pos;             /* reported count (zero/preset/invert)    */
    uint32_t counts_per_s;    /* activity in the last second            */
    uint32_t overflow_count;  /* PCNT wrap events seen                  */
    uint32_t index_count;     /* Z pulses seen                          */
} axis_reader_diag_t;

/** Create the PCNT units, index GPIO interrupts and the sampler tasks. */
esp_err_t axis_reader_init(void);

/** Register the (single) index-event callback. */
void axis_reader_set_index_callback(axis_index_cb_t cb);

/**
 * @brief Copy the current reported positions.
 * @param pos          receives AXIS_READER_COUNT counts
 * @param index_flags  receives bit i = index pulse since the last call
 * @param sample_seq   receives a monotonically increasing sample counter
 */
void axis_reader_snapshot(int32_t pos[AXIS_READER_COUNT],
                          uint8_t *index_flags,
                          uint32_t *sample_seq);

/** Consume and clear the "index pulse seen" bit mask. */
uint8_t axis_reader_take_index_flags(void);

/** Copy the enabled mask (bit i). */
uint8_t axis_reader_enabled_mask(void);

/** Set the current position of one axis, or of every axis in the mask, to 0. */
void axis_reader_zero(uint8_t axis_mask);

/** Set the current reported position of one axis. */
esp_err_t axis_reader_preset(int axis, int32_t value);

/** Flip the counting direction while keeping the reported position stable. */
esp_err_t axis_reader_set_invert(int axis, bool invert);

/** Mark an axis as present/absent (only affects the enabled mask). */
esp_err_t axis_reader_set_enabled(int axis, bool enabled);

/** Enable/disable Z-pulse capture for one axis. */
esp_err_t axis_reader_set_index_enable(int axis, bool enabled);

/**
 * @brief Add a synthetic ramp to every enabled axis.
 *
 * This is the bring-up aid: with `sim on` the link, the frontend and the
 * scaling can all be tested before a single scale is plugged in.
 */
void axis_reader_set_simulation(bool on, int32_t counts_per_s);
bool axis_reader_simulation(void);

void axis_reader_get_diag(int axis, axis_reader_diag_t *out);

/** Read the absolute raw count of one axis (no zero/preset applied). */
int64_t axis_reader_raw(int axis);

/** FreeRTOS task bodies (started by axis_reader_init). */
void axis_reader_sample_task(void *arg);
void axis_reader_index_task(void *arg);

#endif /* AXIS_READER_H */
