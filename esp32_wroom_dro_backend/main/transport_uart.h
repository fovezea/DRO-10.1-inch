/**
 * @file transport_uart.h
 * @brief Wired link transport (UART2 on J3, or UART0 in bench mode).
 */

#ifndef TRANSPORT_UART_H
#define TRANSPORT_UART_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "link.h"

esp_err_t transport_uart_init(void);
bool transport_uart_ready(void);
esp_err_t transport_uart_send(const uint8_t *dst_mac, const uint8_t *data, size_t len);
esp_err_t transport_uart_set_baud(uint32_t baud);
uint32_t transport_uart_baud(void);
const dro_transport_ops_t *transport_uart_ops(void);

#endif /* TRANSPORT_UART_H */
