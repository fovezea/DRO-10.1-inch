#pragma once
#include <stdint.h>
#include <stdbool.h>

void init_frontend_host(void);

// Returns true if the Backend Board is physically plugged in and recognized
bool frontend_usb_is_connected(void);

// Pack new multiplier logic into the USB Output Report and flag for immediate transmission
void frontend_usb_update_els_config(uint8_t target_axis, float mult, bool track, uint16_t accel, uint16_t max_hz);
