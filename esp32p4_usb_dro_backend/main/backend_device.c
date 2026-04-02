#include "sdkconfig.h"
#include "usb_reports.h"

#ifdef ROLE_BACKEND

#include "class/hid/hid_device.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "driver/pulse_cnt.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "bsp.h"
#include <stdint.h>
#include <string.h>

static const char *TAG = "BACKEND_DEV";

// -----------------------------------------------------------------------------
// PCNT HARDWARE STRUCTURES
// -----------------------------------------------------------------------------
typedef struct {
    bool enabled;
    int pin_a;
    int pin_b;
    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t pcnt_chan_a;
    pcnt_channel_handle_t pcnt_chan_b;
    volatile int64_t overflow_accum; // Software 64-bit absolute accumulator
} axis_handle_t;

static axis_handle_t axes[NUM_VIRTUAL_AXES];

static pcnt_unit_handle_t spindle_pcnt_unit = NULL;
static pcnt_channel_handle_t spindle_pcnt_chan_a = NULL;
static pcnt_channel_handle_t spindle_pcnt_chan_b = NULL;
static volatile int64_t spindle_overflow_accum = 0; // Software 64-bit absolute accumulator

// PCNT hardware watchpoint limit (must match unit_config.high_limit / low_limit)
#define PCNT_LIMIT 32000

// =============================================================================
// IRAM-SAFE PCNT OVERFLOW CALLBACK
// Fires from hardware interrupt when any PCNT unit hits +/- PCNT_LIMIT.
// Atomically adjusts the 64-bit software accumulator.
// Enabled by CONFIG_PCNT_ISR_IRAM_SAFE in sdkconfig.
// =============================================================================
static bool IRAM_ATTR pcnt_overflow_cb(pcnt_unit_handle_t unit,
                                        const pcnt_watch_event_data_t *edata,
                                        void *user_ctx) {
    volatile int64_t *accum = (volatile int64_t *)user_ctx;
    if (edata->watch_point_value == PCNT_LIMIT) {
        *accum += PCNT_LIMIT;
    } else {
        *accum -= PCNT_LIMIT;
    }
    return false; // No high-priority task wake-up needed
}

