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
#include "bsp.h"
#include <stdint.h>
#include <string.h>

static const char *TAG = "BACKEND_DEV";

// =============================================================================
// HARDWARE SIMULATION TOGGLE
// Set to 1 to simulate fake encoder data (moving back and forth)
// Set to 0 when real glass scales are physically wired to the ESP32-P4
// =============================================================================
#define SIMULATE_HARDWARE 1

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
} axis_handle_t;

static axis_handle_t axes[NUM_VIRTUAL_AXES];

static pcnt_unit_handle_t spindle_pcnt_unit = NULL;
static pcnt_channel_handle_t spindle_pcnt_chan_a = NULL;
static pcnt_channel_handle_t spindle_pcnt_chan_b = NULL;

static void init_encoders(void) {
    if (SIMULATE_HARDWARE) {
        ESP_LOGI(TAG, "Hardware Simulation is ENABLED. Skipping actual PCNT setup.");
        return;
    }

    // Basic setup from BSP for X, Y, Z, W 
    // (Leaving C disabled to stay within standard 4-PCNT limit if chip doesn't have 8)
    memset(axes, 0, sizeof(axes));
    
    axes[0].enabled = true; axes[0].pin_a = BSP_PIN_ENC_X_A; axes[0].pin_b = BSP_PIN_ENC_X_B;
    axes[1].enabled = true; axes[1].pin_a = BSP_PIN_ENC_Y_A; axes[1].pin_b = BSP_PIN_ENC_Y_B;
    axes[2].enabled = true; axes[2].pin_a = BSP_PIN_ENC_Z_A; axes[2].pin_b = BSP_PIN_ENC_Z_B;
    axes[3].enabled = true; axes[3].pin_a = BSP_PIN_ENC_W_A; axes[3].pin_b = BSP_PIN_ENC_W_B;
    axes[4].enabled = false; axes[4].pin_a = BSP_PIN_ENC_C_A; axes[4].pin_b = BSP_PIN_ENC_C_B;

    int enabled_count = 0;
    for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
        if (!axes[i].enabled) continue;
        enabled_count++;

        ESP_LOGI(TAG, "Initializing PCNT Axis %d on pins %d, %d", i, axes[i].pin_a, axes[i].pin_b);

        pcnt_unit_config_t unit_config = {
            .high_limit = 32000,
            .low_limit = -32000,
        };
        // This will panic gracefully if we exceed the ESP32's hardware PCNT limit
        ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &axes[i].pcnt_unit));

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
        .high_limit = 32000,
        .low_limit = -32000,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&spindle_unit_config, &spindle_pcnt_unit));

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
// USB EVENT CALLBACKS
// -----------------------------------------------------------------------------

// HID Report Descriptor for a custom device
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

