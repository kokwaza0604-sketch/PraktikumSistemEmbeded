/**
 * ============================================================
 *  STM32 SPI LOOPBACK TEST
 *  Modul 07 - SPI & Storage
 * ============================================================
 *  Deskripsi:
 *    Menguji komunikasi SPI dengan menghubungkan MOSI ke MISO
 *    (loopback). Mengirim data via SPI dan memverifikasi bahwa
 *    data yang diterima sama persis.
 *
 *  Wiring:
 *    - PA7 (MOSI) ---wire--- PA6 (MISO)
 *    - PC13: LED indikator
 *    - PA9/PA10: UART1 TX/RX (debug serial)
 *
 *  Test Patterns:
 *    1. Ascending bytes (0x00-0x0F)
 *    2. Alternating 0xAA/0x55
 *    3. All 0xFF
 *    4. All 0x00
 *    5. ASCII characters
 * ============================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "config.h"

/* ==================== Global Handles ==================== */
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

/* ==================== Buffers ==================== */
static uint8_t tx_buffer[SPI_BUFFER_SIZE];
static uint8_t rx_buffer[SPI_BUFFER_SIZE];

/* ==================== Test Statistics ==================== */
static uint32_t total_tests = 0;
static uint32_t passed_tests = 0;
static uint32_t failed_tests = 0;
static uint32_t total_bytes_sent = 0;
static uint32_t total_errors = 0;

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void UART1_Init(void);
static void SPI1_Init(void);
static void GPIO_Init(void);
static void print_hex_dump(const char *label, const uint8_t *data, uint16_t len);
static uint8_t spi_loopback_test(const char *test_name, const uint8_t *test_data, uint16_t len);
static void run_all_tests(void);

/* ==================== Printf Retarget ==================== */
int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 *  SystemClock_Config
 *  HSE 8MHz -> PLL x9 -> SYSCLK 72MHz
 *  AHB=72MHz, APB1=36MHz, APB2=72MHz
 * ============================================================ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* HSE Oscillator Configuration */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;  /* 8MHz x 9 = 72MHz */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        while (1);
    }

    /* System Clock Configuration */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;    /* HCLK = 72MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;     /* APB1 = 36MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;     /* APB2 = 72MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  UART1_Init
 *  PA9=TX, PA10=RX, 115200 baud, 8N1
 * ============================================================ */
static void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* UART1 TX - PA9 (Alternate Push-Pull) */
    GPIO_InitStruct.Pin = UART1_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART1_TX_PORT, &GPIO_InitStruct);

    /* UART1 RX - PA10 (Input Floating) */
    GPIO_InitStruct.Pin = UART1_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(UART1_RX_PORT, &GPIO_InitStruct);

    /* UART Configuration */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART1_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  SPI1_Init
 *  PA5=SCK, PA7=MOSI, PA6=MISO, PA4=NSS(software)
 *  Mode 0, 8-bit, MSB first, Full-duplex Master, 9MHz
 * ============================================================ */
static void SPI1_Init(void) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* SPI1 SCK - PA5 (Alternate Push-Pull) */
    GPIO_InitStruct.Pin = SPI1_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_SCK_PORT, &GPIO_InitStruct);

    /* SPI1 MOSI - PA7 (Alternate Push-Pull) */
    GPIO_InitStruct.Pin = SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_MOSI_PORT, &GPIO_InitStruct);

    /* SPI1 MISO - PA6 (Input Floating) */
    GPIO_InitStruct.Pin = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SPI1_MISO_PORT, &GPIO_InitStruct);

    /* SPI1 NSS - PA4 (Output Push-Pull, Software managed) */
    GPIO_InitStruct.Pin = SPI1_NSS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_NSS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    /* SPI1 Configuration */
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI1_MODE;
    hspi1.Init.Direction = SPI1_DIRECTION;
    hspi1.Init.DataSize = SPI1_DATA_SIZE;
    hspi1.Init.CLKPolarity = SPI1_CPOL;
    hspi1.Init.CLKPhase = SPI1_CPHA;
    hspi1.Init.NSS = SPI1_NSS_MODE;
    hspi1.Init.BaudRatePrescaler = SPI1_PRESCALER;
    hspi1.Init.FirstBit = SPI1_FIRST_BIT;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  GPIO_Init - LED PC13
 * ============================================================ */
