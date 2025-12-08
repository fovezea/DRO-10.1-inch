#include "fpga_protocol.h"
#include <string.h>

// Standard CRC-8 polynomial (x^8 + x^2 + x + 1) -> 0x07 (or 0xE0 depending on representation)
// Here we use a common implementation.
uint8_t fpga_calc_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        uint8_t extract = data[i];
        for (uint8_t tempI = 8; tempI; tempI--) {
            uint8_t sum = (crc ^ extract) & 0x01;
            crc >>= 1;
            if (sum) {
                crc ^= 0x8C;
            }
            extract >>= 1;
        }
    }
    return crc;
}

uint16_t fpga_build_packet(uint8_t *buffer, uint8_t cmd_id, const uint8_t *payload, uint8_t payload_len)
{
    uint16_t idx = 0;
    
    // Header
    buffer[idx++] = FPGA_PACKET_HEADER_0;
    buffer[idx++] = FPGA_PACKET_HEADER_1;
    
    // Length (Payload length)
    buffer[idx++] = payload_len;
    
    // Command ID
    buffer[idx++] = cmd_id;
    
    // Payload
    if (payload && payload_len > 0) {
        memcpy(&buffer[idx], payload, payload_len);
        idx += payload_len;
    }
    
    // Calculate CRC over Length, CmdID, and Payload
    // Start from idx=2 (Length byte)
    // Number of bytes = 1 (Len) + 1 (Cmd) + payload_len
    uint8_t crc = fpga_calc_crc8(&buffer[2], 2 + payload_len);
    buffer[idx++] = crc;
    
    // Footer
    buffer[idx++] = FPGA_PACKET_FOOTER;
    
    return idx;
}
