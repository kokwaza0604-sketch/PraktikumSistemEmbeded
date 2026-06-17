#include "stm32f4xx_hal.h"

// Definisi Pin
#define LED1_PIN        GPIO_PIN_0
#define LED1_PORT       GPIOA
#define LED2_PIN        GPIO_PIN_1
#define LED2_PORT       GPIOA
#define BTN1_PIN        GPIO_PIN_0
#define BTN1_PORT       GPIOB
#define BTN2_PIN        GPIO_PIN_1
#define BTN2_PORT       GPIOB

UART_HandleTypeDef huart1;

// Prototipe Fungsi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void UART_SendString(char* str);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    UART_SendString("System Started - STM32 Control Ready\r\n");

    while (1) {
        // Monitoring Switch (Active Low karena Pull-up)
        GPIO_PinState btn1_state = HAL_GPIO_ReadPin(BTN1_PORT, BTN1_PIN);
        GPIO_PinState btn2_state = HAL_GPIO_ReadPin(BTN2_PORT, BTN2_PIN);

        // Control LED berdasarkan Switch
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, (btn1_state == GPIO_PIN_RESET) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, (btn2_state == GPIO_PIN_RESET) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        // Serial Monitoring
        if (btn1_state == GPIO_PIN_RESET) {
            UART_SendString("Button 1 Pressed\r\n");
            HAL_Delay(200); 
        }
        if (btn2_state == GPIO_PIN_RESET) {
            UART_SendString("Button 2 Pressed\r\n");
            HAL_Delay(200);
        }
    }
}

// Inisialisasi USART1 (PA9=TX, PA10=RX)
static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200; // Sesuai monitor_speed di platformio.ini
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Config LED (Output)
    GPIO_InitStruct.Pin = LED1_PIN | LED2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Config Button (Input Pull-up)
    GPIO_InitStruct.Pin = BTN1_PIN | BTN2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void UART_SendString(char* str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

void SystemClock_Config(void) {
    // Standard clock config for 72MHz (Bluepill)
}