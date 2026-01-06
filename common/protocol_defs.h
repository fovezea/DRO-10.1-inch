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

// Reporting Protocol (Backend -> Frontend)
// Format: [SYNC_1] [SYNC_2] [X(4)] [Y(4)] [Z(4)] [W(4)] [C(4)] [FOOTER]
#define REPORT_SYNC_1   0xAA
#define REPORT_SYNC_2   0x55
#define REPORT_FOOTER   0x0A

#endif // PROTOCOL_DEFS_H
