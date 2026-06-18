#ifndef CONFIG_H
#define CONFIG_H
#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#else
#error "Unsupported STM32 target"
#endif
#define APP_NAME "STM32_01_EventGroup_Basic"
#define PROJECT_ID 1
#define USART_BAUDRATE 115200
#define I2C_TIMEOUT_MS 100
#define I2C_SPEED_HZ 100000
void Error_Handler(void);
#endif
