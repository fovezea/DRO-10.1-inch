/**
 * @file axis_reader.c
 * @brief PCNT quadrature decoding for the five scale inputs.
 */

#include "axis_reader.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_pins.h"

static const char *TAG = "axis";

/* The ESP32 PCNT counter is 16 bit.  Keeping the watch points at +/-32000
 * (rather than at the true +/-32768 limits) leaves head room so a burst of
 * pulses cannot skip past a limit between two reads of the counter. */
#define AXIS_PCNT_LIMIT     32000

/* Ignore pulses shorter than this.  The board's 1 k / 100 pF input filter has
 * a ~100 ns time constant, so 250 ns only removes genuine noise.  Keep this
 * well below half an edge period: a 5 um scale at 3 m/s produces 600 kHz, i.e.
 * a 0.83 us half period, and must not be filtered out. */
#define AXIS_GLITCH_NS      250

typedef struct {
    pcnt_unit_handle_t    unit;
    pcnt_channel_handle_t chan_a;
    pcnt_channel_handle_t chan_b;

    int pin_a;
    int pin_b;
    int pin_z;

    /* Written by the PCNT overflow ISR, read by every other context. */
    volatile int64_t accum;

    int64_t origin;        /* raw value that maps to reported 0 */
    int8_t  sign;          /* +1 or -1                          */
    int64_t sim_offset;    /* synthetic counts                  */
    int64_t sim_remainder;

    bool enabled;
    volatile bool index_enabled;

    volatile uint32_t index_count;
    volatile uint32_t overflow_count;

    int64_t  last_raw;
    uint32_t counts_in_window;
} axis_state_t;

static axis_state_t s_axes[AXIS_READER_COUNT];
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static bool s_initialized;
static volatile bool s_sim_on;
static volatile int32_t s_sim_counts_per_s = 1000;
static volatile uint32_t s_sample_seq;

static volatile uint8_t s_index_flags;
static TaskHandle_t s_index_task;
static axis_index_cb_t s_index_cb;

/* ------------------------------------------------------------------------- */
/* Low-level helpers                                                         */
/* ------------------------------------------------------------------------- */

/** Read the absolute raw count.  `s_mux` must be held. */
static int64_t raw_locked(int axis)
{
    int hw = 0;
    pcnt_unit_get_count(s_axes[axis].unit, &hw);
    return s_axes[axis].accum + hw;
}

static int64_t raw_of(int axis)
{
    portENTER_CRITICAL(&s_mux);
    int64_t raw = raw_locked(axis);
    portEXIT_CRITICAL(&s_mux);
    return raw;
}

/** Convert a raw count into the reported count.  `s_mux` must be held. */
static int32_t reported_locked(int axis, int64_t raw)
{
    const axis_state_t *ax = &s_axes[axis];
    int64_t base = raw + ax->sim_offset;
    return (int32_t)((base - ax->origin) * ax->sign);
}

/* ------------------------------------------------------------------------- */
/* PCNT overflow callback                                                    */
/* ------------------------------------------------------------------------- */
static bool IRAM_ATTR pcnt_overflow_cb(pcnt_unit_handle_t unit,
                                       const pcnt_watch_event_data_t *edata,
                                       void *user_ctx)
{
    (void)unit;
    axis_state_t *ax = (axis_state_t *)user_ctx;

    portENTER_CRITICAL_ISR(&s_mux);
    if (edata->watch_point_value > 0) {
        ax->accum += edata->watch_point_value;
    } else {
        ax->accum += edata->watch_point_value;   /* negative */
    }
    ax->overflow_count++;
    portEXIT_CRITICAL_ISR(&s_mux);

    return false;
}

