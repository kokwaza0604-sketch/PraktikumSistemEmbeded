/**
 * ============================================================================
 * config.h - Konfigurasi Pin dan Parameter
 * STM32_01_Task_Create_Basic
 * 
 * Definisi pin, parameter task, dan konfigurasi hardware
 * untuk program dasar pembuatan task FreeRTOS
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ========================== Konfigurasi LED ============================== */
/* LED Built-in pada Blue Pill (PC13, active LOW) */
#define LED_BUILTIN_PORT        GPIOC
#define LED_BUILTIN_PIN         GPIO_PIN_13
#define LED_BUILTIN_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()

/* LED Eksternal 1 (PB0, active HIGH) */
#define LED_EXT1_PORT           GPIOB
#define LED_EXT1_PIN            GPIO_PIN_0
#define LED_EXT1_CLK_EN()       __HAL_RCC_GPIOB_CLK_ENABLE()

/* LED Eksternal 2 (PB1, active HIGH) */
#define LED_EXT2_PORT           GPIOB
#define LED_EXT2_PIN            GPIO_PIN_1
#define LED_EXT2_CLK_EN()       __HAL_RCC_GPIOB_CLK_ENABLE()

/* ========================== Konfigurasi Tombol =========================== */
/* Tombol pada PA0 (active LOW dengan pull-up) */
#define BTN_PORT                GPIOA
#define BTN_PIN                 GPIO_PIN_0
#define BTN_CLK_EN()            __HAL_RCC_GPIOA_CLK_ENABLE()

/* ========================== Konfigurasi UART ============================= */
/* USART1: PA9 (TX), PA10 (RX) */
#define UART_INSTANCE           USART1
#define UART_BAUDRATE           115200
#define UART_TX_PORT            GPIOA
#define UART_TX_PIN             GPIO_PIN_9
#define UART_RX_PORT            GPIOA
#define UART_RX_PIN             GPIO_PIN_10
#define UART_CLK_EN()           __HAL_RCC_USART1_CLK_ENABLE()
#define UART_GPIO_CLK_EN()      __HAL_RCC_GPIOA_CLK_ENABLE()

/* ======================== Konfigurasi Task FreeRTOS ====================== */
/* Task 1: LED Blink PC13 (500ms) */
#define TASK1_NAME              "Task_LED1"
#define TASK1_STACK_SIZE        256
#define TASK1_PRIORITY          2
#define TASK1_BLINK_PERIOD_MS   500

/* Task 2: LED Blink PB0 (200ms) */
#define TASK2_NAME              "Task_LED2"
#define TASK2_STACK_SIZE        256
#define TASK2_PRIORITY          1
#define TASK2_BLINK_PERIOD_MS   200

/* Task Monitor: Cetak status sistem */
#define MONITOR_TASK_NAME       "Task_Monitor"
#define MONITOR_STACK_SIZE      512
#define MONITOR_PRIORITY        3
#define MONITOR_PERIOD_MS       2000

/* ======================= Konfigurasi Sistem ============================== */
#define SYSTEM_CLOCK_MHZ        72
#define PRINT_BUFFER_SIZE       256
#define MAX_TASK_COUNT          10

/* ======================== Macro Utility ================================== */
#define LED_ON(port, pin)       HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)
#define LED_OFF(port, pin)      HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)
#define LED_TOGGLE(port, pin)   HAL_GPIO_TogglePin(port, pin)

/* Untuk LED built-in (active LOW) */
#define LED_BUILTIN_ON()        HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_RESET)
#define LED_BUILTIN_OFF()       HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET)
#define LED_BUILTIN_TOGGLE()    HAL_GPIO_TogglePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN)

/* Untuk LED eksternal (active HIGH) */
#define LED_EXT1_ON()           HAL_GPIO_WritePin(LED_EXT1_PORT, LED_EXT1_PIN, GPIO_PIN_SET)
#define LED_EXT1_OFF()          HAL_GPIO_WritePin(LED_EXT1_PORT, LED_EXT1_PIN, GPIO_PIN_RESET)
#define LED_EXT1_TOGGLE()       HAL_GPIO_TogglePin(LED_EXT1_PORT, LED_EXT1_PIN)

#endif /* CONFIG_H */
