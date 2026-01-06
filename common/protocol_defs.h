// common/protocol_defs.h
#ifndef PROTOCOL_DEFS_H
#define PROTOCOL_DEFS_H

// ==========================================
// Electronic Leadscrew Communication Protocol
// ==========================================

// UART Configuration
#define BAUD_RATE       115200

// Command Opcodes
#define CMD_SET_RATIO   0x55 // Format: [0x55] [AxisID] [NumH] [NumL] [DenH] [DenL]
#define CMD_ENABLE_AXIS 0x56 // Format: [0x56] [AxisID] [Enable]

// Axis Identifiers
#define AXIS_ID_1       0x00
#define AXIS_ID_2       0x01

// Packet Constants
#define ENABLE_ON       0x01
#define ENABLE_OFF      0x00

#endif // PROTOCOL_DEFS_H
