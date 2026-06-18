/**
 * @file config.h
 * @brief Configuration for SPI Loopback Test
 * 
 * Hardware: ESP32 DevKit V1
 * Connection: Wire MOSI (GPIO13) directly to MISO (GPIO12) for loopback
 * 
 * Pin Mapping:
 *   MOSI  -> GPIO13
 *   MISO  -> GPIO12
 *   SCLK  -> GPIO14
 *   CS    -> GPIO15
 */

#ifndef CONFIG_H
#define CONFIG_H

// SPI Host selection
#define SPI_HOST_ID         SPI2_HOST       // HSPI

// SPI Pin definitions
#define PIN_NUM_MOSI        13
#define PIN_NUM_MISO        12
#define PIN_NUM_SCLK        14
#define PIN_NUM_CS          15

// SPI Configuration
#define SPI_CLOCK_SPEED     (1 * 1000 * 1000)   // 1 MHz
#define SPI_MODE            0                     // SPI Mode 0 (CPOL=0, CPHA=0)
#define SPI_MAX_TRANSFER    64                    // Maximum transfer size in bytes

// Test configuration
#define NUM_TEST_PATTERNS   5
#define TEST_DELAY_MS       2000                  // Delay between tests

#endif // CONFIG_H
