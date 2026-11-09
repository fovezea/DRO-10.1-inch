/**
 * @file status_led.c
 * @brief Status LED patterns:
 *          solid on        link with the frontend is up
 *          slow blink 1 Hz searching (no session yet)
 *          fast blink 5 Hz ESP-NOW/WiFi failed, UART only
 */

#include "status_led.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#ifndef CONFIG_DRO_STATUS_LED_GPIO
#define CONFIG_DRO_STATUS_LED_GPIO 19
#endif

#define LED_GPIO CONFIG_DRO_STATUS_LED_GPIO

static volatile bool s_link_ok;
static volatile bool s_wifi_err;
static volatile bool s_busy;

static void led_write(bool on)
{
#if CONFIG_DRO_STATUS_LED_ACTIVE_HIGH
    gpio_set_level(LED_GPIO, on ? 1 : 0);
#else
    gpio_set_level(LED_GPIO, on ? 0 : 1);
#endif
}

void status_led_init(void)
{
#if LED_GPIO >= 0
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    led_write(false);
#endif
}

void status_led_set_state(bool link_ok, bool wifi_err, bool busy)
{
    s_link_ok = link_ok;
    s_wifi_err = wifi_err;
    s_busy = busy;
}

void status_led_task(void *arg)
{
    (void)arg;

#if LED_GPIO < 0
    vTaskDelete(NULL);
    return;
#endif

    bool phase = false;

    for (;;) {
        if (s_link_ok) {
            led_write(true);
            phase = true;
            vTaskDelay(pdMS_TO_TICKS(100));
        } else if (s_busy) {
            led_write(phase);
            phase = !phase;
            vTaskDelay(pdMS_TO_TICKS(50));
        } else if (s_wifi_err) {
            led_write(phase);
            phase = !phase;
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            led_write(phase);
            phase = !phase;
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
