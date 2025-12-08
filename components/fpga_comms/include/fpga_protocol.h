#pragma once

#include <stdint.h>
#include <stdbool.h>

// Protocol Constants
#define FPGA_PACKET_HEADER_0    0xAA
#define FPGA_PACKET_HEADER_1    0x55
#define FPGA_PACKET_FOOTER      0x0A

// Command IDs (ESP32 -> FPGA)
#define CMD_SET_RATIO           0x10
#define CMD_SET_ENABLE          0x11
#define CMD_SET_WORK_MODE       0x12
#define CMD_REQ_STATUS          0x20

// Work Modes
#define WORK_MODE_SCREW         0
#define WORK_MODE_FOLLOW        1
#define WORK_MODE_CONICAL       2

// Structure for a parsed packet
typedef struct {
    uint8_t cmd_id;
    uint8_t length;
    uint8_t payload[32]; // Max payload size
    bool valid_crc;
} fpga_packet_t;

/**
 * @brief Calculate CRC8 for data
 * 
 * @param data Pointer to data
 * @param len Length of data
 * @return uint8_t CRC8 value
 */
uint8_t fpga_calc_crc8(const uint8_t *data, uint16_t len);

/**
 * @brief Build a packet into a buffer
 * 
 * @param buffer Output buffer
 * @param cmd_id Command ID
 * @param payload Payload data
 * @param payload_len Length of payload
 * @return uint16_t Total length of the packet
 */
uint16_t fpga_build_packet(uint8_t *buffer, uint8_t cmd_id, const uint8_t *payload, uint8_t payload_len);
