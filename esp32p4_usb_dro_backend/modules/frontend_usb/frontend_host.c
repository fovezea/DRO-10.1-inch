#include "sdkconfig.h"
#include "usb_reports.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "usb/hid_host.h"
#include "usb/usb_host.h"
#include <stdint.h>
#include <string.h>

static const char *TAG = "FRONTEND_HOST";

static hid_host_device_handle_t backend_handle = NULL;
static bool device_connected = false;

// -----------------------------------------------------------------------------
// EVENT HANDLERS
// -----------------------------------------------------------------------------

void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg) {
  uint8_t data[64] = {0};
  size_t data_length = 0;

  switch (event) {
  case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
    ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(
        hid_device_handle, data, 64, &data_length));

    // Expected payload size + 1 byte for Report ID (which is pre-pended by HID
    // host)
    if (data_length > 0) {
      uint8_t report_id = data[0];
      if (report_id == REPORT_ID_INPUT) {
        if (data_length == (sizeof(hid_input_report_t) + 1)) {
          hid_input_report_t *in_report = (hid_input_report_t *)(data + 1);
          // Log the axes to prove we got it
          ESP_LOGI(TAG, "IN Report! 0: %ld, 1: %ld, 2: %ld", in_report->axis[0],
                   in_report->axis[1], in_report->axis[2]);
        } else {
          ESP_LOGW(TAG, "IN Report Length Mismatch! Expected %d, Got %d",
                   sizeof(hid_input_report_t) + 1, data_length);
        }
      }
    }
    break;
  case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HID Device DISCONNECTED");
    ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
    backend_handle = NULL;
    device_connected = false;
    break;
  case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
    ESP_LOGW(TAG, "HID Device TRANSFER_ERROR");
    break;
  default:
    break;
  }
}

void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event, void *arg) {
  switch (event) {
  case HID_HOST_DRIVER_EVENT_CONNECTED:
    ESP_LOGI(TAG, "HID Device CONNECTED");

    const hid_host_device_config_t dev_config = {
        .callback = hid_host_interface_callback, .callback_arg = NULL};

    ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
    ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));

    backend_handle = hid_device_handle;
    device_connected = true;
    break;
  default:
    break;
  }
}

// -----------------------------------------------------------------------------
// SETUP & BACKGROUND TASKS
// -----------------------------------------------------------------------------

void frontend_setup_task(void *pvParameters) {
  hid_output_report_t out_report = {
      .command_code = 1,
  };

  // Example: Map Axis 0 to Spindle with 1.5x multiplier and Axis 1 with 0.5x
  for (int i = 0; i < NUM_VIRTUAL_AXES; i++) {
    out_report.mult_axis[i] = 0;
    out_report.track_spindle[i] = 0;
  }
  out_report.mult_axis[0] = 1.5f;
  out_report.track_spindle[0] = 1;
  out_report.mult_axis[1] = 0.5f;
  out_report.track_spindle[1] = 1;

  // Buffer to hold Report ID + Data
  uint8_t out_buffer[sizeof(hid_output_report_t) + 1];
  out_buffer[0] = REPORT_ID_OUTPUT;

  while (1) {
    if (device_connected && backend_handle != NULL) {
      ESP_LOGI(TAG, "Frontend sending setup mapping...");
      memcpy(out_buffer + 1, &out_report, sizeof(hid_output_report_t));

      esp_err_t err = hid_class_request_set_report(
          backend_handle, HID_REPORT_TYPE_OUTPUT, REPORT_ID_OUTPUT, out_buffer,
          sizeof(out_buffer));
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send SET_REPORT: %s", esp_err_to_name(err));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Resend every 5s
  }
}

static void usb_lib_task(void *arg) {
  const usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
      .peripheral_map = 0, // 0 = HS PHY (USB-C port). We will use HS to HS now!
  };
  ESP_ERROR_CHECK(usb_host_install(&host_config));

  // Notify the init task that USB host is ready
  if (arg != NULL) {
    xTaskNotifyGive((TaskHandle_t)arg);
  }

  while (true) {
    uint32_t event_flags;
    usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      ESP_ERROR_CHECK(usb_host_device_free_all());
      break;
    }
  }

  ESP_ERROR_CHECK(usb_host_uninstall());
  vTaskDelete(NULL);
}

void init_frontend_host(void) {
  ESP_LOGI(TAG, "Initializing Native USB Host (HID)...");

  // USB Host background task. Pass the active task handle so it can notify us
  // when ready
  xTaskCreatePinnedToCore(usb_lib_task, "usb_lib_task", 4096,
                          xTaskGetCurrentTaskHandle(), 5, NULL, 0);

  // Wait up to 1000 ticks for usb_host_install to complete in the background
  // task
  ulTaskNotifyTake(false, 1000);

  // Initialise HID Host
  const hid_host_driver_config_t hid_host_driver_config = {
      .create_background_task = true,
      .task_priority = 5,
      .stack_size = 4096,
      .core_id = 0,
      .callback = hid_host_device_event,
      .callback_arg = NULL};
  ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

  // Start setup loop which occasionally sends Output Reports
  xTaskCreate(frontend_setup_task, "frontend_setup_task", 4096, NULL, 5, NULL);

  ESP_LOGI(TAG, "Frontend initialization complete. Ready to connect Backend.");
}
