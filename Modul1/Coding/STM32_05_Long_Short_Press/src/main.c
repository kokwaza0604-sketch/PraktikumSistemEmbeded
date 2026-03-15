/**
 * @file main.c
 * @brief STM32_05_Long_Short_Press - Long/Short Press - Deteksi Durasi Tekan
 * 
 * FUNGSI: Membedakan tekan panjang (>1s) vs tekan pendek (<1s)
 * Supported: STM32F103C8T6, STM32F401CCU6, STM32F411CEU6
 */



#include "config.h"

/* ---- Function Prototypes ---- */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void Error_Handler(void);

/* ---- printf stub (no UART needed) ---- */

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    uint8_t btn_was_pressed = 0;
    uint32_t press_start_tick = 0;
    uint8_t led_short_state = 0;
    uint8_t led_long_state = 0;

    while (1)
    {
        GPIO_PinState btn = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

        if (btn == BTN_PRESSED && !btn_was_pressed)
        {
            /* Button just pressed - debounce */
            HAL_Delay(DEBOUNCE_DELAY_MS);
            btn = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

            if (btn == BTN_PRESSED) {
                /* Confirmed press - record start time */
                btn_was_pressed = 1;
                press_start_tick = HAL_GetTick();
            }
        }
        else if (btn != BTN_PRESSED && btn_was_pressed)
        {
            /* Button just released - debounce */
            HAL_Delay(DEBOUNCE_DELAY_MS);
            btn = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

            if (btn != BTN_PRESSED) {
                /* Confirmed release - calculate duration */
                uint32_t duration = HAL_GetTick() - press_start_tick;
                btn_was_pressed = 0;

                if (duration >= LONG_PRESS_MS) {
                    /* Long press -> toggle LED_LONG */
                    led_long_state = !led_long_state;
                    HAL_GPIO_WritePin(LED_LONG_PORT, LED_LONG_PIN,
                        led_long_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
                } else {
                    /* Short press -> toggle LED_SHORT */
                    led_short_state = !led_short_state;
                    HAL_GPIO_WritePin(LED_SHORT_PORT, LED_SHORT_PIN,
                        led_short_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
                }
            }
        }
    }
}

/* ============================================================
 *  GPIO Initialization
 * ============================================================ */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO Clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* ---- Configure LED_SHORT (PA0) as push-pull output ---- */
    HAL_GPIO_WritePin(LED_SHORT_PORT, LED_SHORT_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin   = LED_SHORT_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_SHORT_PORT, &GPIO_InitStruct);

    /* ---- Configure LED_LONG (PA1) as push-pull output ---- */
    HAL_GPIO_WritePin(LED_LONG_PORT, LED_LONG_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin   = LED_LONG_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_LONG_PORT, &GPIO_InitStruct);

    /* ---- Configure Button (PB0) as input with pull-up ---- */
    GPIO_InitStruct.Pin   = BTN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BTN_PORT, &GPIO_InitStruct);
}

/* ============================================================
 *  System Clock Configuration
 * ============================================================ */
#ifdef STM32F103xC
/* F103: 8MHz HSE -> PLL x9 -> 72MHz SYSCLK */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

#elif defined(STM32F401xC) || defined(STM32F411xE)
/* F4xx: 25MHz HSE -> PLL -> 84MHz SYSCLK */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 168;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}
#endif

/* ============================================================
 *  Error Handler
 * ============================================================ */
void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}
