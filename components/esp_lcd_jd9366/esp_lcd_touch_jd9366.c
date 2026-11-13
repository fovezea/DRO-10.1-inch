/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// JD9366 Touch Screen Driver
// The JD9366 integrates an in-cell touch engine alongside the display controller.
// Communication uses a proprietary protocol over I2C (address 0x68) with
// 32-bit register addresses -- referred to as the "BackDoor" protocol by the
// manufacturer.
//
// Key registers:
//   0x40008004  System Soft Reset  (write 0xA5)
//   0x40008081  POR / Firmware refresh  (write 0x00)
//   0x40008076  Chip ID  (read, expected 0x9032)
//   0x20021120  Touch reporting data  (read 60 bytes per poll)

#include <string.h>
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_touch_jd9366.h"

static const char *TAG = "JD9366_Touch";

// ---------------------------------------------------------------------------
// Register address constants (BackDoor protocol)
// ---------------------------------------------------------------------------
#define JD9366T_REG_CHIP_ID     0x40008076  // Chip ID register (read-only, returns 0x9032)
#define JD9366T_REG_SOFT_RST    0x40008004  // System soft reset (write 0xA5)
#define JD9366T_REG_POR_INIT    0x40008081  // POR / firmware refresh (write 0x00)
#define JD9366T_REG_TOUCH_DATA  0x20021120  // Touch reporting data register

#define JD9366T_EXPECTED_ID     0x9032      // Expected chip ID value

// ---------------------------------------------------------------------------
// Configuration constants
// ---------------------------------------------------------------------------
#define JD9366T_TOUCH_DATA_LEN  60          // Bytes to read per touch poll
#define JD9366T_MAX_TOUCH_PTS   5           // Maximum simultaneous touch points

// Forward declarations
static esp_err_t jd9366_enter_backdoor(esp_lcd_touch_handle_t tp);
static esp_err_t jd9366_read_chip_id(esp_lcd_touch_handle_t tp, uint16_t *chip_id);
static esp_err_t touch_jd9366_read_data(esp_lcd_touch_handle_t tp);
static bool      touch_jd9366_get_xy(esp_lcd_touch_handle_t tp,
                                     uint16_t *x, uint16_t *y, uint16_t *strength,
                                     uint8_t *point_num, uint8_t max_point_num);
static esp_err_t touch_jd9366_del(esp_lcd_touch_handle_t tp);
static esp_err_t touch_jd9366_reset(esp_lcd_touch_handle_t tp);

// ---------------------------------------------------------------------------
// Public creation function
// ---------------------------------------------------------------------------

esp_err_t esp_lcd_touch_new_i2c_jd9366(const esp_lcd_panel_io_handle_t io,
                                        const esp_lcd_touch_config_t *config,
                                        esp_lcd_touch_handle_t *out_touch)
{
    esp_err_t ret = ESP_OK;

    ESP_GOTO_ON_FALSE(io,        ESP_ERR_INVALID_ARG, err, TAG, "invalid IO handle");
    ESP_GOTO_ON_FALSE(config,    ESP_ERR_INVALID_ARG, err, TAG, "invalid config");
    ESP_GOTO_ON_FALSE(out_touch, ESP_ERR_INVALID_ARG, err, TAG, "invalid output handle");

    // Allocate the touch handle
    esp_lcd_touch_handle_t tp = heap_caps_calloc(1, sizeof(esp_lcd_touch_t), MALLOC_CAP_DEFAULT);
    ESP_GOTO_ON_FALSE(tp, ESP_ERR_NO_MEM, err, TAG, "no memory for JD9366 touch handle");

    // Populate the handle
    tp->io             = io;
    tp->read_data      = touch_jd9366_read_data;
    tp->get_xy         = touch_jd9366_get_xy;
    tp->del            = touch_jd9366_del;
    tp->data.lock.owner = portMUX_FREE_VAL;
    memcpy(&tp->config, config, sizeof(esp_lcd_touch_config_t));

    // Hardware reset (only if an RST GPIO is specified)
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        ESP_LOGI(TAG, "Performing hardware reset on GPIO %d", tp->config.rst_gpio_num);
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, 0), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, 1), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
    } else {
        // No hardware reset pin -- use software reset via BackDoor
        ret = touch_jd9366_reset(tp);
        ESP_GOTO_ON_ERROR(ret, err_free, TAG, "JD9366 software reset failed");
    }

    ESP_LOGI(TAG, "Initializing JD9366 touch engine");

    // Read and verify chip ID
    uint16_t chip_id = 0;
    ret = jd9366_read_chip_id(tp, &chip_id);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Chip ID: 0x%04X (expected 0x%04X)", chip_id, JD9366T_EXPECTED_ID);
        if (chip_id != JD9366T_EXPECTED_ID) {
            ESP_LOGW(TAG, "Chip ID mismatch -- continuing anyway");
        }
    } else {
        ESP_LOGE(TAG, "Failed to read chip ID");
        goto err_free;
    }

    // Enter BackDoor mode (soft reset + POR refresh)
    ret = jd9366_enter_backdoor(tp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter BackDoor mode");
        goto err_free;
    }

    ESP_LOGI(TAG, "JD9366 touch driver initialized successfully");
    *out_touch = tp;
    return ESP_OK;

err_free:
    free(tp);
err:
    return ret;
}

