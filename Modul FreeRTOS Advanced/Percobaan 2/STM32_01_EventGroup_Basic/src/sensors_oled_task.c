#include "ssd1306.h"
#include "max6675.h"
#include "mq.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>

void vSensorsTask(void *pvParameters)
{
    (void) pvParameters;

    /* Peripherals and display are initialized in main(). */

    float t_tc = 0.0f, mq_v = 0.0f;
    uint32_t mq_raw = 0;
    char line[32];
    for (;;) {
        int r = max6675_read(&t_tc);
        int m = mq_read(&mq_v, &mq_raw);

        ssd1306_clear();
        ssd1306_set_cursor(0,0);
        if (r == 0) {
            /* Format temperature without floating printf: fixed point with 2 decimals */
            int tc_fixed = (int)((t_tc * 100.0f) + (t_tc >= 0 ? 0.5f : -0.5f));
            int tc_whole = tc_fixed / 100;
            int tc_frac = abs(tc_fixed % 100);
            snprintf(line, sizeof(line), "TC: %d.%02dC", tc_whole, tc_frac);
        } else if (r == -2) {
            snprintf(line, sizeof(line), "TC: Open");
        } else {
            snprintf(line, sizeof(line), "TC: Err");
        }
        ssd1306_write_string(line);

        ssd1306_set_cursor(0,1);
        if (m == 0) {
            int mq_fixed = (int)((mq_v * 100.0f) + 0.5f);
            int mq_whole = mq_fixed / 100;
            int mq_frac = abs(mq_fixed % 100);
            snprintf(line, sizeof(line), "MQ: %d.%02dV", mq_whole, mq_frac);
        } else {
            snprintf(line, sizeof(line), "MQ: Err");
        }
        ssd1306_write_string(line);

        ssd1306_set_cursor(0,2);
        snprintf(line, sizeof(line), "MQ raw: %lu", (unsigned long)mq_raw);
        ssd1306_write_string(line);

        ssd1306_display();

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
