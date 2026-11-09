/**
 * @file console_cmds.h
 * @brief Serial console (UART0 / CH340) commands for bring-up and setup.
 */

#ifndef CONSOLE_CMDS_H
#define CONSOLE_CMDS_H

#include "esp_err.h"

/** Install the REPL on UART0 and register every command. */
esp_err_t console_init(void);

#endif /* CONSOLE_CMDS_H */
