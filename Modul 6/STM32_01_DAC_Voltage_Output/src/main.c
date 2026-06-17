/**
 * ==========================================================
 *  Program     : DAC Voltage Output
 *  Modul       : 05 - DAC & PWM
 *  MCU         : STM32F103C8 (Blue Pill) / STM32F401CC / STM32F411CE
 *  Deskripsi   : Menghasilkan tegangan output melalui DAC (F4)
 *                atau PWM sebagai alternatif (F103 tidak punya DAC).
 *                Output bertahap dari 0V sampai 3.3V.
 *  Catatan     : F103 tidak memiliki peripheral DAC, sehingga
 *                menggunakan PWM pada PA0 sebagai pengganti.
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#endif

#include <stdio.h>
#include <string.h>

/* ===================== Handle Peripheral ===================== */
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;    /* PWM untuk F103 */

#ifdef STM32F4
DAC_HandleTypeDef hdac;     /* DAC hanya untuk F4 */
#endif

/* ===================== Retarget printf ===================== */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===================== SysTick Handler ===================== */
void SysTick_Handler(void) {
    HAL_IncTick();
}

/* ===================== Konfigurasi Clock ===================== */
void SystemClock_Config(void) {
#ifdef STM32F1
    /* Konfigurasi clock F103: HSE 8MHz, PLL x9 = 72MHz */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#elif defined(STM32F4)
    /* Konfigurasi clock F4: HSE 8MHz, PLL -> 84MHz */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 84;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

/* ===================== Inisialisasi UART1 ===================== */
void MX_USART1_UART_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#ifdef STM32F1
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /* PA9 = TX (Alternate Push-Pull) */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* PA10 = RX (Input Floating) */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#elif defined(STM32F4)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ===================== Inisialisasi PWM (F103 sebagai pengganti DAC) ===================== */
void MX_TIM2_PWM_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
#ifdef STM32F1
    /* PA0 = TIM2_CH1 (Alternate Push-Pull) */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    /* Frekuensi PWM = SystemCoreClock / ((Prescaler+1)*(Period+1)) */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 4095;  /* 12-bit resolusi seperti DAC */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

#ifdef STM32F4
/* ===================== Inisialisasi DAC (hanya F4) ===================== */
void MX_DAC_Init(void) {
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA4 = DAC_OUT1 (Analog) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);

    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}
#endif

/* ===================== Inisialisasi LED Indikator ===================== */
void MX_LED_Init(void) {
#ifdef STM32F1
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
#elif defined(STM32F4)
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
#endif
}

/* ===================== Program Utama ===================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_LED_Init();

#ifdef STM32F4
    MX_DAC_Init();
    printf("\r\n=== DAC Voltage Output (STM32F4) ===\r\n");
    printf("DAC Channel 1 pada PA4\r\n");
    printf("Output: 0V -> 3.3V bertahap\r\n\r\n");
#else
    MX_TIM2_PWM_Init();
    printf("\r\n=== PWM Voltage Output (STM32F103) ===\r\n");
    printf("CATATAN: F103 tidak punya DAC!\r\n");
    printf("Menggunakan PWM pada PA0 sebagai alternatif\r\n");
    printf("Tambahkan RC filter untuk mendapat tegangan analog\r\n\r\n");
#endif

    uint32_t step = 0;
    uint32_t dac_value = 0;
    float voltage = 0.0f;

    while (1) {
        /* Naikkan nilai DAC/PWM dari 0 sampai 4095 (12-bit) */
        for (dac_value = 0; dac_value <= 4095; dac_value += 256) {
            voltage = (dac_value / 4095.0f) * 3.3f;

#ifdef STM32F4
            /* Set nilai DAC */
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
            printf("[DAC] Step %2lu: Nilai=%4lu, Tegangan=%.2f V\r\n",
                   step, dac_value, voltage);
#else
            /* Set duty cycle PWM sebagai pengganti DAC */
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, dac_value);
            printf("[PWM] Step %2lu: Duty=%4lu/4095, Tegangan~%.2f V\r\n",
                   step, dac_value, voltage);
#endif

            /* Toggle LED indikator */
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            step++;

            HAL_Delay(500);
        }

        /* Set ke nilai maksimum */
#ifdef STM32F4
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 4095);
        printf("[DAC] MAKS: Nilai=4095, Tegangan=3.30 V\r\n");
#else
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 4095);
        printf("[PWM] MAKS: Duty=4095/4095, Tegangan~3.30 V\r\n");
#endif
        HAL_Delay(1000);

        /* Reset ke 0 */
#ifdef STM32F4
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
        printf("[DAC] RESET: Nilai=0, Tegangan=0.00 V\r\n\r\n");
#else
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        printf("[PWM] RESET: Duty=0/4095, Tegangan~0.00 V\r\n\r\n");
#endif
        step = 0;

        HAL_Delay(1000);
    }
}
