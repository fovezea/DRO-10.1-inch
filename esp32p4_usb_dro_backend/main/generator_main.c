#include "sdkconfig.h"
#include "usb_reports.h"

#ifdef ROLE_GENERATOR

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "bsp.h"

static const char *TAG = "GENERATOR";

static void generator_task(void *pvParameters) {
    uint8_t state = 0;
    int speed_delay = 5; // ms per quadrature state change (creates ~50Hz steps, 200 counts at 4X)
    
    while(1) {
        // State machine for Quadrature:
        // 00 -> 01 -> 11 -> 10
        switch(state) {
            case 0: 
                gpio_set_level(BSP_PIN_SPINDLE_A, 0); 
                gpio_set_level(BSP_PIN_SPINDLE_B, 0); 
                break;
            case 1: 
                gpio_set_level(BSP_PIN_SPINDLE_A, 0); 
                gpio_set_level(BSP_PIN_SPINDLE_B, 1); 
                break;
            case 2: 
                gpio_set_level(BSP_PIN_SPINDLE_A, 1); 
                gpio_set_level(BSP_PIN_SPINDLE_B, 1); 
                break;
            case 3: 
                gpio_set_level(BSP_PIN_SPINDLE_A, 1); 
                gpio_set_level(BSP_PIN_SPINDLE_B, 0); 
                break;
        }
        
        state = (state + 1) % 4;
        
        // Use vTaskDelay for slow speeds or esp_rom_delay_us for very fast
        vTaskDelay(pdMS_TO_TICKS(speed_delay));
    }
}

void init_generator_device(void) {
    ESP_LOGI(TAG, "Initializing Spindle Simulator Generator...");
    
    // We will simulate the Spindle PCNT outputs by toggling the pins.
    // The user connects these outputs to the Backend's Spindle PCNT inputs.
    // So the BSP_PIN_SPINDLE_A and BSP_PIN_SPINDLE_B on the generator board become OUTPUTS.
    
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << BSP_PIN_SPINDLE_A) | (1ULL << BSP_PIN_SPINDLE_B),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "Generator producing fake pulses on Pins %d and %d", BSP_PIN_SPINDLE_A, BSP_PIN_SPINDLE_B);
    
    // Start the FreeRTOS task
    xTaskCreate(generator_task, "spindle_sim_task", 4096, NULL, 5, NULL);
}

#endif // ROLE_GENERATOR
