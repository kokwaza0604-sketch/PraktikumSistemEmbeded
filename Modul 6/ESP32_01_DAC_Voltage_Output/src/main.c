/**
 * ==========================================================================
 * PROGRAM 01: DAC Voltage Output (Output Tegangan DAC)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Program ini mengatur output DAC ke tegangan tertentu (0V, 0.825V,
 *   1.65V, 2.475V, 3.3V) pada ESP32. DAC menggunakan resolusi 8-bit
 *   (0-255) sehingga nilai = tegangan * 255 / 3.3.
 *   Untuk ESP32-S2/S3 yang tidak memiliki DAC, digunakan LEDC PWM
 *   sebagai alternatif.
 *
 * Koneksi Hardware:
 *   - ESP32: DAC1 = GPIO25 (output analog)
 *   - Multimeter pada GPIO25 untuk mengukur tegangan
 *   - ESP32-S2/S3: GPIO18 (PWM output + RC filter)
 *
 * API yang digunakan:
 *   - dac_output_enable()   : Mengaktifkan channel DAC
 *   - dac_output_voltage()  : Mengatur tegangan output DAC
 *   - LEDC PWM              : Alternatif untuk S2/S3
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* Tag untuk logging */
static const char *TAG = "DAC_VOLTAGE";

/*
 * Deteksi ketersediaan DAC berdasarkan target chip
 * ESP32 original memiliki DAC, ESP32-S2/S3 tidak
 */
#if CONFIG_IDF_TARGET_ESP32
    #define HAS_DAC 1
    #include "driver/dac.h"
    #define DAC_CHAN    DAC_CHANNEL_1   /* GPIO25 pada ESP32 */
    #define DAC_GPIO   25
#else
    #define HAS_DAC 0
    #include "driver/ledc.h"
    #define PWM_GPIO   18
    #define PWM_TIMER  LEDC_TIMER_0
    #define PWM_CHANNEL LEDC_CHANNEL_0
    #define PWM_FREQ   5000            /* Frekuensi PWM 5kHz */
    #define PWM_RES    LEDC_TIMER_8_BIT /* Resolusi 8-bit (0-255) */
#endif

/* Daftar tegangan target dalam miliVolt */
static const float target_voltages[] = {0.0f, 0.825f, 1.65f, 2.475f, 3.3f};
static const int num_voltages = sizeof(target_voltages) / sizeof(target_voltages[0]);

/**
 * Konversi tegangan ke nilai DAC 8-bit
 * Rumus: nilai = tegangan * 255 / 3.3
 */
static uint8_t voltage_to_dac(float voltage)
{
    if (voltage <= 0.0f) return 0;
    if (voltage >= 3.3f) return 255;
    return (uint8_t)((voltage * 255.0f) / 3.3f);
}

#if !HAS_DAC
/**
 * Inisialisasi LEDC PWM sebagai pengganti DAC
 * Digunakan pada ESP32-S2/S3 yang tidak memiliki DAC
 */
static void pwm_init(void)
{
    /* Konfigurasi timer LEDC */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num        = PWM_TIMER,
        .duty_resolution = PWM_RES,
        .freq_hz         = PWM_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_conf);

    /* Konfigurasi channel LEDC */
    ledc_channel_config_t ch_conf = {
        .gpio_num   = PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_CHANNEL,
        .timer_sel  = PWM_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ch_conf);

    ESP_LOGW(TAG, "DAC tidak tersedia! Menggunakan LEDC PWM pada GPIO%d", PWM_GPIO);
    ESP_LOGW(TAG, "Hubungkan RC low-pass filter untuk output analog");
}

/**
 * Atur duty cycle PWM sesuai nilai 0-255
 */
static void pwm_set_value(uint8_t value)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, value);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 DAC Voltage Output ===");

#if HAS_DAC
    /* Inisialisasi DAC pada ESP32 original */
    ESP_LOGI(TAG, "Menggunakan DAC Channel 1 (GPIO%d)", DAC_GPIO);
    dac_output_enable(DAC_CHAN);
    ESP_LOGI(TAG, "DAC diaktifkan, resolusi 8-bit (0-255)");
#else
    /* Inisialisasi PWM pada ESP32-S2/S3 */
    pwm_init();
#endif

    ESP_LOGI(TAG, "Akan menampilkan %d level tegangan berbeda", num_voltages);
    ESP_LOGI(TAG, "-------------------------------------------");

    int step = 0;
    while (1) {
        /* Ambil tegangan target saat ini */
        float voltage = target_voltages[step];
        uint8_t dac_val = voltage_to_dac(voltage);

        /* Terapkan nilai ke DAC atau PWM */
#if HAS_DAC
        dac_output_voltage(DAC_CHAN, dac_val);
#else
        pwm_set_value(dac_val);
#endif

        /* Tampilkan informasi ke serial monitor */
        /* Format: DATA:<step>,<voltage>,<dac_value> */
        printf("DATA:%d,%.3f,%d\n", step, voltage, dac_val);
        ESP_LOGI(TAG, "[Step %d/%d] Target: %.3fV | DAC Value: %d/255",
                 step + 1, num_voltages, voltage, dac_val);

        /* Tunggu 2 detik sebelum pindah ke tegangan berikutnya */
        vTaskDelay(pdMS_TO_TICKS(2000));

        /* Lanjut ke step berikutnya, kembali ke awal jika sudah selesai */
        step = (step + 1) % num_voltages;

        if (step == 0) {
            ESP_LOGI(TAG, "--- Siklus selesai, mengulang ---");
        }
    }
}