static void init_encoders(void) {
    // Basic setup from BSP for X, Y, Z, W 
    memset(axes, 0, sizeof(axes));
    
    // In our HIL test, these PCNT pins will be physically wired to our Stepper Output pins!
    axes[0].enabled = true; axes[0].pin_a = BSP_PIN_ENC_X_A; axes[0].pin_b = BSP_PIN_ENC_X_B;
    axes[1].enabled = true; axes[1].pin_a = BSP_PIN_ENC_Y_A; axes[1].pin_b = BSP_PIN_ENC_Y_B;
    axes[2].enabled = true; axes[2].pin_a = BSP_PIN_ENC_Z_A; axes[2].pin_b = BSP_PIN_ENC_Z_B;
    axes[3].enabled = false; axes[3].pin_a = BSP_PIN_ENC_W_A; axes[3].pin_b = BSP_PIN_ENC_W_B; // DISABLED: ESP32-P4 only has 4 PCNT units total!
    axes[4].enabled = false; axes[4].pin_a = BSP_PIN_ENC_C_A; axes[4].pin_b = BSP_PIN_ENC_C_B;

    int enabled_count = 0;
    for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
        if (!axes[i].enabled) continue;
        enabled_count++;

        ESP_LOGI(TAG, "Initializing PCNT Axis %d on pins %d, %d", i, axes[i].pin_a, axes[i].pin_b);

        axes[i].overflow_accum = 0;

        pcnt_unit_config_t unit_config = {
            .high_limit = PCNT_LIMIT,
            .low_limit  = -PCNT_LIMIT,
        };
        ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &axes[i].pcnt_unit));

        // Register watchpoints for overflow detection
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(axes[i].pcnt_unit,  PCNT_LIMIT));
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(axes[i].pcnt_unit, -PCNT_LIMIT));

        // Attach IRAM-safe overflow callback
        pcnt_event_callbacks_t axis_cbs = { .on_reach = pcnt_overflow_cb };
        ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(axes[i].pcnt_unit, &axis_cbs,
                                                           (void *)&axes[i].overflow_accum));

        pcnt_chan_config_t chan_a_config = {
            .edge_gpio_num = axes[i].pin_a,
            .level_gpio_num = axes[i].pin_b,
        };
        ESP_ERROR_CHECK(pcnt_new_channel(axes[i].pcnt_unit, &chan_a_config, &axes[i].pcnt_chan_a));

        pcnt_chan_config_t chan_b_config = {
            .edge_gpio_num = axes[i].pin_b,
            .level_gpio_num = axes[i].pin_a,
        };
        ESP_ERROR_CHECK(pcnt_new_channel(axes[i].pcnt_unit, &chan_b_config, &axes[i].pcnt_chan_b));

        // Quadrature Logic: 4X resolution
        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(axes[i].pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(axes[i].pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
        
        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(axes[i].pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(axes[i].pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        ESP_ERROR_CHECK(pcnt_unit_enable(axes[i].pcnt_unit));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(axes[i].pcnt_unit));
        ESP_ERROR_CHECK(pcnt_unit_start(axes[i].pcnt_unit));
    }

    ESP_LOGI(TAG, "Initializing Spindle PCNT on pins %d, %d", BSP_PIN_SPINDLE_A, BSP_PIN_SPINDLE_B);
    
    pcnt_unit_config_t spindle_unit_config = {
        .high_limit =  PCNT_LIMIT,
        .low_limit  = -PCNT_LIMIT,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&spindle_unit_config, &spindle_pcnt_unit));

    // Register watchpoints for Spindle overflow detection
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(spindle_pcnt_unit,  PCNT_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(spindle_pcnt_unit, -PCNT_LIMIT));

    // Attach IRAM-safe overflow callback for Spindle
    pcnt_event_callbacks_t spindle_cbs = { .on_reach = pcnt_overflow_cb };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(spindle_pcnt_unit, &spindle_cbs,
                                                       (void *)&spindle_overflow_accum));

    pcnt_chan_config_t spindle_chan_a_config = {
        .edge_gpio_num = BSP_PIN_SPINDLE_A,
        .level_gpio_num = BSP_PIN_SPINDLE_B,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(spindle_pcnt_unit, &spindle_chan_a_config, &spindle_pcnt_chan_a));

    pcnt_chan_config_t spindle_chan_b_config = {
        .edge_gpio_num = BSP_PIN_SPINDLE_B,
        .level_gpio_num = BSP_PIN_SPINDLE_A,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(spindle_pcnt_unit, &spindle_chan_b_config, &spindle_pcnt_chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(spindle_pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(spindle_pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(spindle_pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(spindle_pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_enable(spindle_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(spindle_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(spindle_pcnt_unit));

    ESP_LOGI(TAG, "Encoders initialized. Enabled hardware scales: %d", enabled_count);
}

// -----------------------------------------------------------------------------
// TIMER ISR & DDA STEPPER ENGINE
// -----------------------------------------------------------------------------
static int step_pins[NUM_VIRTUAL_AXES] = {BSP_PIN_STEP_X, BSP_PIN_STEP_Y, BSP_PIN_STEP_Z, BSP_PIN_STEP_W, BSP_PIN_STEP_C};
static int dir_pins[NUM_VIRTUAL_AXES]  = {BSP_PIN_DIR_X, BSP_PIN_DIR_Y, BSP_PIN_DIR_Z, BSP_PIN_DIR_W, BSP_PIN_DIR_C};

static volatile int32_t phase_accumulator[NUM_VIRTUAL_AXES] = {0};
static volatile float actual_mult[NUM_VIRTUAL_AXES] = {0.0f};

static volatile int64_t last_spindle_abs = 0;  // Tracks last absolute spindle position
static volatile int32_t target_steps_generated[NUM_VIRTUAL_AXES] = {0};
static volatile uint8_t step_pin_state[NUM_VIRTUAL_AXES] = {0};

static hid_output_report_t active_setup = {0}; // Shared across USB and ISR

static bool IRAM_ATTR els_stepper_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
    // 1. Read the true ABSOLUTE Spindle position (hw_count + 64-bit overflow accumulator)
    int hw_spindle = 0;
    pcnt_unit_get_count(spindle_pcnt_unit, &hw_spindle);
    int64_t abs_spindle = spindle_overflow_accum + hw_spindle;

    int64_t delta = abs_spindle - last_spindle_abs;
    last_spindle_abs = abs_spindle;

    for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
        // ALWAYS CLEAR HIGH PINS ON THE NEXT TICK TO CREATE 20us PULSE WIDTH
        if (step_pin_state[i]) {
            gpio_set_level(step_pins[i], 0);
            step_pin_state[i] = 0;
        }

        if (active_setup.track_spindle[i]) {
            if (delta != 0) {
                // Fixed point Q16.16 math logic inside the ISR
                int32_t fixed_ratio = (int32_t)(actual_mult[i] * 65536.0f);
                phase_accumulator[i] += (delta * fixed_ratio);
            }
            
            // Check Phase Rollover
            int32_t steps_to_fire = phase_accumulator[i] / 65536;
            
            if (steps_to_fire != 0) {
                // Fire Step Pulse
                gpio_set_level(step_pins[i], 1);
                step_pin_state[i] = 1;

                // Handle Direction (Wait 1 tick for DIR setup time ideally, but ESP32 GPIO is fast enough)
                if (steps_to_fire > 0) {
                    gpio_set_level(dir_pins[i], 1);
                    target_steps_generated[i] += 1;
                    phase_accumulator[i] -= 65536; // Only consume 1 step per ISR to prevent pin overriding
                } else {
                    gpio_set_level(dir_pins[i], 0);
                    target_steps_generated[i] -= 1;
                    phase_accumulator[i] += 65536; // Neg math for inverse direction
                }
            }
        }
    }
    return false; // Yielding context not directly required here
}

static void init_stepper_pins() {
    for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
        gpio_config_t step_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << step_pins[i]) | (1ULL << dir_pins[i]),
            .pull_down_en = 0, 
            .pull_up_en = 0
        };
        gpio_config(&step_conf);
        gpio_set_level(step_pins[i], 0);
        gpio_set_level(dir_pins[i], 0);
    }
    ESP_LOGI(TAG, "Stepper Output Pins Initialized.");
}

static void init_els_gptimer() {
    gptimer_handle_t els_timer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz resolution (1 tick = 1 microsecond)
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &els_timer));
    
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 20, // 50kHz = 20 ticks of 1MHz -> 20 microsecond Pulse Width!
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(els_timer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = els_stepper_isr,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(els_timer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(els_timer));
    ESP_ERROR_CHECK(gptimer_start(els_timer));
    
    ESP_LOGI(TAG, "50kHz GPTimer ELS Interrupt Engine Started!");
}