static hid_output_report_t active_setup = {0};

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
// ELS CONTROL LOOP & USB TASK
// -----------------------------------------------------------------------------
void backend_els_task(void *pvParameters) {
  hid_input_report_t in_report = {.status_flags = 0};

  // Sim test variables
  int32_t sim_counters[NUM_VIRTUAL_AXES] = {0, 1000, 5000, -5000, 10000};
  int8_t sim_dir[NUM_VIRTUAL_AXES] = {1, 1, 1, -1, 1};
  
  int32_t sim_spindle_count = 0;
  
  uint32_t loop_counter = 0;
  int32_t last_spindle_count = 0;
  
  // Phase Accumulators (Fixed Point Q16.16)
  int32_t phase_accumulator[NUM_VIRTUAL_AXES] = {0};
  
  // Acceleration State
  float actual_mult[NUM_VIRTUAL_AXES] = {0.0f};

  if (SIMULATE_HARDWARE) {
      ESP_LOGW(TAG, "DEBUG SIMULATION RUNNING: Sending fake moving data!");
  }

  while (1) {
    in_report.status_flags = 0; // Clear alarms each loop

    // 1. Read Raw Counts (Spindle + Axes)
    if (SIMULATE_HARDWARE) {
        sim_spindle_count += 40; // Approx 600 RPM if polled at 100Hz and 4000 PPR
        in_report.spindle_counts = sim_spindle_count;
    } else {
        int spindle_count = 0;
        ESP_ERROR_CHECK(pcnt_unit_get_count(spindle_pcnt_unit, &spindle_count));
        in_report.spindle_counts = spindle_count;
    }

    // --- RPM & DELTA CALCULATION ---
    int32_t current_spindle_count = in_report.spindle_counts;
    int32_t delta = current_spindle_count - last_spindle_count;
    last_spindle_count = current_spindle_count;
    
    // (Delta * 100 samples/sec * 60 sec/min) / 4000 PPR
    float current_rpm = ((float)delta * 6000.0f) / 4000.0f; 
    in_report.spindle_rpm = current_rpm;

    // 2. Electronic Leadscrew (ELS) Math & Step Generation
    for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
        
        // A. ACCELERATION RAMPING
        float target_mult = active_setup.track_spindle[i] ? active_setup.mult_axis[i] : 0.0f;
        
        // If acceleration is 0, snap instantly. Otherwise ramp.
        float max_change = (active_setup.acceleration[i] == 0) ? 9999.0f : ((float)active_setup.acceleration[i] / 100.0f); 
        
        if (actual_mult[i] < target_mult) {
            actual_mult[i] += max_change;
            if (actual_mult[i] > target_mult) actual_mult[i] = target_mult;
        } else if (actual_mult[i] > target_mult) {
            actual_mult[i] -= max_change;
            if (actual_mult[i] < target_mult) actual_mult[i] = target_mult;
        }

        // B. SAFETY ALARM (Over-Speed Checking)
        if (actual_mult[i] > 0.0f && active_setup.max_step_hz[i] > 0) {
            // Estimated steps we will generate this 10ms loop
            float expected_steps = (float)delta * actual_mult[i];
            float expected_hz = expected_steps * 100.0f; // * 100 because loop runs every 10ms
            
            if (expected_hz > (float)active_setup.max_step_hz[i]) {
                in_report.status_flags |= (1 << i); // Throw the alarm bit for this axis!
                // NOTE: User requested ALARM ONLY, not physical E-Stop. So we continue math.
            }
        }

        // C. PHASE ACCUMULATION
        // We only generate stepper pulses if in ELS mode. Otherwise, just report regular scales.
        if (active_setup.track_spindle[i]) {
            // Convert clamped floating point ratio to Q16.16
            int32_t fixed_ratio = (int32_t)(actual_mult[i] * 65536.0f);
            
            phase_accumulator[i] += (delta * fixed_ratio);
            
            int32_t steps_to_fire = phase_accumulator[i] / 65536;
            if (steps_to_fire != 0) {
                // To Simulate firing physical steppers, we just add the steps directly to the reporting axis
                if (SIMULATE_HARDWARE) {
                    sim_counters[i] += steps_to_fire;
                    in_report.axis[i] = sim_counters[i];
                }
                phase_accumulator[i] -= (steps_to_fire * 65536);
                
                // Real Hardware: Queue these `steps_to_fire` to the RMT driver here
            }
        } else {
            // Normal DRO Mode: Read Encoders
            if (SIMULATE_HARDWARE) {
                sim_counters[i] += (sim_dir[i] * 5); 
                if (sim_counters[i] > 100000 || sim_counters[i] < -100000) sim_dir[i] *= -1; 
                in_report.axis[i] = sim_counters[i];
            } else {
                if (axes[i].enabled) {
                    int count_val = 0;
                    ESP_ERROR_CHECK(pcnt_unit_get_count(axes[i].pcnt_unit, &count_val));
                    in_report.axis[i] = count_val;
                } else {
                    in_report.axis[i] = 0;
                }
            }
        }
    }

    // 2. Transmit USB Report (Throttled to 50Hz = every 20ms)
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
// USB CONFIGURATION DESCRIPTOR
// -----------------------------------------------------------------------------
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(hid_report_descriptor), 0x81, 64, 10)
};

void init_backend_device(void) {
  ESP_LOGI(TAG, "Initializing TinyUSB HID Device for ELS Backend...");

  init_encoders();

  // Safety fallback just in case no frontend setup arrives immediately
  active_setup.track_spindle[0] = 1;
  active_setup.mult_axis[0] = 1.0f;

  tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
  tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
  tusb_cfg.descriptor.high_speed_config = hid_configuration_descriptor;

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  // Pin to Core 1 for Real-Time Performance
  xTaskCreatePinnedToCore(backend_els_task, "backend_els_task", 4096, NULL, 5, NULL, 1);

  ESP_LOGI(TAG, "Backend initialization complete. Waiting for Frontend to connect.");
}

#endif // ROLE_BACKEND
