#ifndef BSP_P4_BACKEND_H
#define BSP_P4_BACKEND_H

//=============================================================================
// ESP32 DRO/ELS Backend Board Support Package
// Configures Hardware Pinouts based on Connector 1 & 2
// ** MULTI-BOARD CONFIGURATION VIA KCONFIG **
//=============================================================================

// ============================================================================
// BOARD: ESP32-P4C6-osprey Baseboard
// ============================================================================
#if defined(CONFIG_BOARD_ESP32P4C6_OSPREY)

    // SPINDLE ENCODER
    #define BSP_PIN_SPINDLE_A       0
    #define BSP_PIN_SPINDLE_B       1
    #define BSP_PIN_SPINDLE_Z       2

    // AXIS ENCODERS
    #define BSP_PIN_ENC_X_A         3
    #define BSP_PIN_ENC_X_B         4
    #define BSP_PIN_ENC_Y_A         5
    #define BSP_PIN_ENC_Y_B         6
    #define BSP_PIN_ENC_Z_A         14
    #define BSP_PIN_ENC_Z_B         15
    #define BSP_PIN_ENC_W_A         16
    #define BSP_PIN_ENC_W_B         17
    #define BSP_PIN_ENC_C_A         18
    #define BSP_PIN_ENC_C_B         19

    // STEPPER MOTOR DRIVERS
    #define BSP_PIN_STEPPER_ENABLE  22
    #define BSP_PIN_STEP_X          23
    #define BSP_PIN_DIR_X           24
    #define BSP_PIN_STEP_Y          25
    #define BSP_PIN_DIR_Y           27
    #define BSP_PIN_STEP_Z          28
    #define BSP_PIN_DIR_Z           29
    #define BSP_PIN_STEP_W          30
    #define BSP_PIN_DIR_W           31
    #define BSP_PIN_STEP_C          34
    #define BSP_PIN_DIR_C           35

    // LIMIT SWITCHES / ALARMS
    #define BSP_PIN_LIMIT_X         45
    #define BSP_PIN_LIMIT_Y         46
    #define BSP_PIN_LIMIT_Z         47
    #define BSP_PIN_LIMIT_W         48
    #define BSP_PIN_LIMIT_C         49
    #define BSP_PIN_ESTOP           52
    #define BSP_PIN_ALARM           53

// ============================================================================
// BOARD: ESP32-P4 WT9932P4-TINY
// ============================================================================
#elif defined(CONFIG_BOARD_ESP32P4_TINY)

    // SPINDLE ENCODER
    #define BSP_PIN_SPINDLE_A       0
    #define BSP_PIN_SPINDLE_B       1
    #define BSP_PIN_SPINDLE_Z       2

    // AXIS ENCODERS
    #define BSP_PIN_ENC_X_A         3
    #define BSP_PIN_ENC_X_B         4
    #define BSP_PIN_ENC_Y_A         5
    #define BSP_PIN_ENC_Y_B         6
    #define BSP_PIN_ENC_Z_A         14
    #define BSP_PIN_ENC_Z_B         15
    #define BSP_PIN_ENC_W_A         16
    #define BSP_PIN_ENC_W_B         17
    #define BSP_PIN_ENC_C_A         18
    #define BSP_PIN_ENC_C_B         19

    // STEPPER MOTOR DRIVERS
    #define BSP_PIN_STEPPER_ENABLE  22
    #define BSP_PIN_STEP_X          23
    #define BSP_PIN_DIR_X           10  // Moved from 24 (USB D+)
    #define BSP_PIN_STEP_Y          11  // Moved from 25 (USB D-)
    #define BSP_PIN_DIR_Y           27
    #define BSP_PIN_STEP_Z          28
    #define BSP_PIN_DIR_Z           29
    #define BSP_PIN_STEP_W          30
    #define BSP_PIN_DIR_W           31
    #define BSP_PIN_STEP_C          34
    #define BSP_PIN_DIR_C           12  // Moved from 35 (BOOT)

    // LIMIT SWITCHES / ALARMS
    #define BSP_PIN_LIMIT_X         45
    #define BSP_PIN_LIMIT_Y         46
    #define BSP_PIN_LIMIT_Z         47
    #define BSP_PIN_LIMIT_W         48
    #define BSP_PIN_LIMIT_C         49
    #define BSP_PIN_ESTOP           52
    #define BSP_PIN_ALARM           53

// ============================================================================
// BOARD: ESP32-S3 Generic Dev Board
// ============================================================================
#elif defined(CONFIG_BOARD_ESP32S3)

    // ⚠️ USER TODO: Configure these pins for the ESP32-S3
    // Currently filled with dummy values of '255' to prevent compile if unconfigured
    #define BSP_PIN_SPINDLE_A       255
    #define BSP_PIN_SPINDLE_B       255
    #define BSP_PIN_SPINDLE_Z       255

    #define BSP_PIN_ENC_X_A         255
    #define BSP_PIN_ENC_X_B         255
    #define BSP_PIN_ENC_Y_A         255
    #define BSP_PIN_ENC_Y_B         255
    #define BSP_PIN_ENC_Z_A         255
    #define BSP_PIN_ENC_Z_B         255
    #define BSP_PIN_ENC_W_A         255
    #define BSP_PIN_ENC_W_B         255
    #define BSP_PIN_ENC_C_A         255
    #define BSP_PIN_ENC_C_B         255

    #define BSP_PIN_STEPPER_ENABLE  255
    #define BSP_PIN_STEP_X          255
    #define BSP_PIN_DIR_X           255
    #define BSP_PIN_STEP_Y          255
    #define BSP_PIN_DIR_Y           255
    #define BSP_PIN_STEP_Z          255
    #define BSP_PIN_DIR_Z           255
    #define BSP_PIN_STEP_W          255
    #define BSP_PIN_DIR_W           255
    #define BSP_PIN_STEP_C          255
    #define BSP_PIN_DIR_C           255

    #define BSP_PIN_LIMIT_X         255
    #define BSP_PIN_LIMIT_Y         255
    #define BSP_PIN_LIMIT_Z         255
    #define BSP_PIN_LIMIT_W         255
    #define BSP_PIN_LIMIT_C         255
    #define BSP_PIN_ESTOP           255
    #define BSP_PIN_ALARM           255

#else
    #error "NO E-LEADSCREW BOARD TARGET SELECTED IN MENUCONFIG!"
#endif

// ----------------------------------------------------------------------------
// GLOBAL FRONTEND UART SETTINGS
// ----------------------------------------------------------------------------
/* Ensure RS485 UART pins are safely mapped for the target board.
 * - ESP32-P4C6-osprey: TX=50, RX=51
 * - ESP32-P4 TINY: DO NOT USE IO51 (Hardwired to WS2812 RGB LED)
 */

#endif // BSP_P4_BACKEND_H
