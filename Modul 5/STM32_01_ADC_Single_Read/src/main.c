/**
 * ============================================================================
 * Program     : STM32_01_ADC_Single_Read
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Membaca nilai ADC secara single conversion dari potensiometer
 *               yang terhubung ke pin PA0 (ADC1 Channel 0).
 *               Menggunakan HAL_ADC_Start(), HAL_ADC_PollForConversion(),
 *               dan HAL_ADC_GetValue() untuk mendapatkan nilai digital 12-bit.
 * Koneksi     : PA0 -> Potentiometer (wiper)
 *               PA9 -> UART TX (untuk Serial Monitor)
 *               PA10 -> UART RX
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handle untuk peripheral */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;

/* Redirect printf ke UART */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/**
 * Konfigurasi System Clock
 * HSE 8MHz -> PLL x9 -> SYSCLK 72MHz
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi HSE dan PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Konfigurasi clock bus */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/**
 * Inisialisasi USART1 untuk komunikasi serial
 * PA9 = TX, PA10 = RX, Baudrate = 115200
 */
void MX_USART1_UART_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA9 sebagai UART TX (Alternate Function Push-Pull) */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 sebagai UART RX (Input) */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi UART */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

/**
 * Inisialisasi ADC1 Channel 0 (PA0)
 * Mode: Single Conversion, Software Trigger
 * Resolusi: 12-bit (0-4095)
 */
void MX_ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Konfigurasi PA0 sebagai input analog */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi ADC1 */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    /* Kalibrasi ADC (penting untuk STM32F1) */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/**
 * Konfigurasi channel ADC
 * Channel 0 (PA0), Rank 1, Sampling Time 239.5 cycles
 */
void ADC_Select_Channel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
 * Membaca nilai ADC secara single conversion
 * Mengembalikan nilai 12-bit (0-4095)
 */
uint32_t ADC_Read(void) {
    ADC_Select_Channel(ADC_CHANNEL_0);
    HAL_ADC_Start(&hadc1);                              /* Mulai konversi */
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);   /* Tunggu selesai */
    uint32_t value = HAL_ADC_GetValue(&hadc1);          /* Baca nilai */
    HAL_ADC_Stop(&hadc1);                               /* Stop ADC */
    return value;
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    printf("=== STM32 ADC Single Read ===\r\n");
    printf("Membaca potensiometer pada PA0\r\n");
    printf("Resolusi: 12-bit (0-4095)\r\n\r\n");

    uint32_t adc_raw = 0;
    uint32_t counter = 0;

    while (1) {
        /* Baca nilai ADC dari PA0 */
        adc_raw = ADC_Read();

        /* Tampilkan hasil pembacaan */
        printf("[%lu] ADC_RAW:%lu\r\n", counter, adc_raw);

        counter++;
        HAL_Delay(500);  /* Delay 500ms antar pembacaan */
    }
}
