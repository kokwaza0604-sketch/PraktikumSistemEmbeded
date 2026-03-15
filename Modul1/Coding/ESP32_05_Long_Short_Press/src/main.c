/**
 * ==========================================================================
 *  ESP32_05_Long_Short_Press — Deteksi Durasi Tekan Tombol
 * ==========================================================================
 *  Modul   : 01 - GPIO Digital I/O
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF (PlatformIO)
 *
 *  Deskripsi:
 *    Program mengukur durasi penekanan tombol:
 *      - Short press (< 1 detik)  → toggle LED_SHORT
 *      - Long press  (≥ 1 detik)  → toggle LED_LONG
 *    Durasi diukur dari saat tombol ditekan sampai dilepas.
 *    Aksi dilakukan saat tombol dilepas (on release).
 *
 *  Rangkaian / Wiring:
 *    Button:
 *      ESP32 GPIO4 ──┬── Resistor 10kΩ ── VCC 3.3V  (pull-up)
 *                     └── Push Button ── GND
 *      (Ditekan = LOW, Dilepas = HIGH)
 *
 *    LED Short Press:
 *      ESP32 GPIO16 ──► Resistor 220Ω ──► LED1 Anoda | Katoda ──► GND
 *
 *    LED Long Press:
 *      ESP32 GPIO17 ──► Resistor 220Ω ──► LED2 Anoda | Katoda ──► GND
 *
 *  Komponen:
 *    - 1x Push button + 1x Resistor 10kΩ (pull-up)
 *    - 2x LED (warna berbeda: misal hijau=short, merah=long)
 *    - 2x Resistor 220Ω
 *    - Breadboard + kabel jumper
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "LONG_SHORT";

/* ── State machine ────────────────────────────────────────────────────── */
typedef enum {
    STATE_IDLE,             /* Menunggu tombol ditekan */
    STATE_DEBOUNCE_PRESS,   /* Debounce saat ditekan */
    STATE_PRESSED,          /* Tombol sedang ditahan */
    STATE_DEBOUNCE_RELEASE  /* Debounce saat dilepas */
} press_state_t;

/**
 * @brief Inisialisasi GPIO untuk button dan kedua LED
 */
static void gpio_init_all(void)
{
    /* Konfigurasi kedua LED sebagai output */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_SHORT) | (1ULL << LED_LONG),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_SHORT, 0);
    gpio_set_level(LED_LONG, 0);

    /* Konfigurasi Button sebagai input dengan pull-up */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    ESP_LOGI(TAG, "GPIO terinitialisasi — Button:GPIO%d  LED_SHORT:GPIO%d  LED_LONG:GPIO%d",
             BUTTON_PIN, LED_SHORT, LED_LONG);
}

/**
 * @brief Entry point utama
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Long/Short Press Detection ===");
    ESP_LOGI(TAG, "Short press (< %d ms) → toggle LED SHORT", LONG_PRESS_MS);
    ESP_LOGI(TAG, "Long  press (>= %d ms) → toggle LED LONG", LONG_PRESS_MS);
    gpio_init_all();

    press_state_t state = STATE_IDLE;
    int64_t press_start_ms  = 0;    /* Waktu mulai ditekan */
    int64_t debounce_start  = 0;    /* Waktu mulai debounce */
    bool led_short_state    = false;
    bool led_long_state     = false;

    while (1) {
        int btn_level = gpio_get_level(BUTTON_PIN);     /* 0=ditekan, 1=dilepas */
        int64_t now = esp_timer_get_time() / 1000;      /* ms */

        switch (state) {
            case STATE_IDLE:
                if (btn_level == 0) {
                    /* Tombol mulai ditekan → debounce */
                    state = STATE_DEBOUNCE_PRESS;
                    debounce_start = now;
                }
                break;

            case STATE_DEBOUNCE_PRESS:
                if (btn_level == 1) {
                    /* Dilepas sebelum debounce → noise */
                    state = STATE_IDLE;
                } else if ((now - debounce_start) >= DEBOUNCE_MS) {
                    /* Debounce selesai, tombol valid ditekan */
                    state = STATE_PRESSED;
                    press_start_ms = now;
                    ESP_LOGD(TAG, "Tombol ditekan (valid)");
                }
                break;

            case STATE_PRESSED:
                if (btn_level == 1) {
                    /* Tombol dilepas → debounce release */
                    state = STATE_DEBOUNCE_RELEASE;
                    debounce_start = now;
                }
                break;

            case STATE_DEBOUNCE_RELEASE:
                if (btn_level == 0) {
                    /* Ditekan lagi selama debounce → masih ditahan */
                    state = STATE_PRESSED;
                } else if ((now - debounce_start) >= DEBOUNCE_MS) {
                    /* Release terkonfirmasi — hitung durasi */
                    int64_t duration = now - press_start_ms;

                    if (duration >= LONG_PRESS_MS) {
                        /* Long press */
                        led_long_state = !led_long_state;
                        gpio_set_level(LED_LONG, (uint32_t)led_long_state);
                        ESP_LOGI(TAG, "LONG PRESS  (%lld ms) — LED_LONG %s",
                                 duration, led_long_state ? "ON" : "OFF");
                    } else {
                        /* Short press */
                        led_short_state = !led_short_state;
                        gpio_set_level(LED_SHORT, (uint32_t)led_short_state);
                        ESP_LOGI(TAG, "SHORT PRESS (%lld ms) — LED_SHORT %s",
                                 duration, led_short_state ? "ON" : "OFF");
                    }

                    state = STATE_IDLE;
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}
