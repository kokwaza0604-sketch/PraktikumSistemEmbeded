/**
 * ==========================================================
 *  Modul 06 - STM32_01_I2C_Scanner
 *  Memindai bus I2C untuk menemukan perangkat yang terhubung
 * ==========================================================
 *  Deskripsi:
 *    Probe alamat 0x01 - 0x7F menggunakan HAL_I2C_IsDeviceReady().
 *    Menampilkan tabel perangkat yang ditemukan melalui UART.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    PA9 = TX, PA10 = RX (USART1)
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif

#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;

/* Redirect printf ke UART */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void SystemClock_Config(void) {
#ifdef STM32F1
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
#else
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8;
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV4;
    osc.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
#endif
}

void MX_USART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F1
    g.Pin = GPIO_PIN_9; g.Mode = GPIO_MODE_AF_PP; g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = GPIO_PIN_10; g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#else
    g.Pin = GPIO_PIN_9|GPIO_PIN_10; g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP; g.Speed = GPIO_SPEED_FREQ_HIGH; g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &g);
#endif
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart1);
}

void MX_I2C1_Init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F1
    g.Pin = GPIO_PIN_6|GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_OD;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &g);
#else
    g.Pin = GPIO_PIN_6|GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &g);
#endif
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/**
 * Memindai seluruh alamat I2C (0x01 - 0x7F)
 * Mengembalikan jumlah perangkat yang ditemukan
 */
int I2C_Scan(void) {
    int jumlah_ditemukan = 0;

    printf("\r\n========================================\r\n");
    printf("  I2C Bus Scanner - STM32\r\n");
    printf("========================================\r\n");
    printf("Memindai alamat I2C 0x01 - 0x7F...\r\n\r\n");

    /* Header tabel */
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");

    for (uint8_t baris = 0; baris < 8; baris++) {
        printf("%02X: ", baris << 4);

        for (uint8_t kolom = 0; kolom < 16; kolom++) {
            uint8_t alamat = (baris << 4) | kolom;

            /* Lewati alamat reserved (0x00 dan > 0x7F) */
            if (alamat < 0x01 || alamat > 0x7F) {
                printf("   ");
                continue;
            }

            /* Coba probe perangkat di alamat ini */
            HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(&hi2c1, (alamat << 1), 3, 10);

            if (status == HAL_OK) {
                printf("%02X ", alamat);
                jumlah_ditemukan++;
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }

    printf("\r\nTotal perangkat ditemukan: %d\r\n", jumlah_ditemukan);
    printf("========================================\r\n");

    return jumlah_ditemukan;
}

/**
 * Identifikasi perangkat I2C yang umum berdasarkan alamatnya
 */
void I2C_IdentifikasiPerangkat(uint8_t alamat) {
    printf("  0x%02X -> ", alamat);

    switch (alamat) {
        case 0x23: case 0x5C:
            printf("BH1750 (Sensor Cahaya)");
            break;
        case 0x27: case 0x3F:
            printf("PCF8574 (I/O Expander / LCD)");
            break;
        case 0x3C: case 0x3D:
            printf("SSD1306 (OLED Display)");
            break;
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
            printf("AT24Cxx (EEPROM)");
            break;
        case 0x68:
            printf("DS3231 (RTC) / MPU6050 (IMU)");
            break;
        case 0x76: case 0x77:
            printf("BMP280/BME280 (Sensor Tekanan/Suhu)");
            break;
        default:
            printf("Perangkat tidak dikenal");
            break;
    }
    printf("\r\n");
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n===================================\r\n");
    printf("  STM32 I2C Scanner\r\n");
    printf("  SCL=PB6, SDA=PB7\r\n");
    printf("===================================\r\n");

    uint32_t nomor_scan = 0;

    while (1) {
        nomor_scan++;
        printf("\r\n--- Scan #%lu ---\r\n", nomor_scan);

        /* Lakukan pemindaian */
        int jumlah = I2C_Scan();

        /* Identifikasi perangkat yang ditemukan */
        if (jumlah > 0) {
            printf("\r\nIdentifikasi perangkat:\r\n");
            for (uint8_t addr = 0x01; addr <= 0x7F; addr++) {
                if (HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 1, 5) == HAL_OK) {
                    I2C_IdentifikasiPerangkat(addr);
                }
            }
        }

        /* Tunggu 5 detik sebelum scan berikutnya */
        printf("\r\nScan berikutnya dalam 5 detik...\r\n");
        HAL_Delay(5000);
    }
}
