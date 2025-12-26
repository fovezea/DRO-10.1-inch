#pragma once

/**
 * @file dro_pins.h
 * @brief Centralized Pin Definitions for DRO Project (ESP32-P4)
 * 
 * This file serves as the Single Source of Truth for GPIO usage.
 * ALWAYS check this file before assigning new pins.
 */

/* ========================================================================= */
/*                              SDIO (WiFi)                                  */
/* ========================================================================= */
// NOTE: These pins are configured in sdkconfig (Kconfig) by the esp_hosted component.
// Changing them here WILL NOT affect the driver unless you also update sdkconfig.
//
// CONFIG_ESP_HOSTED_SDIO_PIN_CLK     -> GPIO 18
// CONFIG_ESP_HOSTED_SDIO_PIN_CMD     -> GPIO 19
// CONFIG_ESP_HOSTED_SDIO_PIN_D0      -> GPIO 14
// CONFIG_ESP_HOSTED_SDIO_PIN_D1      -> GPIO 15
// CONFIG_ESP_HOSTED_SDIO_PIN_D2      -> GPIO 16
// CONFIG_ESP_HOSTED_SDIO_PIN_D3      -> GPIO 17
// CONFIG_ESP_HOSTED_SDIO_GPIO_RESET_SLAVE -> GPIO 54

#define PIN_SDIO_CLK            18  // Defined in sdkconfig
#define PIN_SDIO_CMD            19  // Defined in sdkconfig
#define PIN_SDIO_D0             14  // Defined in sdkconfig
#define PIN_SDIO_D1             15  // Defined in sdkconfig
#define PIN_SDIO_D2             16  // Defined in sdkconfig
#define PIN_SDIO_D3             17  // Defined in sdkconfig
#define PIN_WIFI_RESET          53  // Defined in sdkconfig

/* ========================================================================= */
/*                              DISPLAY / TOUCH                              */
/* ========================================================================= */
// Managed by BSP (esp32_p4_function_ev_board)
// DO NOT USE THESE PINS
#define PIN_BSP_I2C_SDA         7   // Touch & Audio Config
#define PIN_BSP_I2C_SCL         8   // Touch & Audio Config
#define PIN_BSP_TOUCH_INT       21
#define PIN_BSP_TOUCH_RST       22
#define PIN_BSP_LCD_BL          23
#define PIN_BSP_LCD_RST         27

/* ========================================================================= */
/*                              AUDIO (I2S)                                  */
/* ========================================================================= */
// Managed by BSP
#define PIN_BSP_I2S_DOUT        9
#define PIN_BSP_I2S_LCLK        10
#define PIN_BSP_I2S_DSIN        11
#define PIN_BSP_I2S_SCLK        12
#define PIN_BSP_I2S_MCLK        13
#define PIN_BSP_AMP_EN          20

/* ========================================================================= */
/*                              SD CARD                                      */
/* ========================================================================= */
// Managed by BSP
#define PIN_BSP_SD_D0           39
#define PIN_BSP_SD_D1           40
#define PIN_BSP_SD_D2           41
#define PIN_BSP_SD_D3           42
#define PIN_BSP_SD_CLK          43
#define PIN_BSP_SD_CMD          44

/* ========================================================================= */
/*                              FPGA COMM                                    */
/* ========================================================================= */
// 16/17 -> Conflict with SDIO
// 8/9   -> Conflict with BSP I2C/I2S
// 50/51 -> Proposed SAFE
#define PIN_FPGA_UART_TX        50
#define PIN_FPGA_UART_RX        51

/* ========================================================================= */
/*                              AVAILABLE PINS                               */
/* ========================================================================= */
// Safe candidates: 24, 25, 28-38, 45-49, 52-53

