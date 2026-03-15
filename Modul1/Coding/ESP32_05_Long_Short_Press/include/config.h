/**
 * ==========================================================================
 *  config.h — Konfigurasi Pin untuk Long/Short Press Detection
 * ==========================================================================
 *  Board Target:
 *    - ESP32 DevKit   : Button GPIO4, LED_SHORT GPIO16, LED_LONG GPIO17
 *    - ESP32-S2       : Button GPIO9, LED_SHORT GPIO33, LED_LONG GPIO34
 *    - ESP32-S3       : Button GPIO0, LED_SHORT GPIO4, LED_LONG GPIO5
 *
 *  Hardware:
 *    - 1x Push button (active-LOW, pull-up internal)
 *    - 2x LED + 2x Resistor 220Ω
 *    - LED_SHORT menyala saat short press (<1s)
 *    - LED_LONG  menyala saat long press  (≥1s)
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ── Pilih pin sesuai board ───────────────────────────────────────────── */
#if defined(CONFIG_IDF_TARGET_ESP32S2)
    #define BUTTON_PIN  GPIO_NUM_9
    #define LED_SHORT   GPIO_NUM_33
    #define LED_LONG    GPIO_NUM_34
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    #define BUTTON_PIN  GPIO_NUM_0
    #define LED_SHORT   GPIO_NUM_4
    #define LED_LONG    GPIO_NUM_5
#else
    /* ESP32 DevKit (default) */
    #define BUTTON_PIN  GPIO_NUM_4
    #define LED_SHORT   GPIO_NUM_16
    #define LED_LONG    GPIO_NUM_17
#endif

/* ── Threshold durasi tekan ───────────────────────────────────────────── */
#define LONG_PRESS_MS   1000    /* ≥ 1000ms dianggap long press */

/* ── Debounce ─────────────────────────────────────────────────────────── */
#define DEBOUNCE_MS     50

/* ── Polling interval ─────────────────────────────────────────────────── */
#define POLL_INTERVAL_MS  10

#endif /* CONFIG_H */