// -----------------------------------------------------------------------------
// USB EVENT CALLBACKS
// -----------------------------------------------------------------------------

const uint8_t hid_report_descriptor[] = {
    HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2), HID_USAGE(0x01),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),

    // Input Report
    HID_REPORT_ID(REPORT_ID_INPUT) HID_USAGE(0x02), HID_LOGICAL_MIN(0x00),
    HID_LOGICAL_MAX_N(0xff, 2), HID_REPORT_SIZE(8),
    HID_REPORT_COUNT(sizeof(hid_input_report_t)), // Payload size
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

    // Output Report
    HID_REPORT_ID(REPORT_ID_OUTPUT) HID_USAGE(0x03), HID_LOGICAL_MIN(0x00),
    HID_LOGICAL_MAX_N(0xff, 2), HID_REPORT_SIZE(8),
    HID_REPORT_COUNT(sizeof(hid_output_report_t)), // Payload size
    HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

    HID_COLLECTION_END};

const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
  return 0; // Not implemented for this mock
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == REPORT_ID_OUTPUT) {
    if (bufsize == sizeof(hid_output_report_t)) {
      memcpy(&active_setup, buffer, bufsize);
      ESP_LOGI(TAG, "Backend received new FrontEnd mapping!");
      for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
        if (active_setup.track_spindle[i]) {
          ESP_LOGI(TAG, "  Axis%d Tracking Spindle! Ratio: %.4f", i, (double)active_setup.mult_axis[i]);
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
// SLOW RTOS BACKGROUND LOOP (ACCELERATION & USB)
// -----------------------------------------------------------------------------
void backend_els_task(void *pvParameters) {
  hid_input_report_t in_report = {.status_flags = 0};
  uint32_t loop_counter = 0;
  
  // Throttle HIL printouts to 1Hz
  uint32_t hil_print_counter = 0; 
  int64_t slow_loop_last_spindle = 0;

  while (1) {
    in_report.status_flags = 0; 

    // 1. Absolute Spindle position (hw_count + 64-bit overflow accumulator)
    int hw_spindle = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(spindle_pcnt_unit, &hw_spindle));
    int64_t abs_spindle = spindle_overflow_accum + hw_spindle;
    in_report.spindle_counts = (int32_t)abs_spindle; // truncated for HID, frontend handles wrapping

    // RPM math: delta between this slow-tick and last
    int64_t spindle_delta = abs_spindle - slow_loop_last_spindle;
    slow_loop_last_spindle = abs_spindle;
    float current_rpm = ((float)spindle_delta * 6000.0f) / 4000.0f; 
    in_report.spindle_rpm = current_rpm;

    // 2. Slow Background Acceleration Math
    for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
        float target_mult = active_setup.track_spindle[i] ? active_setup.mult_axis[i] : 0.0f;
        float max_change = (active_setup.acceleration[i] == 0) ? 9999.0f : ((float)active_setup.acceleration[i] / 100.0f); 
        
        if (actual_mult[i] < target_mult) {
            actual_mult[i] += max_change;
            if (actual_mult[i] > target_mult) actual_mult[i] = target_mult;
        } else if (actual_mult[i] > target_mult) {
            actual_mult[i] -= max_change;
            if (actual_mult[i] < target_mult) actual_mult[i] = target_mult;
        }

        // Read Local Axis Encoders — absolute position = hw_count + 64-bit overflow
        if (axes[i].enabled) {
            int hw_count = 0;
            ESP_ERROR_CHECK(pcnt_unit_get_count(axes[i].pcnt_unit, &hw_count));
            int64_t abs_count = axes[i].overflow_accum + hw_count;
            in_report.axis[i] = (int32_t)abs_count; // truncated for HID
            
            // HIL VALIDATION PRINTOUT 1HZ
            if (hil_print_counter >= 100 && active_setup.track_spindle[i]) {
                ESP_LOGI(TAG, "[HIL] Axis %d -> Target Generated: %ld | Loopback Abs: %lld | Discrepancy: %lld", 
                         i, (long)target_steps_generated[i], abs_count,
                         (long long)(target_steps_generated[i] - abs_count));
            }
        } else {
            in_report.axis[i] = 0;
        }
    }
    
    if (hil_print_counter >= 100) hil_print_counter = 0;
    hil_print_counter++;

    // 3. Transmit USB Report (Throttled to 50Hz = every 20ms)
    loop_counter++;
    if (loop_counter >= 2) { 
      loop_counter = 0;
      if (tud_mounted() && tud_hid_ready()) {
        tud_hid_report(REPORT_ID_INPUT, &in_report, sizeof(in_report));
      }
    }

    // Run control loop reliably at 100Hz (every 10ms)
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// -----------------------------------------------------------------------------
// USB CONFIGURATION DESCRIPTOR & INIT
// -----------------------------------------------------------------------------
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(hid_report_descriptor), 0x81, 64, 10)
};

void init_backend_device(void) {
  ESP_LOGI(TAG, "Initializing TinyUSB HID Device for ELS Backend...");

  init_encoders();
  init_stepper_pins();
  init_els_gptimer();

  // Give initial dummy config for testing immediately without UI
  active_setup.track_spindle[0] = 1; // Axis 0 (X) tracks Spindle
  active_setup.mult_axis[0] = 1.0f;  // 1 to 1 ratio

  tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
  tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
  tusb_cfg.descriptor.high_speed_config = hid_configuration_descriptor;

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  // Pin to Core 1 for Real-Time Performance
  xTaskCreatePinnedToCore(backend_els_task, "backend_els_task", 4096, NULL, 5, NULL, 1);

  ESP_LOGI(TAG, "Backend initialization complete. Waiting for Frontend to connect.");
}

#endif // ROLE_BACKEND
