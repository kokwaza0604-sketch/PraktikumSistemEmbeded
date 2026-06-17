/**
 * @file config.h
 * @brief Konfigurasi UART Echo
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "driver/uart.h"

/* ── UART Configuration ─────────────────────────── */
#ifndef UART_PORT
#define UART_PORT       UART_NUM_0
#endif

#ifndef UART_BAUD
#define UART_BAUD       115200
#endif

#ifndef BUF_SIZE
#define BUF_SIZE        1024
#endif

#endif /* CONFIG_H */
