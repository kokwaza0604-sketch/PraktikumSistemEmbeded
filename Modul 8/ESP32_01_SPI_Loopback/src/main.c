/**
 * @file main.c
 * @brief ESP32 SPI Loopback Test
 * @details Tests SPI communication by wiring MOSI to MISO and verifying
 *          data integrity across multiple test patterns.
 *
 * Program: ESP32_01_SPI_Loopback
 * Module:  07 - SPI & Storage
 * 
 * Hardware Connections:
 *   ESP32 GPIO13 (MOSI) --wire--> ESP32 GPIO12 (MISO)
 *   ESP32 GPIO14 (SCLK)
 *   ESP32 GPIO15 (CS)
 *
 * Pin Mapping:
 *   MOSI  = GPIO13
 *   MISO  = GPIO12
 *   SCLK  = GPIO14
 *   CS    = GPIO15
 *
 * Description:
 *   This program performs SPI loopback testing using HSPI (SPI2_HOST).
 *   MOSI is physically connected to MISO so that transmitted data is
 *   received back. Five test patterns are used:
 *     1. Ascending bytes (0x00, 0x01, ..., 0x0F)
 *     2. Alternating 0xAA / 0x55
 *     3. All 0xFF
 *     4. All 0x00
 *     5. ASCII string "Hello SPI!"
 *   Each pattern is transmitted and received, then compared byte-by-byte.
 *   Results are printed as PASS or FAIL with hex dumps.
 *
 * Framework: ESP-IDF
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "SPI_LOOPBACK";

static spi_device_handle_t spi_handle;

/**
 * @brief Initialize SPI bus and add device
 */
static esp_err_t spi_loopback_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_MAX_TRANSFER,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI bus initialized successfully");

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = SPI_MODE,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI device added successfully");

    return ESP_OK;
}

/**
 * @brief Perform SPI transfer and return result
 */
static esp_err_t spi_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len)
{
    spi_transaction_t trans = {
        .length = len * 8,          // Length in bits
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    return spi_device_transmit(spi_handle, &trans);
}

/**
 * @brief Print hex dump of a buffer
 */
static void print_hex(const char *label, const uint8_t *buf, size_t len)
{
    printf("  %s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

/**
 * @brief Compare TX and RX buffers and report result
 */
static bool verify_data(const uint8_t *tx_buf, const uint8_t *rx_buf, size_t len)
{
    int errors = 0;
    for (size_t i = 0; i < len; i++) {
        if (tx_buf[i] != rx_buf[i]) {
            errors++;
            if (errors <= 5) {  // Show first 5 mismatches
                ESP_LOGW(TAG, "  Mismatch at byte %d: TX=0x%02X, RX=0x%02X",
                         (int)i, tx_buf[i], rx_buf[i]);
            }
        }
    }

    if (errors > 0) {
        ESP_LOGE(TAG, "  Bit errors found: %d / %d bytes", errors, (int)len);
        return false;
    }
    return true;
}

/**
 * @brief Run a single test pattern
 */
static bool run_test(const char *test_name, const uint8_t *tx_buf, size_t len)
{
    uint8_t rx_buf[SPI_MAX_TRANSFER] = {0};

    ESP_LOGI(TAG, "--- Test: %s (length=%d) ---", test_name, (int)len);
    print_hex("TX", tx_buf, len);

    esp_err_t ret = spi_transfer(tx_buf, rx_buf, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "  SPI transfer failed: %s", esp_err_to_name(ret));
        printf("  Result: FAIL (transfer error)\n");
        return false;
    }

    print_hex("RX", rx_buf, len);

    bool pass = verify_data(tx_buf, rx_buf, len);
    printf("  Result: %s\n\n", pass ? "PASS" : "FAIL");
    return pass;
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 SPI Loopback Test");
    ESP_LOGI(TAG, "  Module 07 - SPI & Storage");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Pin Configuration:");
    ESP_LOGI(TAG, "  MOSI = GPIO%d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  MISO = GPIO%d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  SCLK = GPIO%d", PIN_NUM_SCLK);
    ESP_LOGI(TAG, "  CS   = GPIO%d", PIN_NUM_CS);
    ESP_LOGI(TAG, "  Clock: %d Hz, Mode: %d", SPI_CLOCK_SPEED, SPI_MODE);
    ESP_LOGI(TAG, "");

    // Initialize SPI
    esp_err_t ret = spi_loopback_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed. Halting.");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    int pass_count = 0;
    int total_tests = NUM_TEST_PATTERNS;

    // ---- Test 1: Ascending bytes ----
    {
        uint8_t tx_buf[16];
        for (int i = 0; i < 16; i++) {
            tx_buf[i] = (uint8_t)i;
        }
        if (run_test("Ascending Bytes (0x00-0x0F)", tx_buf, 16)) {
            pass_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
    }

    // ---- Test 2: Alternating 0xAA / 0x55 ----
    {
        uint8_t tx_buf[16];
        for (int i = 0; i < 16; i++) {
            tx_buf[i] = (i % 2 == 0) ? 0xAA : 0x55;
        }
        if (run_test("Alternating 0xAA/0x55", tx_buf, 16)) {
            pass_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
    }

    // ---- Test 3: All 0xFF ----
    {
        uint8_t tx_buf[16];
        memset(tx_buf, 0xFF, 16);
        if (run_test("All 0xFF", tx_buf, 16)) {
            pass_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
    }

    // ---- Test 4: All 0x00 ----
    {
        uint8_t tx_buf[16];
        memset(tx_buf, 0x00, 16);
        if (run_test("All 0x00", tx_buf, 16)) {
            pass_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
    }

    // ---- Test 5: ASCII "Hello SPI!" ----
    {
        const char *msg = "Hello SPI!";
        size_t len = strlen(msg);
        if (run_test("ASCII 'Hello SPI!'", (const uint8_t *)msg, len)) {
            pass_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
    }

    // ---- Summary ----
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  TEST SUMMARY: %d/%d PASSED", pass_count, total_tests);
    ESP_LOGI(TAG, "  Overall: %s", (pass_count == total_tests) ? "ALL PASS" : "SOME FAILED");
    ESP_LOGI(TAG, "========================================");

    // Cleanup
    spi_bus_remove_device(spi_handle);
    spi_bus_free(SPI_HOST_ID);
    ESP_LOGI(TAG, "SPI bus freed. Test complete.");
}
