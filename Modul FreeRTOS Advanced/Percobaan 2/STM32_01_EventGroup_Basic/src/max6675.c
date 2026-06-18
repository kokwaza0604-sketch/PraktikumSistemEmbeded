#include "max6675.h"
#include "main.h"
#include "stm32f1xx_hal.h"

/* MAX6675 on SPI1, CS on PA4 */
int max6675_read(float *temperature_celsius)
{
    uint8_t tx[2] = {0x00, 0x00};
    uint8_t rx[2] = {0,0};

    /* Select chip (CS low) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(1);

    if (HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 100) != HAL_OK) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        return -1;
    }

    /* Deselect chip */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    uint16_t raw = ((uint16_t)rx[0] << 8) | rx[1];
    if (raw & 0x4) {
        /* No thermocouple or open */
        return -2;
    }

    uint16_t value = (raw >> 3) & 0x0FFF; /* 12-bit */
    *temperature_celsius = value * 0.25f;
    return 0;
}