/* ------------------------------------------------------------------------- */
/* Index (Z) interrupt                                                       */
/* ------------------------------------------------------------------------- */
static void IRAM_ATTR index_isr(void *arg)
{
    int axis = (int)(intptr_t)arg;

    if (!s_axes[axis].index_enabled) {
        return;
    }

    s_axes[axis].index_count++;

    BaseType_t woken = pdFALSE;
    if (s_index_task != NULL) {
        xTaskNotifyFromISR(s_index_task, (uint32_t)(1u << axis), eSetBits, &woken);
    }
    if (woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/* ------------------------------------------------------------------------- */
/* Initialisation                                                            */
/* ------------------------------------------------------------------------- */
static esp_err_t init_one_axis(int axis)
{
    const board_axis_pins_t *pins = &BOARD_AXIS_PINS[axis];
    axis_state_t *ax = &s_axes[axis];

    memset(ax, 0, sizeof(*ax));
    ax->pin_a = pins->pin_a;
    ax->pin_b = pins->pin_b;
    ax->pin_z = pins->pin_z;
    ax->sign = 1;
    ax->enabled = true;
    ax->index_enabled = false;

    pcnt_unit_config_t unit_cfg = {
        .clk_src = PCNT_CLK_SRC_DEFAULT,
        .high_limit = AXIS_PCNT_LIMIT,
        .low_limit = -AXIS_PCNT_LIMIT,
        .intr_priority = 0,
        .flags.accum_count = 0,
    };
    esp_err_t err = pcnt_new_unit(&unit_cfg, &ax->unit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[%s] pcnt_new_unit failed: %s", pins->name, esp_err_to_name(err));
        return err;
    }

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(ax->unit, AXIS_PCNT_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(ax->unit, -AXIS_PCNT_LIMIT));

    pcnt_event_callbacks_t cbs = { .on_reach = pcnt_overflow_cb };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(ax->unit, &cbs, ax));

    pcnt_chan_config_t chan_a_cfg = {
        .edge_gpio_num = pins->pin_a,
        .level_gpio_num = pins->pin_b,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(ax->unit, &chan_a_cfg, &ax->chan_a));

    pcnt_chan_config_t chan_b_cfg = {
        .edge_gpio_num = pins->pin_b,
        .level_gpio_num = pins->pin_a,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(ax->unit, &chan_b_cfg, &ax->chan_b));

    /* Standard 4x quadrature decoding: both edges of both channels count, and
     * the level of the other channel selects up or down. */
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        ax->chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        ax->chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        ax->chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        ax->chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    pcnt_glitch_filter_config_t glitch = { .max_glitch_ns = AXIS_GLITCH_NS };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(ax->unit, &glitch));

    ESP_ERROR_CHECK(pcnt_unit_enable(ax->unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(ax->unit));
    ESP_ERROR_CHECK(pcnt_unit_start(ax->unit));

    /* Index input: rising edge, external 10 k pull-up already fitted. */
    gpio_config_t z_cfg = {
        .pin_bit_mask = 1ULL << pins->pin_z,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&z_cfg));
    ESP_ERROR_CHECK(gpio_isr_handler_add(pins->pin_z, index_isr, (void *)(intptr_t)axis));

    ESP_LOGI(TAG, "[%s] A=IO%d B=IO%d Z=IO%d", pins->name, pins->pin_a, pins->pin_b, pins->pin_z);
    return ESP_OK;
}

esp_err_t axis_reader_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    /* No ESP_INTR_FLAG_IRAM here on purpose: the handler wakes a task with
     * xTaskNotifyFromISR(), which lives in flash.  During a flash write the
     * flash cache is off and interrupts are masked, so the pulse stays pending
     * and is serviced a few microseconds later instead of crashing. */
    esp_err_t isr_err = gpio_install_isr_service(0);
    if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service: %s", esp_err_to_name(isr_err));
        return isr_err;
    }

    for (int i = 0; i < AXIS_READER_COUNT; i++) {
        esp_err_t err = init_one_axis(i);
        if (err != ESP_OK) {
            return err;
        }
    }

    if (xTaskCreatePinnedToCore(axis_reader_index_task, "axis_index", 3072, NULL, 12,
                                &s_index_task, 1) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreatePinnedToCore(axis_reader_sample_task, "axis_sample", 3584, NULL, 10,
                                NULL, 1) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "5 axes running at 1 kHz, 4x quadrature, index capture armed");
    return ESP_OK;
}

