/**
 * ==========================================================================
 * PROGRAM 01: ADC Single Read (Pembacaan ADC Tunggal)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini membaca nilai ADC dari potensiometer yang terhubung ke 
 *   ADC1_CHANNEL_6 (GPIO34 pada ESP32, GPIO4 pada S2/S3) menggunakan
 *   mode single-shot (pembacaan tunggal). Nilai mentah (raw) ditampilkan
 *   setiap 500ms melalui serial monitor.
 * 
 * Koneksi Hardware:
 *   - Potensiometer: VCC → 3.3V, GND → GND, Wiper → GPIO34 (ESP32)
 *   - Atau Wiper → GPIO4 (ESP32-S2/S3)
 * 
 * API yang digunakan (ESP-IDF v5.x Oneshot API):
 *   - adc_oneshot_new_unit()       : Membuat unit handle ADC
 *   - adc_oneshot_config_channel() : Mengatur konfigurasi channel (atenuasi, bitwidth)
 *   - adc_oneshot_read()           : Membaca nilai mentah ADC (single-shot)
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

/* Tag untuk logging */
static const char *TAG = "ADC_SINGLE";

/* 
 * Konfigurasi Channel ADC
 * ESP32   : ADC1_CHANNEL_6 = GPIO34
 * ESP32-S2: ADC1_CHANNEL_3 = GPIO4
 * ESP32-S3: ADC1_CHANNEL_3 = GPIO4
 */
#if CONFIG_IDF_TARGET_ESP32
    #define ADC_CHANNEL     ADC_CHANNEL_6    /* GPIO34 pada ESP32 */
    #define ADC_GPIO_NUM    34
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define ADC_CHANNEL     ADC_CHANNEL_3    /* GPIO4 pada S2/S3 */
    #define ADC_GPIO_NUM    4
#else
    #define ADC_CHANNEL     ADC_CHANNEL_6
    #define ADC_GPIO_NUM    34
#endif

/* Atenuasi: 12dB untuk rentang penuh 0-3.3V */
#define ADC_ATTEN       ADC_ATTEN_DB_12

/* Interval pembacaan dalam milidetik */
#define READ_INTERVAL_MS    500

/**
 * @brief Fungsi utama untuk inisialisasi dan pembacaan ADC
 * 
 * Langkah-langkah:
 * 1. Membuat unit handle ADC1 (adc_oneshot_new_unit)
 * 2. Konfigurasi channel (bitwidth + atenuasi)
 * 3. Loop pembacaan nilai mentah setiap 500ms
 */
void app_main(void)
{
    /* ====== INISIALISASI ADC ====== */

    /* Langkah 1: Buat unit handle ADC1 */
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membuat unit ADC: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Unit ADC1 berhasil dibuat");

    /* Langkah 2: Konfigurasi channel (resolusi 12-bit, atenuasi 12dB) */
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal mengatur konfigurasi channel: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Atenuasi channel diatur ke 12dB (0-3.3V)");

    /* Cetak informasi konfigurasi */
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Single Read - Pembacaan Tunggal");
    ESP_LOGI(TAG, "  Channel : ADC1_CHANNEL_%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Resolusi: 12-bit (0-4095)");
    ESP_LOGI(TAG, "  Atenuasi: 12dB (0-3.3V)");
    ESP_LOGI(TAG, "  Interval: %d ms", READ_INTERVAL_MS);
    ESP_LOGI(TAG, "========================================");

    /* Variabel untuk menyimpan nilai pembacaan */
    int raw_value = 0;
    int counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        /* Baca nilai mentah ADC */
        ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Gagal membaca ADC: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
            continue;
        }

        /* Hitung persentase dari nilai maksimum (4095) */
        float percentage = (raw_value / 4095.0f) * 100.0f;

        /* Tampilkan hasil pembacaan */
        counter++;
        printf("[%04d] ADC Raw: %4d | Persentase: %6.2f%%\n", 
               counter, raw_value, percentage);

        /* Tunggu sebelum pembacaan berikutnya */
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