static void GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* LED off (active LOW on Blue Pill) */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ============================================================
 *  print_hex_dump
 *  Print data in hexadecimal format
 * ============================================================ */
static void print_hex_dump(const char *label, const uint8_t *data, uint16_t len) {
    printf("  %s [%d bytes]: ", label, len);
    for (uint16_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0 && (i + 1) < len) {
            printf("\n                    ");
        }
    }
    printf("\n");
}

/* ============================================================
 *  spi_loopback_test
 *  Send tx_buffer via HAL_SPI_TransmitReceive, compare with rx_buffer
 *  Returns: 1=PASS, 0=FAIL
 * ============================================================ */
static uint8_t spi_loopback_test(const char *test_name, const uint8_t *test_data, uint16_t len) {
    HAL_StatusTypeDef status;
    uint32_t error_count = 0;

    /* Clear RX buffer */
    memset(rx_buffer, 0, SPI_BUFFER_SIZE);

    /* Copy test data to TX buffer */
    memcpy(tx_buffer, test_data, len);

    printf("\n--- Test: %s ---\n", test_name);

    /* Assert NSS (active LOW) */
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);

    /* SPI Transmit and Receive simultaneously */
    status = HAL_SPI_TransmitReceive(&hspi1, tx_buffer, rx_buffer, len, 1000);

    /* Deassert NSS */
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    if (status != HAL_OK) {
        printf("  SPI TransmitReceive ERROR! HAL Status: %d\n", (int)status);
        printf("  SPI Error Code: 0x%08lX\n", hspi1.ErrorCode);
        total_tests++;
        failed_tests++;
        return 0;
    }

    /* Print TX and RX hex dumps */
    print_hex_dump("TX", tx_buffer, len);
    print_hex_dump("RX", rx_buffer, len);

    /* Compare TX and RX */
    for (uint16_t i = 0; i < len; i++) {
        if (tx_buffer[i] != rx_buffer[i]) {
            error_count++;
            if (error_count <= 5) {
                printf("  MISMATCH at byte[%d]: TX=0x%02X, RX=0x%02X\n",
                       i, tx_buffer[i], rx_buffer[i]);
            }
        }
    }

    total_bytes_sent += len;
    total_errors += error_count;
    total_tests++;

    if (error_count == 0) {
        printf("  Result: PASS (0 errors in %d bytes)\n", len);
        passed_tests++;
        return 1;
    } else {
        printf("  Result: FAIL (%lu errors in %d bytes)\n", error_count, len);
        float ber = (float)error_count * 8.0f / ((float)len * 8.0f) * 100.0f;
        printf("  BER: %.4f%%\n", ber);
        failed_tests++;
        return 0;
    }
}

/* ============================================================
 *  run_all_tests
 *  Execute 5 test patterns
 * ============================================================ */
