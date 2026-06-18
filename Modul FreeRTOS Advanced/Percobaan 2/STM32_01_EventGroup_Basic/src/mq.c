#include "mq.h"
#include "main.h"
#include "stm32f1xx_hal.h"

int mq_read(float *voltage, uint32_t *raw)
{
    if (HAL_ADC_Start(&hadc1) != HAL_OK) return -1;
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return -1;
    }
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    if (raw) *raw = val;
    if (voltage) *voltage = ((float)val) * 3.3f / 4095.0f;
    return 0;
}
