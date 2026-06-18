/**
 * ============================================================
 *  KONFIGURASI - STM32 SPI LOOPBACK TEST
 *  Modul 07 - SPI & Storage
 * ============================================================
 *  Board : Blue Pill STM32F103C8T6
 *  SPI   : SPI1 (Full-Duplex Master)
 *  Wiring: MOSI (PA7) -> MISO (PA6) untuk loopback
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== SPI1 Pin Configuration ==================== */
#define SPI1_SCK_PIN            GPIO_PIN_5
#define SPI1_SCK_PORT           GPIOA
#define SPI1_MOSI_PIN           GPIO_PIN_7
#define SPI1_MOSI_PORT          GPIOA
#define SPI1_MISO_PIN           GPIO_PIN_6
#define SPI1_MISO_PORT          GPIOA
#define SPI1_NSS_PIN            GPIO_PIN_4
#define SPI1_NSS_PORT           GPIOA

/* ==================== SPI1 Parameters ==================== */
#define SPI1_PRESCALER          SPI_BAUDRATEPRESCALER_8   /* 72MHz / 8 = 9MHz */
#define SPI1_CPOL               SPI_POLARITY_LOW          /* Clock idle LOW */
#define SPI1_CPHA               SPI_PHASE_1EDGE           /* Sample on 1st edge (Mode 0) */
#define SPI1_DATA_SIZE          SPI_DATASIZE_8BIT
#define SPI1_FIRST_BIT          SPI_FIRSTBIT_MSB
#define SPI1_MODE               SPI_MODE_MASTER
#define SPI1_DIRECTION          SPI_DIRECTION_2LINES      /* Full-duplex */
#define SPI1_NSS_MODE           SPI_NSS_SOFT              /* Software managed NSS */

/* ==================== UART1 Configuration ==================== */
#define UART1_TX_PIN            GPIO_PIN_9
#define UART1_TX_PORT           GPIOA
#define UART1_RX_PIN            GPIO_PIN_10
#define UART1_RX_PORT           GPIOA
#define UART1_BAUDRATE          115200

/* ==================== LED Configuration ==================== */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

/* ==================== Buffer Configuration ==================== */
#define SPI_BUFFER_SIZE         64
#define TEST_DATA_LENGTH        16

/* ==================== Test Configuration ==================== */
#define NUM_TEST_PATTERNS       5
#define TEST_DELAY_MS           3000

#endif /* CONFIG_H */