void axis_reader_set_index_callback(axis_index_cb_t cb)
{
    s_index_cb = cb;
}

/* ------------------------------------------------------------------------- */
/* Tasks                                                                     */
/* ------------------------------------------------------------------------- */
void axis_reader_index_task(void *arg)
{
    (void)arg;

    for (;;) {
        uint32_t bits = 0;
        if (xTaskNotifyWait(0, UINT32_MAX, &bits, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        uint32_t t_ms = (uint32_t)(esp_timer_get_time() / 1000);

        for (int a = 0; a < AXIS_READER_COUNT; a++) {
            if ((bits & (1u << a)) == 0) {
                continue;
            }

            portENTER_CRITICAL(&s_mux);
            int64_t raw = raw_locked(a);
            int32_t pos = reported_locked(a, raw);
            s_index_flags |= (uint8_t)(1u << a);
            portEXIT_CRITICAL(&s_mux);

            if (s_index_cb != NULL) {
                s_index_cb(a, pos, t_ms);
            }
        }
    }
}

void axis_reader_sample_task(void *arg)
{
    (void)arg;

    TickType_t last_wake = xTaskGetTickCount();
    int tick_in_second = 0;

    for (;;) {
        for (int a = 0; a < AXIS_READER_COUNT; a++) {
            axis_state_t *ax = &s_axes[a];

            portENTER_CRITICAL(&s_mux);
            int64_t raw = raw_locked(a);

            /* Synthetic movement for bench testing without scales. */
            if (s_sim_on && ax->enabled) {
                ax->sim_remainder += s_sim_counts_per_s;
                int32_t step = (int32_t)(ax->sim_remainder / 1000);
                ax->sim_remainder -= (int64_t)step * 1000;
                ax->sim_offset += step;   /* applied in reported_locked() */
            }

            int64_t delta = raw - ax->last_raw;
            if (delta < 0) {
                delta = -delta;
            }
            if (delta > (int64_t)UINT32_MAX - ax->counts_in_window) {
                ax->counts_in_window = UINT32_MAX;
            } else {
                ax->counts_in_window += (uint32_t)delta;
            }
            ax->last_raw = raw;
            portEXIT_CRITICAL(&s_mux);
        }

        portENTER_CRITICAL(&s_mux);
        s_sample_seq++;
        portEXIT_CRITICAL(&s_mux);

        if (++tick_in_second >= 1000) {
            tick_in_second = 0;
            portENTER_CRITICAL(&s_mux);
            for (int a = 0; a < AXIS_READER_COUNT; a++) {
                s_axes[a].counts_in_window = 0;
            }
            portEXIT_CRITICAL(&s_mux);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1));
    }
}

/* ------------------------------------------------------------------------- */
/* Public accessors                                                          */
/* ------------------------------------------------------------------------- */
void axis_reader_snapshot(int32_t pos[AXIS_READER_COUNT],
                          uint8_t *index_flags,
                          uint32_t *sample_seq)
{
    portENTER_CRITICAL(&s_mux);
    for (int a = 0; a < AXIS_READER_COUNT; a++) {
        if (pos != NULL) {
            pos[a] = reported_locked(a, raw_locked(a));
        }
    }
    if (index_flags != NULL) {
        *index_flags = s_index_flags;
        s_index_flags = 0;
    }
    if (sample_seq != NULL) {
        *sample_seq = s_sample_seq;
    }
    portEXIT_CRITICAL(&s_mux);
}

uint8_t axis_reader_take_index_flags(void)
{
    portENTER_CRITICAL(&s_mux);
    uint8_t flags = s_index_flags;
    s_index_flags = 0;
    portEXIT_CRITICAL(&s_mux);
    return flags;
}

uint8_t axis_reader_enabled_mask(void)
{
    uint8_t mask = 0;
    portENTER_CRITICAL(&s_mux);
    for (int a = 0; a < AXIS_READER_COUNT; a++) {
        if (s_axes[a].enabled) {
            mask |= (uint8_t)(1u << a);
        }
    }
    portEXIT_CRITICAL(&s_mux);
    return mask;
}

void axis_reader_zero(uint8_t axis_mask)
{
    portENTER_CRITICAL(&s_mux);
    for (int a = 0; a < AXIS_READER_COUNT; a++) {
        if (axis_mask & (1u << a)) {
            s_axes[a].origin = raw_locked(a) + s_axes[a].sim_offset;
        }
    }
    portEXIT_CRITICAL(&s_mux);
}

esp_err_t axis_reader_preset(int axis, int32_t value)
{
    if (axis < 0 || axis >= AXIS_READER_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_mux);
    int64_t base = raw_locked(axis) + s_axes[axis].sim_offset;
    /* reported = (base - origin) * sign  =>  origin = base - value * sign */
    s_axes[axis].origin = base - (int64_t)value * s_axes[axis].sign;
    portEXIT_CRITICAL(&s_mux);
    return ESP_OK;
}

esp_err_t axis_reader_set_invert(int axis, bool invert)
{
    if (axis < 0 || axis >= AXIS_READER_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_mux);
    int8_t new_sign = invert ? -1 : 1;
    if (new_sign != s_axes[axis].sign) {
        int64_t raw = raw_locked(axis);
        int64_t base = raw + s_axes[axis].sim_offset;
        int32_t current = reported_locked(axis, raw);
        s_axes[axis].sign = new_sign;
        s_axes[axis].origin = base - (int64_t)current * new_sign;
    }
    portEXIT_CRITICAL(&s_mux);
    return ESP_OK;
}

esp_err_t axis_reader_set_enabled(int axis, bool enabled)
{
    if (axis < 0 || axis >= AXIS_READER_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    portENTER_CRITICAL(&s_mux);
    s_axes[axis].enabled = enabled;
    portEXIT_CRITICAL(&s_mux);
    return ESP_OK;
}

esp_err_t axis_reader_set_index_enable(int axis, bool enabled)
{
    if (axis < 0 || axis >= AXIS_READER_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    s_axes[axis].index_enabled = enabled;
    return ESP_OK;
}

void axis_reader_set_simulation(bool on, int32_t counts_per_s)
{
    if (counts_per_s < 0) {
        counts_per_s = -counts_per_s;
    }
    if (counts_per_s == 0) {
        counts_per_s = 1000;
    }

    portENTER_CRITICAL(&s_mux);
    if (!on && s_sim_on) {
        /* Remove the synthetic travel from the origin so the displayed
         * position does not jump, then start following the real scale again
         * from wherever it is now. */
        for (int a = 0; a < AXIS_READER_COUNT; a++) {
            s_axes[a].origin -= s_axes[a].sim_offset;
            s_axes[a].sim_offset = 0;
            s_axes[a].sim_remainder = 0;
        }
    }
    s_sim_counts_per_s = counts_per_s;
    s_sim_on = on;
    portEXIT_CRITICAL(&s_mux);

    ESP_LOGW(TAG, "simulation %s (%" PRId32 " counts/s)", on ? "ON" : "OFF", counts_per_s);
}

bool axis_reader_simulation(void)
{
    return s_sim_on;
}

void axis_reader_get_diag(int axis, axis_reader_diag_t *out)
{
    if (out == NULL || axis < 0 || axis >= AXIS_READER_COUNT) {
        return;
    }

    portENTER_CRITICAL(&s_mux);
    out->raw = raw_locked(axis);
    out->pos = reported_locked(axis, out->raw);
    out->counts_per_s = s_axes[axis].counts_in_window;
    out->overflow_count = s_axes[axis].overflow_count;
    out->index_count = s_axes[axis].index_count;
    portEXIT_CRITICAL(&s_mux);
}

int64_t axis_reader_raw(int axis)
{
    if (axis < 0 || axis >= AXIS_READER_COUNT) {
        return 0;
    }
    return raw_of(axis);
}
