/**
 * @file app_main.c
 * @brief ESP32-WROOM-32E 5-axis DRO backend - entry point.
 *
 * Boot order:
 *   NVS -> settings -> status LED -> PCNT axes -> link -> transports -> console
 *
 * The board is a "dumb pipe": it reads the five scale ports and streams raw
 * counts.  All coordinate maths stays in the frontend, per the project's
 * "smart client, dumb backend" rule.
 */

#include <stdarg.h>
#include <stdio.h>

#include "axis_reader.h"
#include "board_pins.h"
#include "config_store.h"
#include "console_cmds.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "link.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "status_led.h"
#include "transport_espnow.h"
#include "transport_uart.h"

static const char *TAG = "main";

#if CONFIG_DRO_LINK_UART_PORT_0
/* The binary link owns UART0 in bench mode: silence the log so nothing but
 * frames appears on the wire. */
static int silent_vprintf(const char *fmt, va_list args)
{
    (void)fmt;
    (void)args;
    return 0;
}
#endif

static void led_state_task(void *arg)
{
    (void)arg;

    for (;;) {
        dro_link_status_t st;
        dro_link_get_status(&st);
        status_led_set_state(st.session_valid, st.wifi_error && !st.wifi_ready, false);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void print_banner(void)
{
    printf("\n");
    printf("==============================================================\n");
    printf("  ESP32-WROOM-32E  5-axis DRO backend  (board Rev C)\n");
    printf("  X IO34/35/36   Y IO39/32/33   Z IO25/26/27\n");
    printf("  W IO14/13/4    V IO16/17/18\n");
    printf("  wired link: UART%d @ %u baud, wireless: ESP-NOW\n",
#if CONFIG_DRO_LINK_UART_PORT_0
           0,
#else
           2,
#endif
           (unsigned)CONFIG_DRO_LINK_UART_BAUD);
    printf("  type 'help' for the console, 'status' for live counts\n");
    printf("==============================================================\n\n");
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init: %s", esp_err_to_name(err));
    }

#if CONFIG_DRO_LINK_UART_PORT_0
    esp_log_set_vprintf(silent_vprintf);
#endif

    status_led_init();
    ESP_ERROR_CHECK(config_store_init());

    ESP_ERROR_CHECK(axis_reader_init());
    ESP_ERROR_CHECK(dro_link_init());

    dro_link_register_transport(transport_uart_ops());
    ESP_ERROR_CHECK(transport_uart_init());

#if CONFIG_DRO_ESPNOW_ENABLE
    dro_link_register_transport(transport_espnow_ops());
    if (transport_espnow_init() != ESP_OK) {
        ESP_LOGW(TAG, "ESP-NOW unavailable, the wired link still works");
    }
#endif

    /* Apply the saved baud rate, Wi-Fi channel and axis flags. */
    dro_link_apply_config();

    if (xTaskCreate(led_state_task, "led_state", 2048, NULL, 2, NULL) != pdPASS) {
        ESP_LOGW(TAG, "could not start LED task");
    }

    print_banner();
    console_init();

    ESP_LOGI(TAG, "backend running");
}