// ---------------------------------------------------------------------------
// BackDoor initialization
// Resets the touch firmware and refreshes POR parameters so the touch engine
// reports correctly after the LCD is already running.
// ---------------------------------------------------------------------------
static esp_err_t jd9366_enter_backdoor(esp_lcd_touch_handle_t tp)
{
    esp_err_t ret;
    ESP_LOGI(TAG, "Entering BackDoor mode (soft reset + POR refresh)");

    // Step 1: Soft reset the touch IC
    ret = esp_lcd_panel_io_tx_param(tp->io, JD9366T_REG_SOFT_RST, (uint8_t[]){0xa5}, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Soft reset command failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 2: Refresh POR firmware parameters
    ret = esp_lcd_panel_io_tx_param(tp->io, JD9366T_REG_POR_INIT, (uint8_t[]){0x00}, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "POR init command failed: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "BackDoor mode entered successfully");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Chip ID read via BackDoor register 0x40008076
// Expected value: 0x9032 (little-endian: [0]=0x32, [1]=0x90)
// ---------------------------------------------------------------------------
static esp_err_t jd9366_read_chip_id(esp_lcd_touch_handle_t tp, uint16_t *chip_id)
{
    esp_err_t ret;
    uint8_t data[2] = {0};

    ret = esp_lcd_panel_io_rx_param(tp->io, JD9366T_REG_CHIP_ID, data, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read chip ID: %s", esp_err_to_name(ret));
        return ret;
    }

    // Manufacturer byte order: chip_id = (buf[1] << 8) | buf[0]
    *chip_id = (uint16_t)((data[1] << 8) | data[0]);
    ESP_LOGI(TAG, "Chip ID raw bytes: [0]=0x%02X [1]=0x%02X => 0x%04X", data[0], data[1], *chip_id);

    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Touch data polling
// Reads 60 bytes from the touch reporting register and decodes the first
// touch point using the manufacturer's region-based coordinate scheme.
//
// Data format (per touch point, starts at byte 0):
//   [0]  Touch ID / presence flag (non-zero = touch active)
//   [1]  0x00
//   [2]  0xFF
//   [3]  X region (0-2, divides screen width into 3 equal bands)
//   [4]  X coordinate within the region (0-255)
//   [5]  Y region (0-4, divides screen height into 5 equal bands)
//   [6]  Y coordinate within the region (0-255)
// ---------------------------------------------------------------------------
static esp_err_t touch_jd9366_read_data(esp_lcd_touch_handle_t tp)
{
    esp_err_t ret;
    uint8_t buf[JD9366T_TOUCH_DATA_LEN] = {0};

    ret = esp_lcd_panel_io_rx_param(tp->io, JD9366T_REG_TOUCH_DATA, buf, JD9366T_TOUCH_DATA_LEN);
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "Touch data read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "Touch raw: %02x %02x %02x %02x %02x %02x %02x %02x",
             buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    portENTER_CRITICAL(&tp->data.lock);

    if (buf[0] != 0x00) {
        // Touch point detected
        uint8_t x_region = buf[3];  // X band (0-2)
        uint8_t x_coord  = buf[4];  // X position within band (0-255)
        uint8_t y_region = buf[5];  // Y band (0-4)
        uint8_t y_coord  = buf[6];  // Y position within band (0-255)

        // Map region + sub-coord to absolute screen pixels
        uint16_t screen_w  = tp->config.x_max;
        uint16_t screen_h  = tp->config.y_max;
        uint16_t band_w    = screen_w / 3;
        uint16_t band_h    = screen_h / 5;

        uint16_t abs_x = (uint16_t)(x_region * band_w + ((uint32_t)x_coord * band_w) / 255);
        uint16_t abs_y = (uint16_t)(y_region * band_h + ((uint32_t)y_coord * band_h) / 255);

        // Clamp to screen bounds
        if (abs_x > screen_w) abs_x = screen_w;
        if (abs_y > screen_h) abs_y = screen_h;

        tp->data.coords[0].x        = abs_x;
        tp->data.coords[0].y        = abs_y;
        tp->data.coords[0].strength = 50;   // Fixed pressure value
        tp->data.points             = 1;
    } else {
        // No touch
        tp->data.points = 0;
    }

    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Software reset via BackDoor (used when no RST GPIO is available)
// ---------------------------------------------------------------------------
static esp_err_t touch_jd9366_reset(esp_lcd_touch_handle_t tp)
{
    assert(tp != NULL);

    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        // Hardware reset sequence per manufacturer spec
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num,  tp->config.levels.reset), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(100));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    // If no RST GPIO, the BackDoor soft-reset (0x40008004) handles it inside
    // jd9366_enter_backdoor() which is called after this function.

    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Return cached touch coordinates to LVGL
// ---------------------------------------------------------------------------
static bool touch_jd9366_get_xy(esp_lcd_touch_handle_t tp,
                                 uint16_t *x, uint16_t *y, uint16_t *strength,
                                 uint8_t *point_num, uint8_t max_point_num)
{
    assert(tp != NULL);
    assert(x != NULL);
    assert(y != NULL);
    assert(point_num != NULL);
    assert(max_point_num > 0);

    portENTER_CRITICAL(&tp->data.lock);

    *point_num = (tp->data.points > max_point_num) ? max_point_num : tp->data.points;

    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;
        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }

    // Invalidate after reading so a stale point is not repeated
    tp->data.points = 0;

    portEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

// ---------------------------------------------------------------------------
// Deallocate the driver instance
// ---------------------------------------------------------------------------
static esp_err_t touch_jd9366_del(esp_lcd_touch_handle_t tp)
{
    free(tp);
    return ESP_OK;
}
