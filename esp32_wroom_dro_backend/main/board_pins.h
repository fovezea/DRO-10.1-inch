/**
 * @file board_pins.h
 * @brief Pin map of the ESP32-WROOM-32E 5-axis DRO backend board (Rev C).
 *
 * Every pin below was taken from the KiCad netlist of
 * C:/SRC/esp32-DRO-backend/esp32-DRO-backend.kicad_sch, not from memory.
 *
 * ---------------------------------------------------------------------------
 * Scale inputs
 * ---------------------------------------------------------------------------
 * Each DB9 carries 5 V single-ended TTL: pin 3 = A, pin 4 = B, pin 5 = Z,
 * pin 2 = +5 V, pin 1/6 = GND.  The 5 V lines pass through an SRV05-4 ESD
 * array, a 1 k series resistor and a 100 pF capacitor, then through one half
 * of a SN74LVC245A level translator (U3/U4) which drives the 3.3 V ESP32 pin.
 *
 *   axis   A      B      Z      shift register
 *   X      IO34   IO35   IO36   U3, DB9 J4
 *   Y      IO39   IO32   IO33   U3, DB9 J5
 *   Z      IO25   IO26   IO27   U3, DB9 J6
 *   W      IO14   IO13   IO4    U4, DB9 J7
 *   V      IO16   IO17   IO18   U4, DB9 J8
 *
 * Note: IO34..IO39 are input-only pins with no internal pull resistors.
 * That is fine here: the LVC245 drives them push-pull.  The Z lines also have
 * 10 k pull-ups (R21..R25) on the 3.3 V side.
 *
 * ---------------------------------------------------------------------------
 * Everything else
 * ---------------------------------------------------------------------------
 *   IO19        status LED D3 (through R4 1 k, active high)
 *   IO1 / IO3   UART0 TX / RX -> CH340C (USB-C) and J2 pin 3 / pin 4
 *   IO0         BOOT button SW2 (strapping)
 *   EN          RESET button SW1 / CH340 auto-reset
 *   IO21/22/23  J3 spare header (1=3V3, 2=GND, 3=IO21, 4=IO22, 5=IO23)
 *   IO2/5/12/15 not connected anywhere on the board
 *
 * The V axis is the reason UART2's default pins are unusable: GPIO16 is V_A
 * and GPIO17 is V_B.  UART2 is therefore routed to J3 (GPIO21/22) through the
 * GPIO matrix, see Kconfig.
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_AXIS_COUNT 5

typedef struct {
    const char *name;
    int pin_a;
    int pin_b;
    int pin_z;
} board_axis_pins_t;

static const board_axis_pins_t BOARD_AXIS_PINS[BOARD_AXIS_COUNT] __attribute__((unused)) = {
    { .name = "X", .pin_a = 34, .pin_b = 35, .pin_z = 36 },
    { .name = "Y", .pin_a = 39, .pin_b = 32, .pin_z = 33 },
    { .name = "Z", .pin_a = 25, .pin_b = 26, .pin_z = 27 },
    { .name = "W", .pin_a = 14, .pin_b = 13, .pin_z = 4  },
    { .name = "V", .pin_a = 16, .pin_b = 17, .pin_z = 18 },
};

/* Status LED on the front panel header (D3 + R4). */
#ifndef CONFIG_DRO_STATUS_LED_GPIO
#define CONFIG_DRO_STATUS_LED_GPIO 19
#endif

/* Spare header J3, kept as documentation for the wired link. */
#define BOARD_SPARE_GPIO_1 21
#define BOARD_SPARE_GPIO_2 22
#define BOARD_SPARE_GPIO_3 23

#ifdef __cplusplus
}
#endif

#endif /* BOARD_PINS_H */
