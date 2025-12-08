#include "fpga_comms.h"
#include "fpga_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

#define TAG "FPGA_COMMS"

// UART Configuration
#define UART_PORT_NUM           UART_NUM_1
#define TX_PIN                  16
#define RX_PIN                  17
#define BAUD_RATE               9600
#define UART_BUF_SIZE           1024

// Task Configuration
#define FPGA_TASK_STACK_SIZE    4096
#define FPGA_TASK_PRIORITY      10 

// Command Queue
#define CMD_QUEUE_SIZE          10
typedef struct {
    uint8_t cmd_id;
    uint8_t payload[32];
    uint8_t len;
} cmd_item_t;

// State Store
static fpga_state_t g_fpga_state = {0};
static SemaphoreHandle_t g_state_mutex = NULL;
static QueueHandle_t g_cmd_queue = NULL;

// Internal Helpers
static void fpga_comms_task(void *arg);
static void process_incoming_byte(uint8_t byte);

esp_err_t fpga_comms_init(void)
{
    // Create synchronization objects
    g_state_mutex = xSemaphoreCreateMutex();
    g_cmd_queue = xQueueCreate(CMD_QUEUE_SIZE, sizeof(cmd_item_t));
    
    if (!g_state_mutex || !g_cmd_queue) {
        ESP_LOGE(TAG, "Failed to create OS objects");
        return ESP_FAIL;
    }

    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    // Start Task
    xTaskCreate(fpga_comms_task, "fpga_comms", FPGA_TASK_STACK_SIZE, NULL, FPGA_TASK_PRIORITY, NULL);
    
    ESP_LOGI(TAG, "FPGA Comms Initialized (Provisional)");
    return ESP_OK;
}

esp_err_t fpga_comms_send_cmd(uint8_t cmd_id, const uint8_t *payload, uint8_t len)
{
    if (!g_cmd_queue) return ESP_FAIL;
    
    if (len > 32) {
        ESP_LOGE(TAG, "Payload too long");
        return ESP_ERR_INVALID_ARG;
    }
    
    cmd_item_t item;
    item.cmd_id = cmd_id;
    item.len = len;
    if (payload && len > 0) {
        memcpy(item.payload, payload, len);
    }
    
    if (xQueueSend(g_cmd_queue, &item, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Command queue full");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

void fpga_comms_get_state(fpga_state_t *state_out)
{
    if (!g_state_mutex || !state_out) return;
    
    xSemaphoreTake(g_state_mutex, portMAX_DELAY);
    *state_out = g_fpga_state;
    xSemaphoreGive(g_state_mutex);
}

static void fpga_comms_task(void *arg)
{
    uint8_t *rx_buf = (uint8_t *) malloc(UART_BUF_SIZE);
    uint8_t *tx_buf = (uint8_t *) malloc(256); // Packet buffer
    
    cmd_item_t cmd;
    
    while (1) {
        // 1. Check for commands to send
        if (xQueueReceive(g_cmd_queue, &cmd, 0) == pdTRUE) {
            uint16_t tx_len = fpga_build_packet(tx_buf, cmd.cmd_id, cmd.payload, cmd.len);
            uart_write_bytes(UART_PORT_NUM, (const char *)tx_buf, tx_len);
            ESP_LOGD(TAG, "Sent Cmd: 0x%02X", cmd.cmd_id);
        }
        
        // 2. Check for incoming data
        // Read with a short timeout to keep the loop responsive
        int len = uart_read_bytes(UART_PORT_NUM, rx_buf, UART_BUF_SIZE, pdMS_TO_TICKS(20));
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                process_incoming_byte(rx_buf[i]);
            }
        }
        
        // 3. Heartbeat monitor (To be implemented fully)
    }
    
    free(rx_buf);
    free(tx_buf);
    vTaskDelete(NULL);
}

// Simple state machine for parsing could go here.
// For now, we will just count bytes or look for headers if we were implementing RX fully.
// The roadmap says "Phase 3: State Synchronization" is next, so for now RX might just be for ACKs.
static void process_incoming_byte(uint8_t byte)
{
    // Placeholder for RX parsing logic
    // We would look for 0xAA 0x55 ...
}
