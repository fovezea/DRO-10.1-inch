/**
 * @file transport_uart.c
 * @brief Framed binary link over a UART.
 *
 * Default build: UART2 routed to the J3 spare header (TX = GPIO21,
 * RX = GPIO22) at 460800 baud.  UART2's default pins (GPIO16/17) are the V-axis
 * encoder on this board, so the GPIO matrix is used instead.
 *
 * Bench build (CONFIG_DRO_LINK_UART_PORT_0): the link runs on UART0, i.e. the
 * USB-C / CH340 port, and the console is disabled so the wire carries only
 * frames.
 */

#include "transport_uart.h"

#include <inttypes.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "uart_link";

#if CONFIG_DRO_LINK_UART_PORT_0
#define LINK_UART_PORT   UART_NUM_0
#define LINK_UART_TX     1
#define LINK_UART_RX     3
#else
#define LINK_UART_PORT   UART_NUM_2
#ifndef CONFIG_DRO_LINK_UART_TX_GPIO
#define CONFIG_DRO_LINK_UART_TX_GPIO 21
#endif
#ifndef CONFIG_DRO_LINK_UART_RX_GPIO
#define CONFIG_DRO_LINK_UART_RX_GPIO 22
#endif
#define LINK_UART_TX     CONFIG_DRO_LINK_UART_TX_GPIO
#define LINK_UART_RX     CONFIG_DRO_LINK_UART_RX_GPIO
#endif

#ifndef CONFIG_DRO_LINK_UART_BAUD
#define CONFIG_DRO_LINK_UART_BAUD 460800
#endif

#define UART_RX_BUF_SIZE 4096
#define UART_TX_BUF_SIZE 4096

static SemaphoreHandle_t s_tx_lock;
static volatile bool     s_ready;
static volatile uint32_t s_baud = CONFIG_DRO_LINK_UART_BAUD;

static bool uart_ready(void)
{
    return s_ready;
}

esp_err_t transport_uart_set_baud(uint32_t baud)
{
    if (baud < 9600 || baud > 3000000) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_ready) {
        s_baud = baud;
        return ESP_OK;
    }
    if (baud == s_baud) {
        return ESP_OK;
    }

    if (s_tx_lock != NULL) {
        xSemaphoreTake(s_tx_lock, portMAX_DELAY);
    }
    uart_wait_tx_done(LINK_UART_PORT, pdMS_TO_TICKS(200));
    esp_err_t err = uart_set_baudrate(LINK_UART_PORT, baud);
    if (err == ESP_OK) {
        s_baud = baud;
        ESP_LOGI(TAG, "baud rate now %" PRIu32, baud);
    } else {
        ESP_LOGE(TAG, "uart_set_baudrate(%" PRIu32 "): %s", baud, esp_err_to_name(err));
    }
    if (s_tx_lock != NULL) {
        xSemaphoreGive(s_tx_lock);
    }
    return err;
}

uint32_t transport_uart_baud(void)
{
    return s_baud;
}

esp_err_t transport_uart_send(const uint8_t *dst_mac, const uint8_t *data, size_t len)
{
    (void)dst_mac;

    if (!s_ready || data == NULL || len == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_tx_lock != NULL) {
        xSemaphoreTake(s_tx_lock, portMAX_DELAY);
    }
    int written = uart_write_bytes(LINK_UART_PORT, data, len);
    if (s_tx_lock != NULL) {
        xSemaphoreGive(s_tx_lock);
    }
    return (written == (int)len) ? ESP_OK : ESP_FAIL;
}

/* ------------------------------------------------------------------------- */
/* RX: COBS frames are delimited by 0x00                                     */
/* ------------------------------------------------------------------------- */
static void uart_rx_task(void *arg)
{
    (void)arg;

    uint8_t chunk[160];
    uint8_t frame[DRO_LINK_MAX_WIRE];
    size_t  frame_len = 0;

    for (;;) {
        int n = uart_read_bytes(LINK_UART_PORT, chunk, sizeof(chunk), pdMS_TO_TICKS(20));
        for (int i = 0; i < n; i++) {
            uint8_t b = chunk[i];
            if (b == 0x00) {
                if (frame_len > 0) {
                    dro_link_rx(DRO_TR_UART, NULL, frame, frame_len);
                    frame_len = 0;
                }
            } else if (frame_len < sizeof(frame)) {
                frame[frame_len++] = b;
            } else {
                /* Overlong garbage: wait for the next delimiter. */
                frame_len = 0;
            }
        }
    }
}

esp_err_t transport_uart_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    s_tx_lock = xSemaphoreCreateMutex();
    if (s_tx_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }

    uart_config_t cfg = {
        .baud_rate = (int)s_baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(LINK_UART_PORT, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install(%d): %s", (int)LINK_UART_PORT, esp_err_to_name(err));
        return err;
    }

    ESP_ERROR_CHECK(uart_param_config(LINK_UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(LINK_UART_PORT, LINK_UART_TX, LINK_UART_RX,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    s_ready = true;

    if (xTaskCreatePinnedToCore(uart_rx_task, "uart_rx", 4096, NULL, 8, NULL, 0) != pdPASS) {
        s_ready = false;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "UART%d link on TX=IO%d RX=IO%d at %" PRIu32 " baud",
             (int)LINK_UART_PORT, LINK_UART_TX, LINK_UART_RX, s_baud);
    return ESP_OK;
}

const dro_transport_ops_t *transport_uart_ops(void)
{
    static const dro_transport_ops_t ops = {
        .id = DRO_TR_UART,
        .name = "uart",
        .ready = uart_ready,
        .send = transport_uart_send,
        .set_channel = NULL,
        .set_baud = transport_uart_set_baud,
    };
    return &ops;
}
