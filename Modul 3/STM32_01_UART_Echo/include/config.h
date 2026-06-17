#ifndef CONFIG_H
#define CONFIG_H

/* -------- UART Configuration -------- */
#define UART_BAUD       115200
#define UART_TIMEOUT    1000        /* ms */

/* -------- LED Pin (PC13 on Blue Pill) -------- */
#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13

#endif /* CONFIG_H */
