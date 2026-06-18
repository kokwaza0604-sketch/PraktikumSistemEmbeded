/* Minimal FreeRTOS port wrappers for linking
 * Provides pvPortMalloc/vPortFree (using libc malloc/free)
 * and the static allocation hooks required by the kernel.
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>

/* Provide pvPortMalloc and vPortFree so the kernel links when a
 * heap_x.c implementation isn't present in the project. This uses
 * the C library malloc/free (newlib) available in the toolchain.
 */
void *pvPortMalloc( size_t xSize )
{
    return malloc( xSize );
}

void vPortFree( void *pv )
{
    free( pv );
}

void vPortInitialiseBlocks( void )
{
}

size_t xPortGetFreeHeapSize( void )
{
    return 0;
}

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return 0;
}

/* Static allocation support (required when configSUPPORT_STATIC_ALLOCATION == 1)
 * Provide the application callback implementations to supply memory for the
 * Idle and Timer tasks.
 */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* Timer task statically-allocated memory */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize )
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