static void run_all_tests(void) {
    uint8_t test_data[TEST_DATA_LENGTH];
    uint8_t all_passed = 1;
    uint32_t start_tick = HAL_GetTick();

    printf("\n========================================\n");
    printf("  SPI Loopback Test Suite\n");
    printf("  SPI Clock: 9 MHz (72MHz / 8)\n");
    printf("  Mode: CPOL=0, CPHA=0 (Mode 0)\n");
    printf("  Data: 8-bit, MSB first\n");
    printf("========================================\n");

    /* Test 1: Ascending bytes (0x00 - 0x0F) */
    for (uint16_t i = 0; i < TEST_DATA_LENGTH; i++) {
        test_data[i] = (uint8_t)(i & 0xFF);
    }
    if (!spi_loopback_test("Ascending (0x00-0x0F)", test_data, TEST_DATA_LENGTH))
        all_passed = 0;

    HAL_Delay(100);

    /* Test 2: Alternating 0xAA / 0x55 */
    for (uint16_t i = 0; i < TEST_DATA_LENGTH; i++) {
        test_data[i] = (i % 2 == 0) ? 0xAA : 0x55;
    }
    if (!spi_loopback_test("Alternating 0xAA/0x55", test_data, TEST_DATA_LENGTH))
        all_passed = 0;

    HAL_Delay(100);

    /* Test 3: All 0xFF */
    memset(test_data, 0xFF, TEST_DATA_LENGTH);
    if (!spi_loopback_test("All 0xFF", test_data, TEST_DATA_LENGTH))
        all_passed = 0;

    HAL_Delay(100);

    /* Test 4: All 0x00 */
    memset(test_data, 0x00, TEST_DATA_LENGTH);
    if (!spi_loopback_test("All 0x00", test_data, TEST_DATA_LENGTH))
        all_passed = 0;

    HAL_Delay(100);

    /* Test 5: ASCII characters */
    const char *ascii_str = "STM32F103-SPI!!";
    uint16_t ascii_len = strlen(ascii_str);
    if (ascii_len > TEST_DATA_LENGTH) ascii_len = TEST_DATA_LENGTH;
    memcpy(test_data, ascii_str, ascii_len);
    /* Pad remaining bytes */
    for (uint16_t i = ascii_len; i < TEST_DATA_LENGTH; i++) {
        test_data[i] = 0x00;
    }
    if (!spi_loopback_test("ASCII Characters", test_data, TEST_DATA_LENGTH))
        all_passed = 0;

    /* Summary */
    uint32_t elapsed = HAL_GetTick() - start_tick;
    printf("\n========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("  Total Tests : %lu\n", total_tests);
    printf("  Passed      : %lu\n", passed_tests);
    printf("  Failed      : %lu\n", failed_tests);
    printf("  Total Bytes : %lu\n", total_bytes_sent);
    printf("  Total Errors: %lu\n", total_errors);
    if (total_bytes_sent > 0) {
        float overall_ber = (float)total_errors / (float)(total_bytes_sent * 8) * 100.0f;
        printf("  Overall BER : %.6f%%\n", overall_ber);
    }
    printf("  Time Elapsed: %lu ms\n", elapsed);

    if (all_passed) {
        printf("\n  >>> ALL TESTS PASSED <<<\n");
        /* Blink LED fast for success */
        for (int i = 0; i < 6; i++) {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            HAL_Delay(100);
        }
    } else {
        printf("\n  >>> SOME TESTS FAILED <<<\n");
        /* LED on for failure */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    }
    printf("========================================\n");
}

/* ============================================================
 *  Main Entry Point
 * ============================================================ */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    SPI1_Init();
    GPIO_Init();

    printf("\n\n");
    printf("============================================\n");
    printf("  STM32F103 SPI Loopback Test\n");
    printf("  Modul 07 - SPI & Storage\n");
    printf("============================================\n");
    printf("  SYSCLK : 72 MHz\n");
    printf("  SPI1   : 9 MHz (Prescaler /8)\n");
    printf("  Mode   : 0 (CPOL=0, CPHA=0)\n");
    printf("  Pins   : SCK=PA5, MOSI=PA7, MISO=PA6\n");
    printf("============================================\n");
    printf("\n>>> Hubungkan PA7 (MOSI) ke PA6 (MISO) <<<\n");
    printf(">>> untuk loopback test                  <<<\n\n");

    HAL_Delay(1000);

    while (1) {
        run_all_tests();

        printf("\nMenunggu %d detik untuk test berikutnya...\n", TEST_DELAY_MS / 1000);
        HAL_Delay(TEST_DELAY_MS);
    }
}
