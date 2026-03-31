// Define the shared report structures for both Host and Device

#ifndef _USB_REPORTS_H_
#define _USB_REPORTS_H_

#include <stdint.h>

// ======================================================================
// ROLE SELECTION
// Change this #define to securely toggle between FRONTEND and BACKEND Roles
// ======================================================================
// #define ROLE_BACKEND // Uncomment to build Backend Device Role
#define ROLE_FRONTEND // Uncomment to build Frontend Host Role

// Endpoint addresses for custom HID
#define EPNUM_HID 1

#define NUM_VIRTUAL_AXES 5

// --- Input Report (Backend -> Frontend) ---
// Contains position data (pulses) dynamically managed by the backend
typedef struct __attribute__((packed)) {
  uint32_t status_flags;          // e.g., limit switches, alarms
  int32_t axis[NUM_VIRTUAL_AXES]; // Position count per axis
} hid_input_report_t;

// --- Output Report (Frontend -> Backend) ---
// Contains setup instructions and axis mapping (multiplication factors)
typedef struct __attribute__((packed)) {
  float mult_axis[NUM_VIRTUAL_AXES]; // The multiplication factors per axis
  uint8_t track_spindle[NUM_VIRTUAL_AXES]; // Which axes track the spindle
                                           // (boolean or ID mapping)
  uint8_t command_code; // e.g., 1 = Config Update, 2 = Stop, etc.
} hid_output_report_t;

// Report IDs
#define REPORT_ID_INPUT 1
#define REPORT_ID_OUTPUT 2

#endif // _USB_REPORTS_H_
