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
// Note: These are configured in sdkconfig, but listed here for reference.
// DO NOT USE THESE PINS FOR ANYTHING ELSE.
#define PIN_SDIO_CLK            18
#define PIN_SDIO_CMD            19
#define PIN_SDIO_D0             14
#define PIN_SDIO_D1             15
#define PIN_SDIO_D2             16  // WARNING: Conflict with generic UART default
#define PIN_SDIO_D3             17  // WARNING: Conflict with generic UART default
#define PIN_WIFI_RESET          54  // Reset line for Slave (ESP32-C6)

/* ========================================================================= */
/*                              FPGA COMM                                    */
/* ========================================================================= */
#define PIN_FPGA_UART_TX        8   // Remapped from 16 to avoid SDIO conflict
#define PIN_FPGA_UART_RX        9   // Remapped from 17 to avoid SDIO conflict

/* ========================================================================= */
/*                              DISPLAY / TOUCH                              */
/* ========================================================================= */
// Managed by esp32_p4_function_ev_board component, but typically:
// MIPI DSI (Dedicated Pins)
// I2C Touch (Generic GPIOs) - Check board schematic if conflicts arise

/* ========================================================================= */
/*                              AVAILABLE PINS                               */
/* ========================================================================= */
// Only assign new peripherals to pins NOT listed above.
