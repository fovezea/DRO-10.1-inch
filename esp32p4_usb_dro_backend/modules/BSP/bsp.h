#ifndef BSP_P4_BACKEND_H
#define BSP_P4_BACKEND_H

//=============================================================================
// ESP32-P4 DRO/ELS Backend Board Support Package
// Configures Hardware Pinouts based on Connector 1 & 2
// ** UPDATED FOR ESP32-P4 Base Board V1.3 SCHEMATIC SAFE PINS **
//=============================================================================

// ----------------------------------------------------------------------------
// SPINDLE ENCODER (Connector 1)
// ----------------------------------------------------------------------------
#define BSP_PIN_SPINDLE_A       0
#define BSP_PIN_SPINDLE_B       1
#define BSP_PIN_SPINDLE_Z       2

// ----------------------------------------------------------------------------
// AXIS ENCODERS (Input Scales for DRO Readout)
// ----------------------------------------------------------------------------
// Axis 0 (X)
#define BSP_PIN_ENC_X_A         3
#define BSP_PIN_ENC_X_B         4
// Axis 1 (Y)
#define BSP_PIN_ENC_Y_A         5
#define BSP_PIN_ENC_Y_B         6
// Axis 2 (Z)
#define BSP_PIN_ENC_Z_A         14
#define BSP_PIN_ENC_Z_B         15
// Axis 3 (W)
#define BSP_PIN_ENC_W_A         16
#define BSP_PIN_ENC_W_B         17
// Axis 4 (C)
#define BSP_PIN_ENC_C_A         18
#define BSP_PIN_ENC_C_B         19

// ----------------------------------------------------------------------------
// STEPPER MOTOR DRIVERS (Electronic Leadscrew Outputs)
// ----------------------------------------------------------------------------
#define BSP_PIN_STEPPER_ENABLE  22

// Axis 0 (X)
#define BSP_PIN_STEP_X          23
#define BSP_PIN_DIR_X           24
// Axis 1 (Y)
#define BSP_PIN_STEP_Y          25
#define BSP_PIN_DIR_Y           27
// Axis 2 (Z)
#define BSP_PIN_STEP_Z          28
#define BSP_PIN_DIR_Z           29
// Axis 3 (W)
#define BSP_PIN_STEP_W          30
#define BSP_PIN_DIR_W           31
// Axis 4 (C)
#define BSP_PIN_STEP_C          34
#define BSP_PIN_DIR_C           35

// ----------------------------------------------------------------------------
// HARDWARE LIMIT SWITCHES / ALARMS
// ----------------------------------------------------------------------------
// Limit Switches
#define BSP_PIN_LIMIT_X         45
#define BSP_PIN_LIMIT_Y         46
#define BSP_PIN_LIMIT_Z         47
#define BSP_PIN_LIMIT_W         48
#define BSP_PIN_LIMIT_C         49

// Machine Halt / Alarms
#define BSP_PIN_ESTOP           52
#define BSP_PIN_ALARM           53

// ----------------------------------------------------------------------------
// RESERVED / UNAVAILABLE PINS (ESP32-P4 Board V1.3 Internal Connections)
// ----------------------------------------------------------------------------
// DO NOT USE THESE GPIOs in future development unless the corresponding 
// on-board peripherals are physically disabled or unused.

/* 
 * I2C Bus (Shared by Codec, Touch, Camera, etc.)
 *   GPIO 7  - I2C_SDA
 *   GPIO 8  - I2C_SCL
 * 
 * I2S Audio Codec (ES8311)
 *   GPIO 9  - I2S Data/Clock
 *   GPIO 10 - I2S Data/Clock
 *   GPIO 11 - I2S Data/Clock
 *   GPIO 12 - I2S Data/Clock
 *   GPIO 13 - I2S Data/Clock
 * 
 * CSI Camera
 *   GPIO 26 - CSI_RST (Reset line via MOSFET)
 * 
 * USB / UART Mux & Download Switching
 *   GPIO 37 - USB_MUX
 *   GPIO 38 - USB_MUX
 * 
 * Frontend Communication (UART)
 *   GPIO 50 - TX (Frontend)
 *   GPIO 51 - RX (Frontend)
 * 
 * ------------------------------------------------------------------
 * CONDITIONALLY FREE PINS (Peripherals Unpopulated / Disabled):
 * ------------------------------------------------------------------
 * If this BSP is used on a board with an LCD or SD Card, these 
 * pins MUST be avoided. On the headless Backend, they are 100% free:
 * 
 *   LCD Display Control:
 *     GPIO 20 - LCD_BL (Backlight)
 *     GPIO 32 - LCD_RST (Reset)
 *     GPIO 33 - LCD_TE (Tearing Effect)
 * 
 *   TF / SD Card (Unused on Backend):
 *     GPIO 39 - SD_D1
 *     GPIO 40 - SD_D2
 *     GPIO 41 - SD_D3
 *     GPIO 42 - SD_CMD
 *     GPIO 43 - SD_CLK
 *     GPIO 44 - SD_DET (Card Detect)
 * 
 *   Audio Amplifier:
 *     GPIO 53 - Amp Input (Currently safe, mapped to ALARM input)
 * ------------------------------------------------------------------
 */

#endif // BSP_P4_BACKEND_H
