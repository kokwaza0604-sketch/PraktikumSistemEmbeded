/**
 * @file config.h
 * @brief Hardware configuration for STM32_05_Long_Short_Press
 *
 * Button on PB0, LED_SHORT on PA0, LED_LONG on PA1
 * Short press (<1s) toggles LED_SHORT
 * Long press  (>1s) toggles LED_LONG
 * Uses HAL_GetTick() for timing
 * Multi-board support: F103, F401, F411
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Include HAL header based on target MCU ---- */
#if defined(STM32F103xC)
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported STM32 target! Define STM32F103xC, STM32F401xC, or STM32F411xE"
#endif

/* ---- LED Pin Configuration ---- */
#define LED_SHORT_PORT    GPIOA
#define LED_SHORT_PIN     GPIO_PIN_0

#define LED_LONG_PORT     GPIOA
#define LED_LONG_PIN      GPIO_PIN_1

/* ---- Button Pin Configuration ---- */
#define BTN_PORT          GPIOB
#define BTN_PIN           GPIO_PIN_0
#define BTN_PRESSED       GPIO_PIN_RESET  /* Active-low with pull-up */

/* ---- Timing Thresholds ---- */
#define LONG_PRESS_MS     1000   /* Press > 1000ms = long press */
#define DEBOUNCE_DELAY_MS 50

#endif /* CONFIG_H */
