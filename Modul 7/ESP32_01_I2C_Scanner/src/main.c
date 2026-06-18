/**
 * ESP32_01_I2C_Scanner
 * Modul 06 - I2C & Sensor
 *
 * Memindai bus I2C dari alamat 0x01 hingga 0x7F,
 * mendeteksi perangkat yang terhubung dan menampilkan
 * tabel dengan nama-nama perangkat umum.
 *
 * Menggunakan ESP-IDF v5.x I2C Master API (driver/i2c_master.h)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "I2C_SCANNER";

/* ---- Pin & Konfigurasi I2C ---- */
#if CONFIG_IDF_TARGET_ESP32
#define I2C_SDA 21
#define I2C_SCL 22
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define I2C_SDA 8
#define I2C_SCL 9
#else
#define I2C_SDA 21
#define I2C_SCL 22
#endif
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ 100000

/* ---- Handle I2C bus ---- */
static i2c_master_bus_handle_t bus_handle;

/* ---- Inisialisasi I2C Master ---- */
static void i2c_master_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
}

/* ---- Helper: tulis register (menggunakan device handle) ---- */
static esp_err_t i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t val) {
    uint8_t write_buf[2] = {reg, val};
    return i2c_master_transmit(dev_handle, write_buf, 2, -1);
}

/* ---- Helper: baca register (menggunakan device handle) ---- */
static esp_err_t i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *buf, size_t len) {
    return i2c_master_transmit_receive(dev_handle, &reg, 1, buf, len, -1);
}

/* ---- Nama perangkat I2C yang umum ---- */
static const char* get_device_name(uint8_t addr) {
    switch (addr) {
        case 0x20: return "PCF8574 / MCP23017";
        case 0x21: return "PCF8574 (A0=1)";
        case 0x22: return "PCF8574 (A1=1)";
        case 0x23: return "PCF8574 / BH1750";
        case 0x27: return "PCF8574 (LCD I2C)";
        case 0x3C: return "SSD1306 OLED";
        case 0x3D: return "SSD1306 OLED (alt)";
        case 0x40: return "INA219 / HDC1080 / PCA9685";
        case 0x48: return "ADS1115 / PCF8591 / TMP102";
        case 0x49: return "ADS1115 (A0=1)";
        case 0x50: return "AT24C32 EEPROM";
        case 0x51: return "AT24C32 EEPROM (A0=1)";
        case 0x52: return "AT24C32 EEPROM (A1=1)";
        case 0x57: return "AT24C32 (DS3231 board)";
        case 0x5A: return "MLX90614";
        case 0x5B: return "CCS811";
        case 0x60: return "SI1145 / MCP4725";
        case 0x68: return "MPU6050 / DS3231 RTC";
        case 0x69: return "MPU6050 (AD0=1)";
        case 0x76: return "BMP280 / BME280";
        case 0x77: return "BMP280 / BME280 (alt)";
        default:   return "Tidak dikenal";
    }
}

/* ---- Scan seluruh bus I2C ---- */
static void i2c_scan(void) {
    uint8_t found[128];
    int count = 0;

    printf("\n======================================\n");
    printf("       I2C BUS SCANNER\n");
    printf("======================================\n");
    printf("SDA=GPIO%d  SCL=GPIO%d  Freq=%dHz\n", I2C_SDA, I2C_SCL, I2C_FREQ);
    printf("--------------------------------------\n");

    /* Tabel header */
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    for (int row = 0; row < 8; row++) {
        printf("%02X: ", row * 16);
        for (int col = 0; col < 16; col++) {
            uint8_t addr = (uint8_t)(row * 16 + col);
            if (addr < 0x01 || addr > 0x7F) {
                printf("   ");
                continue;
            }
            /* Gunakan i2c_master_probe() untuk deteksi perangkat */
            esp_err_t ret = i2c_master_probe(bus_handle, addr, 50);
            if (ret == ESP_OK) {
                printf("%02X ", addr);
                found[count++] = addr;
            } else {
                printf("-- ");
            }
        }
        printf("\n");
    }

    printf("--------------------------------------\n");
    printf("Ditemukan %d perangkat.\n\n", count);

    if (count > 0) {
        printf("Daftar perangkat:\n");
        printf("  Alamat  | Nama\n");
        printf("  --------|---------------------------\n");
        for (int i = 0; i < count; i++) {
            printf("  0x%02X    | %s\n", found[i], get_device_name(found[i]));
            /* Format output untuk parsing Python */
            printf("DEVICE_FOUND: addr=0x%02X name=%s\n", found[i], get_device_name(found[i]));
        }
    }
    printf("SCAN_COMPLETE: total=%d\n\n", count);
}

void app_main(void) {
    ESP_LOGI(TAG, "Inisialisasi I2C Master...");
    i2c_master_init();

    /* Suppress unused warnings */
    (void)i2c_write_reg;
    (void)i2c_read_reg;

    while (1) {
        i2c_scan();
        ESP_LOGI(TAG, "Scan berikutnya dalam 5 detik...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
