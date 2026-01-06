#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "protocol_defs.h"  // Shared protocol definitions

#define TAG "DRO_BACKEND"
#define BUF_SIZE (1024)

// Unit mapping: X=0, Y=1, Z=2, W=3, C=4
#define NUM_AXES 5

// Axis configuration tracking
typedef struct {
    bool enabled;
    int pin_a;
    int pin_b;
    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t pcnt_chan_a;
    pcnt_channel_handle_t pcnt_chan_b;
} axis_handle_t;

static axis_handle_t axes[NUM_AXES];

static void init_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(CONFIG_DRO_UART_PORT_NUM, &uart_config);
    uart_set_pin(CONFIG_DRO_UART_PORT_NUM, CONFIG_DRO_UART_TX_PIN, CONFIG_DRO_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(CONFIG_DRO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    ESP_LOGI(TAG, "UART Initialized at %d baud", BAUD_RATE);
}

static void config_axes_struct(void) {
    // Clear all
    memset(axes, 0, sizeof(axes));

    #ifdef CONFIG_DRO_ENABLE_AXIS_0
    axes[0].enabled = true;
    axes[0].pin_a = CONFIG_DRO_ENC_AXIS_0_A;
    axes[0].pin_b = CONFIG_DRO_ENC_AXIS_0_B;
    #endif

    #ifdef CONFIG_DRO_ENABLE_AXIS_1
    axes[1].enabled = true;
    axes[1].pin_a = CONFIG_DRO_ENC_AXIS_1_A;
    axes[1].pin_b = CONFIG_DRO_ENC_AXIS_1_B;
    #endif

    #ifdef CONFIG_DRO_ENABLE_AXIS_2
    axes[2].enabled = true;
    axes[2].pin_a = CONFIG_DRO_ENC_AXIS_2_A;
    axes[2].pin_b = CONFIG_DRO_ENC_AXIS_2_B;
    #endif

    #ifdef CONFIG_DRO_ENABLE_AXIS_3
    axes[3].enabled = true;
    axes[3].pin_a = CONFIG_DRO_ENC_AXIS_3_A;
    axes[3].pin_b = CONFIG_DRO_ENC_AXIS_3_B;
    #endif

    #ifdef CONFIG_DRO_ENABLE_AXIS_4
    axes[4].enabled = true;
    axes[4].pin_a = CONFIG_DRO_ENC_AXIS_4_A;
    axes[4].pin_b = CONFIG_DRO_ENC_AXIS_4_B;
    #endif
}

static void init_encoders(void) {
    config_axes_struct();

    int enabled_count = 0;
    for (int i = 0; i < NUM_AXES; i++) {
        if (!axes[i].enabled) continue;
        enabled_count++;

        ESP_LOGI(TAG, "Initializing Axis %d on pins %d, %d", i, axes[i].pin_a, axes[i].pin_b);

        pcnt_unit_config_t unit_config = {
            .high_limit = 32000,
            .low_limit = -32000,
        };
        // This will fail if we run out of PCNT units (e.g. >4 on ESP32-S3)
        // We let it panic/error so the user knows configuration is invalid for the hardware.
        ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &axes[i].pcnt_unit));

        pcnt_chan_config_t chan_a_config = {
            .edge_gpio_num = axes[i].pin_a,
            .level_gpio_num = axes[i].pin_b, // Initial level pin, logic handled below
        };
        ESP_ERROR_CHECK(pcnt_new_channel(axes[i].pcnt_unit, &chan_a_config, &axes[i].pcnt_chan_a));

        pcnt_chan_config_t chan_b_config = {
            .edge_gpio_num = axes[i].pin_b,
            .level_gpio_num = axes[i].pin_a,
        };
        ESP_ERROR_CHECK(pcnt_new_channel(axes[i].pcnt_unit, &chan_b_config, &axes[i].pcnt_chan_b));

        // Quadrature Logic: 4X resolution (Count on all edges)
        // POS EDGE A, HIGH B -> DEC
        // POS EDGE A, LOW B  -> INC
        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(axes[i].pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(axes[i].pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
        
        // POS EDGE B, HIGH A -> INC
        // POS EDGE B, LOW A  -> DEC
        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(axes[i].pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(axes[i].pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        ESP_ERROR_CHECK(pcnt_unit_enable(axes[i].pcnt_unit));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(axes[i].pcnt_unit));
        ESP_ERROR_CHECK(pcnt_unit_start(axes[i].pcnt_unit));
    }

    ESP_LOGI(TAG, "Encoders initialized. Enabled axes: %d", enabled_count);
}

// Simple protocol: Frontend asks for data, Backend streams it. 
// "Smart Client, Dumb Backend": We stream 5 integers (one for each potential axis)
// regardless of how many are actually enabled. This keeps parsing simple.
// Disabled axes just send '0'.
static void comms_task(void *arg) {
    char tx_buf[128];
    
    #ifdef CONFIG_DRO_DEBUG_SIMULATE_DATA
    ESP_LOGW(TAG, "DEBUG SIMULATION ENABLED: Generating fake encoder data!");
    int32_t sim_counters[NUM_AXES] = {0, 1000, 5000, -5000, 10000};
    int8_t sim_dir[NUM_AXES] = {1, 1, 1, -1, 1};
    #endif

    while (1) {
        int count_vals[NUM_AXES];
        for (int i = 0; i < NUM_AXES; i++) {
            if (axes[i].enabled) {
                #ifdef CONFIG_DRO_DEBUG_SIMULATE_DATA
                    // Simulate movement: Move back and forth
                    sim_counters[i] += (sim_dir[i] * 10); // Move 10 counts per cycle
                    if (sim_counters[i] > 20000 || sim_counters[i] < -20000) sim_dir[i] *= -1; // Reverse
                    count_vals[i] = sim_counters[i];
                #else
                    ESP_ERROR_CHECK(pcnt_unit_get_count(axes[i].pcnt_unit, &count_vals[i]));
                #endif
            } else {
                count_vals[i] = 0; // Placeholder for disabled/missing axis
            }
        }

        // --- PROTOCOL: STREAMING ---
        // Packet: [SYNC1] [SYNC2] [Ax0:4] [Ax1:4] [Ax2:4] [Ax3:4] [Ax4:4] [FOOTER]
        int offset = 0;
        tx_buf[offset++] = REPORT_SYNC_1;
        tx_buf[offset++] = REPORT_SYNC_2;

        memcpy(&tx_buf[offset], &count_vals[0], sizeof(int32_t) * NUM_AXES);
        offset += sizeof(int32_t) * NUM_AXES;

        tx_buf[offset++] = REPORT_FOOTER;

        // Send over UART
        uart_write_bytes(CONFIG_DRO_UART_PORT_NUM, tx_buf, offset);
        
        // Optional: Keep log for debug (verbose) make it DEBUG level so it doesn't flood 
        ESP_LOGD(TAG, "Sent %d bytes: [Ax0:%d] [Ax1:%d]...", offset, count_vals[0], count_vals[1]);

        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz update rate
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32 DRO Backend...");
    init_uart();
    init_encoders();
    xTaskCreate(comms_task, "comms_task", 4096, NULL, 5, NULL);
}
