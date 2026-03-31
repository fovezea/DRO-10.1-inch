#include "sdkconfig.h"
#include "usb_reports.h"

#ifdef ROLE_BACKEND

#include "class/hid/hid_device.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include <stdint.h>

static const char *TAG = "BACKEND_DEV";

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

// TinyUSB configuration callback
const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return hid_report_descriptor;
}

// Callback invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0; // Not implemented for this mock
}

// Callback invoked when received SET_REPORT control request or output data on
// OUT endpoint
static hid_output_report_t active_setup = {0};

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == REPORT_ID_OUTPUT) {
    // Because the Report Descriptor explicitly defined the Report ID, TinyUSB
    // strips it. The buffer contains just the payload (which is exactly our
    // struct size).
    if (bufsize == sizeof(hid_output_report_t)) {
      // Copy the payload directly into the active_setup struct
      memcpy(&active_setup, buffer, bufsize);

      ESP_LOGI(TAG, "Backend received new FrontEnd mapping!");
      for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
        if (active_setup.track_spindle[i]) {
          ESP_LOGI(TAG, "  Axis%d Tracking Spindle! Ratio: %.4f", i,
                   (double)active_setup.mult_axis[i]);
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
  for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
    in_report.axis[i] = 0;
  }

  // Fixed-Point Phase Accumulators for our axes
  // We use Q16.16 arithmetic for extreme speed. 1.0f = 65536
  int32_t phase_accumulator[NUM_VIRTUAL_AXES] = {0};

  // Mock variable: Simulates the PCNT hardware delta
  // A 10,000 PPR encoder spinning at 600 RPM generates 100,000 pulses/sec.
  // In a 5ms control loop (200Hz), we expect ~500 pulses per loop tick.
  int32_t simulated_spindle_speed_delta = 500;

  uint32_t loop_counter = 0;

  while (1) {
    // 1. Simulating reading the Hardware PCNT (Pulse Counter)
    // In real code: pcnt_unit_get_count(unit, &simulated_spindle_speed_delta);
    int32_t encoder_delta = simulated_spindle_speed_delta;

    // 2. Execute Phase Accumulator (DDS) Math for all active axes
    for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
      if (active_setup.track_spindle[i]) {
        // Convert floating point ratio to Q16.16 fixed-point multiplier
        int32_t fixed_ratio = (int32_t)(active_setup.mult_axis[i] * 65536.0f);

        // Add (Delta * Multiplier) to our phase accumulator
        phase_accumulator[i] += (encoder_delta * fixed_ratio);

        // Check if the accumulator rolled over 1.0 (65536)
        // If so, we need to generate stepper pulses!
        int32_t steps_to_fire = phase_accumulator[i] / 65536;
        if (steps_to_fire != 0) {
          // Update the positional mock to send to the frontend
          in_report.axis[i] += steps_to_fire;

          // Remove the whole-number steps from the accumulator, leaving the
          // remainder
          phase_accumulator[i] -= (steps_to_fire * 65536);

          // In real hardware, here you configure the RMT/MCPWM to fire exactly
          // `steps_to_fire` within the next 5ms window so it perfectly syncs
          // with the spindle.
        }
      }
    }

    // 3. Send the USB Report to the Frontend (Throttle to 50Hz = every 20ms)
    loop_counter++;
    if (loop_counter >= 2) { // Every 2 loops of 10ms = 20ms
      loop_counter = 0;
      if (tud_mounted() && tud_hid_ready()) {
        // By passing REPORT_ID_INPUT, TinyUSB prepends the ID to the payload
        // automatically. We pass the directly aligned payload struct pointer.
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
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, protocol, report descriptor len, EP In
    // address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE,
                       sizeof(hid_report_descriptor), 0x81, 64, 10)};

void init_backend_device(void) {
  ESP_LOGI(TAG, "Initializing TinyUSB HID Device for ELS Backend...");

  // Safety fallback just in case no frontend setup arrives immediately
  active_setup.track_spindle[0] = 1;
  active_setup.mult_axis[0] = 1.0f;

  tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
  // Do NOT override port - TINYUSB_DEFAULT_CONFIG() correctly selects
  // the HS peripheral which is wired to the physical USB-C connector.
  tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
  tusb_cfg.descriptor.high_speed_config = hid_configuration_descriptor;

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  xTaskCreate(backend_els_task, "backend_els_task", 4096, NULL, 5, NULL);

  ESP_LOGI(TAG,
           "Backend initialization complete. Waiting for Frontend to connect.");
}

#endif // ROLE_BACKEND
