#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================================
 * STM32_01_DAC_Voltage_Output
 * Modul-05-DAC-PWM
 * Platform: STM32 (Blue Pill F103 / Black Pill F401/F411)
 * ============================================================================ */

/* --- Board Detection & HAL Include --- */
#if defined(STM32F103xB)
    #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
    #include "stm32f4xx_hal.h"
#else
    #error "Unsupported STM32 target. Define STM32F103xB, STM32F401xC, or STM32F411xE"
#endif

/* --- LED Pin (On-board) --- */
#if defined(STM32F103xB)
    #define LED_PORT        GPIOC
    #define LED_PIN         GPIO_PIN_13
    #define LED_ACTIVE_LOW  1
    #define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#else
    #define LED_PORT        GPIOC
    #define LED_PIN         GPIO_PIN_13
    #define LED_ACTIVE_LOW  1
    #define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#endif


/* --- Timing --- */
#define BLINK_DELAY_MS  500

/* --- UART for printf --- */
#define PRINTF_UART     USART1

/* --- Function Prototypes --- */
void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
