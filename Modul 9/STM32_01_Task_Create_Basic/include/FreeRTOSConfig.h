/**
 * ============================================================================
 * FreeRTOSConfig.h - Konfigurasi Kernel FreeRTOS
 * Target: STM32F103C8 (Cortex-M3) @ 72MHz
 * 
 * Konfigurasi untuk program dasar pembuatan task FreeRTOS
 * ============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ======================== Konfigurasi Dasar ============================== */
/* Preemptive scheduling: task prioritas tinggi akan mengambil alih CPU */
#define configUSE_PREEMPTION                    1

/* Frekuensi clock CPU dalam Hz (72 MHz dari PLL) */
#define configCPU_CLOCK_HZ                      72000000

/* Frekuensi tick FreeRTOS: 1000 Hz = 1ms per tick */
#define configTICK_RATE_HZ                      1000

/* Jumlah level prioritas task (0 = terendah, 6 = tertinggi) */
#define configMAX_PRIORITIES                    7

/* Ukuran stack minimum dalam words (128 words = 512 bytes) */
#define configMINIMAL_STACK_SIZE                128

/* Total heap FreeRTOS: 10KB dari 20KB SRAM STM32F103C8 */
#define configTOTAL_HEAP_SIZE                   10240

/* Panjang maksimum nama task */
#define configMAX_TASK_NAME_LEN                 16

/* Gunakan tipe data 16-bit untuk tick counter */
#define configUSE_16_BIT_TICKS                  0

/* Idle task tidak yield ke task lain dengan prioritas sama */
#define configIDLE_SHOULD_YIELD                 1

/* ====================== Fitur Sinkronisasi =============================== */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1

/* ======================= Alokasi Memori ================================== */
/* Mendukung alokasi statik (tanpa malloc) */
#define configSUPPORT_STATIC_ALLOCATION         1

/* Mendukung alokasi dinamis (dengan pvPortMalloc) */
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* ======================== Hook Functions ================================= */
/* Hook dipanggil saat idle task berjalan */
#define configUSE_IDLE_HOOK                     0

/* Hook dipanggil setiap tick interrupt */
#define configUSE_TICK_HOOK                     0

/* Deteksi stack overflow: metode 2 (pattern checking) */
#define configCHECK_FOR_STACK_OVERFLOW          2

/* Hook dipanggil jika pvPortMalloc gagal */
#define configUSE_MALLOC_FAILED_HOOK            1

/* ======================== Software Timer ================================= */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ====================== Trace & Statistics =============================== */
/* Aktifkan fasilitas trace untuk monitoring task */
#define configUSE_TRACE_FACILITY                1

/* Aktifkan fungsi format statistik (vTaskList, vTaskGetRunTimeStats) */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* =================== Konfigurasi Interrupt NVIC ========================== */
/* STM32F103 menggunakan 4 bit prioritas */
#define configPRIO_BITS                         4

/* Prioritas interrupt terendah (15 untuk 4-bit) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15

/* Prioritas tertinggi yang boleh memanggil API FreeRTOS dari ISR */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

/* Konversi ke format register NVIC (shift ke 4 bit teratas) */
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ========================= Assert Macro ================================== */
/* Jika kondisi gagal, matikan interrupt dan loop selamanya (untuk debugging) */
#define configASSERT(x) if ((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* ==================== INCLUDE Function APIs ============================== */
/* Mengaktifkan API fungsi FreeRTOS yang dibutuhkan */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_eTaskGetState                   1

/* ============== Pemetaan Handler ke Port FreeRTOS ======================== */
/* Handler interrupt Cortex-M3 yang digunakan FreeRTOS */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
/* SysTick_Handler didefinisikan manual di main.c */

#endif /* FREERTOS_CONFIG_H */
