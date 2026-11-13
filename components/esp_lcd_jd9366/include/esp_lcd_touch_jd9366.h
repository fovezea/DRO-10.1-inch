/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file esp_lcd_touch_jd9366.h
 * @brief JD9366 in-cell touch controller driver (I2C, BackDoor protocol)
 *
 * The JD9366 integrates the display controller and touch engine in a single IC.
 * The touch engine communicates over I2C at address 0x68 using a proprietary
 * 32-bit register addressing scheme ("BackDoor" protocol).
 */

#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C 7-bit address of the JD9366 touch controller
 */
#define ESP_LCD_TOUCH_IO_I2C_JD9366_ADDRESS    (0x68)

/**
 * @brief Driver-private configuration passed via esp_lcd_touch_config_t::driver_data
 */
typedef struct {
    uint8_t dev_addr;   /*!< I2C device address (normally ESP_LCD_TOUCH_IO_I2C_JD9366_ADDRESS) */
} esp_lcd_touch_io_jd9366_config_t;

/**
 * @brief I2C panel IO configuration macro for the JD9366 touch controller.
 *
 * Key settings:
 *  - 32-bit command/register addresses
 *  - Control phase disabled (BackDoor protocol handles addressing internally)
 */
#define ESP_LCD_TOUCH_IO_I2C_JD9366_CONFIG()   \
    {                                           \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_JD9366_ADDRESS, \
        .control_phase_bytes = 1,               \
        .dc_bit_offset = 0,                     \
        .lcd_cmd_bits = 32,                     \
        .flags =                                \
        {                                       \
            .disable_control_phase = 1,         \
        }                                       \
    }

/**
 * @brief Create a new JD9366 touch driver instance.
 *
 * Performs the following steps:
 *  1. Optional hardware reset via rst_gpio_num (if >= 0)
 *  2. Software reset (write 0xA5 to 0x40008004)
 *  3. POR firmware refresh (write 0x00 to 0x40008081)
 *  4. Chip ID verification (register 0x40008076, expected 0x9032)
 *
 * @note The I2C master bus must be created before calling this function.
 *
 * @param[in]  io        Panel IO handle (I2C, created with esp_lcd_new_panel_io_i2c)
 * @param[in]  config    Touch configuration (x_max, y_max, rst_gpio_num, ...)
 * @param[out] out_touch Touch instance handle
 * @return
 *      - ESP_OK            on success
 *      - ESP_ERR_NO_MEM    if heap allocation failed
 *      - Other error codes on I2C communication failure
 */
esp_err_t esp_lcd_touch_new_i2c_jd9366(const esp_lcd_panel_io_handle_t io,
                                        const esp_lcd_touch_config_t *config,
                                        esp_lcd_touch_handle_t *out_touch);

#ifdef __cplusplus
}
#endif
