#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "usb_reports.h"

// Bring in the conditionally compiled role initializers

#ifdef ROLE_BACKEND
extern void init_backend_device(void);
#endif

void app_main(void)
{
    ESP_LOGI("MAIN", "=================================================");
    ESP_LOGI("MAIN", "    ESP32-P4 USB HID Communication Boilerplate   ");
    ESP_LOGI("MAIN", "=================================================");


#ifdef ROLE_BACKEND
    ESP_LOGI("MAIN", "Current Role: BACKEND (USB HID Device sending Encoder data)");
    init_backend_device();
#endif

#ifdef ROLE_GENERATOR
    ESP_LOGI("MAIN", "Current Role: GENERATOR (HIL Dummy Spindle Simulator)");
    extern void init_generator_device(void);
    init_generator_device();
#endif

    // The main application loops cleanly.
    // RTOS tasks started in init_... handle the heavy lifting.
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
