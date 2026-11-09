/**
 * @file status_led.h
 * @brief Single status LED on IO19 (D3 + R4).
 */

#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdbool.h>

void status_led_init(void);

/**
 * @brief Update the LED pattern.
 *
 * @param link_ok   a frontend session is active
 * @param wifi_err  the ESP-NOW/WiFi stack failed to start
 * @param busy      a command is being executed (short double flash)
 */
void status_led_set_state(bool link_ok, bool wifi_err, bool busy);

/** LED worker task, created by status_led_init(). */
void status_led_task(void *arg);

#endif /* STATUS_LED_H */
