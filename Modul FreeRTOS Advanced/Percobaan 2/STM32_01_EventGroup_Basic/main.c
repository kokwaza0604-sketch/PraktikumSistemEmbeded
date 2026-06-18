#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

EventGroupHandle_t xEventGroup;

void vTaskSetBits(void *pvParameters) {
  for (;;) {
    xEventGroupSetBits(xEventGroup, 0x01);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xEventGroupSetBits(xEventGroup, 0x02);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void vTaskWaitBits(void *pvParameters) {
  for (;;) {
    xEventGroupWaitBits(xEventGroup, 0x03, pdTRUE, pdTRUE, portMAX_DELAY);
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xEventGroup = xEventGroupCreate();
  configASSERT(xEventGroup != NULL);
  
  xTaskCreate(vTaskSetBits, "SetBits", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(vTaskWaitBits, "WaitBits", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
