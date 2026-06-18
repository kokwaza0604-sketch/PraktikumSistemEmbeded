/* SysTick handler: keep HAL tick working and forward to FreeRTOS after start.
 * This ensures HAL_Delay (and other HAL timing) works before the scheduler
 * is started and that FreeRTOS receives ticks once running.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

/* Prototype for the FreeRTOS SysTick handler implemented in the portable layer. */
void xPortSysTickHandler( void );

void SysTick_Handler(void)
{
    /* Increment HAL tick for HAL_Delay and HAL timing functions. */
    HAL_IncTick();

    /* If the scheduler has started, forward the tick to FreeRTOS. */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}
