/**
 * ============================================================================
 * PROGRAM 01: Task Create Basic - Pembuatan Task FreeRTOS Dasar
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan pembuatan task dasar menggunakan FreeRTOS
 *   pada mikrokontroler STM32F103C8 (Blue Pill). Dua task LED blink dibuat
 *   menggunakan xTaskCreate() dengan prioritas berbeda, ditambah satu task
 *   monitor untuk memantau status sistem.
 * 
 * Konsep yang Dipelajari:
 *   1. xTaskCreate()     - Membuat task baru secara dinamis
 *   2. vTaskDelay()      - Menunda eksekusi task (non-blocking)
 *   3. Task Priority     - Pengaruh prioritas terhadap penjadwalan
 *   4. Stack Management  - Pemantauan penggunaan stack
 *   5. Heap Monitoring   - Pemantauan sisa memori heap
 * 
 * Arsitektur Task:
 *   ┌─────────────┐  ┌─────────────┐  ┌──────────────┐
 *   │  Task LED1  │  │  Task LED2  │  │ Task Monitor │
 *   │  Prio: 2    │  │  Prio: 1    │  │  Prio: 3     │
 *   │  PC13 500ms │  │  PB0 200ms  │  │  Status 2s   │
 *   └─────────────┘  └─────────────┘  └──────────────┘
 *         │                │                  │
 *         └────────────────┼──────────────────┘
 *                          │
 *                   ┌──────┴──────┐
 *                   │  Scheduler  │
 *                   │  FreeRTOS   │
 *                   └─────────────┘
 * 
 * Hardware:
 *   - STM32F103C8 Blue Pill (72 MHz, 20KB SRAM)
 *   - LED Built-in: PC13 (active LOW)
 *   - LED Eksternal: PB0 (active HIGH)
 *   - USART1: PA9(TX)/PA10(RX) @ 115200 baud
 * 
 * Output Format (untuk Python parser):
 *   [DATA]TASK,<nama>,<prioritas>,<stack_free>,<toggle_count>,<tick>
 *   [DATA]HEAP,<free_bytes>,<min_free>,<tick>
 *   [DATA]SYSTEM,<task_count>,<uptime_sec>,<tick>
 * 
 * ============================================================================
 */

/* ========================== HEADER INCLUDES ============================== */
#include "stm32f1xx_hal.h"          /* STM32 HAL Library                    */
#include "FreeRTOS.h"               /* Kernel FreeRTOS                      */
#include "task.h"                   /* API Task Management                  */
#include "config.h"                 /* Konfigurasi pin dan parameter        */
#include <stdio.h>                  /* printf, sprintf                      */
#include <string.h>                 /* strlen, memset                       */

/* ======================== VARIABEL GLOBAL ================================ */

/**
 * Handle UART untuk komunikasi serial
 * Digunakan oleh semua task melalui printf (retarget _write)
 */
static UART_HandleTypeDef huart1;

/**
 * Handle task FreeRTOS
 * Disimpan untuk memantau status dan mengambil informasi task
 */
static TaskHandle_t xTask1Handle = NULL;    /* Handle Task LED1 (PC13)      */
static TaskHandle_t xTask2Handle = NULL;    /* Handle Task LED2 (PB0)       */
static TaskHandle_t xMonitorHandle = NULL;  /* Handle Task Monitor          */

/**
 * Counter toggle LED untuk setiap task
 * Volatile karena diakses dari task berbeda
 */
static volatile uint32_t ulTask1ToggleCount = 0;
static volatile uint32_t ulTask2ToggleCount = 0;

/**
 * Flag untuk menandakan scheduler sudah berjalan
 * Digunakan oleh SysTick_Handler untuk memanggil xPortSysTickHandler
 */
static volatile uint8_t ucSchedulerStarted = 0;

/**
 * Nilai heap minimum yang pernah tercatat
 * Berguna untuk mendeteksi kebocoran memori
 */
static volatile uint32_t ulMinFreeHeap = 0xFFFFFFFF;

/**
 * Buffer untuk formatted print output
 */
static char pcPrintBuffer[PRINT_BUFFER_SIZE];

/* ==================== DEKLARASI FUNGSI PROTOTYPE ========================= */

/* Inisialisasi Hardware */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);

/* Task Functions */
static void vTaskLED1(void *pvParameters);
static void vTaskLED2(void *pvParameters);
static void vTaskMonitor(void *pvParameters);

/* Utility Functions */
static void vPrintSystemInfo(void);
static void vPrintTaskInfo(TaskHandle_t xTask, const char *pcTaskName);
static void vPrintHeapInfo(void);
static void vPrintSeparator(char cChar, uint8_t ucLength);
static const char* pcTaskStateToString(eTaskState eState);

/* ========================= RETARGET PRINTF =============================== */

/**
 * Retarget fungsi _write() ke USART1
 * 
 * Fungsi ini dipanggil oleh printf/sprintf untuk mengirim karakter
 * ke output. Kita arahkan ke UART1 agar bisa dilihat di serial monitor.
 * 
 * @param file  File descriptor (tidak digunakan)
 * @param ptr   Pointer ke buffer data
 * @param len   Jumlah byte yang akan ditulis
 * @return      Jumlah byte yang berhasil ditulis
 */
int _write(int file, char *ptr, int len) {
    (void)file;  /* Parameter tidak digunakan, hindari warning */
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===================== INTERRUPT HANDLERS ================================ */

/**
 * SysTick Timer Handler
 * 
 * SysTick digunakan bersama oleh HAL (untuk HAL_Delay) dan FreeRTOS
 * (untuk tick scheduling). Handler ini memanggil kedua fungsi:
 * 1. HAL_IncTick()         - Increment counter HAL
 * 2. xPortSysTickHandler() - Tick handler FreeRTOS (hanya jika scheduler aktif)
 * 
 * PENTING: xPortSysTickHandler() TIDAK boleh dipanggil sebelum 
 * vTaskStartScheduler() karena akan menyebabkan crash.
 */
void SysTick_Handler(void) {
    HAL_IncTick();
    
    /* Hanya panggil FreeRTOS tick handler jika scheduler sudah berjalan */
    if (ucSchedulerStarted) {
        xPortSysTickHandler();
    }
}

/* =================== HOOK FUNCTIONS FreeRTOS ============================= */

/**
 * Hook: Malloc Failed
 * 
 * Dipanggil oleh kernel FreeRTOS ketika pvPortMalloc() gagal mengalokasi
 * memori. Biasanya terjadi karena heap penuh.
 * 
 * Aksi: Matikan semua LED, cetak error, dan loop selamanya.
 */
void vApplicationMallocFailedHook(void) {
    /* Matikan semua LED sebagai indikasi error */
    HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_EXT1_PORT, LED_EXT1_PIN, GPIO_PIN_RESET);
    
    printf("\r\n[ERROR] !! MALLOC GAGAL - Heap penuh !!\r\n");
    printf("[ERROR] Free Heap: %lu bytes\r\n", (unsigned long)xPortGetFreeHeapSize());
    
    /* Loop tanpa akhir - sistem harus di-reset */
    taskDISABLE_INTERRUPTS();
    for (;;) {
        /* Blink cepat LED built-in sebagai SOS */
        HAL_GPIO_TogglePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

/**
 * Hook: Stack Overflow
 * 
 * Dipanggil ketika FreeRTOS mendeteksi stack overflow pada suatu task.
 * configCHECK_FOR_STACK_OVERFLOW = 2 menggunakan metode "pattern checking"
 * yang memeriksa apakah pattern di akhir stack masih utuh.
 * 
 * @param xTask      Handle task yang mengalami overflow
 * @param pcTaskName Nama task yang mengalami overflow
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;  /* Hindari warning unused parameter */
    
    printf("\r\n[ERROR] !! STACK OVERFLOW pada task: %s !!\r\n", pcTaskName);
    
    taskDISABLE_INTERRUPTS();
    for (;;) {
        /* Blink pattern khusus: 2 kali cepat, pause */
        HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_RESET);
        for (volatile uint32_t i = 0; i < 100000; i++);
        HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET);
        for (volatile uint32_t i = 0; i < 100000; i++);
        HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_RESET);
        for (volatile uint32_t i = 0; i < 100000; i++);
        HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET);
        for (volatile uint32_t i = 0; i < 500000; i++);
    }
}

/**
 * Menyediakan memori statis untuk Idle Task
 * 
 * Ketika configSUPPORT_STATIC_ALLOCATION = 1, FreeRTOS memerlukan
 * callback ini untuk mendapatkan buffer memori bagi idle task.
 * Idle task berjalan ketika tidak ada task lain yang siap dieksekusi.
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
    /* Buffer statis - dialokasikan sekali, tidak pernah dibebaskan */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/**
 * Menyediakan memori statis untuk Timer Task
 * 
 * Timer task mengelola semua software timer FreeRTOS.
 * Dibutuhkan ketika configUSE_TIMERS = 1 dan 
 * configSUPPORT_STATIC_ALLOCATION = 1.
 */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* ==================== KONFIGURASI SYSTEM CLOCK =========================== */

/**
 * Konfigurasi System Clock: HSE 8MHz → PLL x9 → 72 MHz
 * 
 * Diagram Clock Tree:
 *   HSE (8 MHz) ──→ PLL (x9) ──→ SYSCLK (72 MHz)
 *                                    ├──→ AHB  (72 MHz) → HCLK
 *                                    ├──→ APB1 (/2 = 36 MHz)
 *                                    └──→ APB2 (72 MHz)
 * 
 * APB1 max = 36 MHz (untuk USART2, I2C, SPI2, Timer2-7)
 * APB2 max = 72 MHz (untuk USART1, SPI1, GPIO, ADC, Timer1)
 */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi oscillator HSE dan PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;           /* Nyalakan HSE 8MHz */
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;  /* Tidak dibagi      */
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;           /* Nyalakan PLL      */
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;    /* Sumber dari HSE   */
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;         /* Kalikan 9x = 72MHz*/

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* Jika gagal konfigurasi oscillator, loop error */
        for (;;);
    }

    /* Konfigurasi bus clock (AHB, APB1, APB2) */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK; /* SYSCLK dari PLL */
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;         /* AHB = 72 MHz    */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;           /* APB1 = 36 MHz   */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;           /* APB2 = 72 MHz   */

    /* Flash latency harus 2 wait states untuk 72 MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        for (;;);
    }
}

/* ======================== INISIALISASI GPIO ============================== */

/**
 * Inisialisasi GPIO untuk LED dan Tombol
 * 
 * Pin yang dikonfigurasi:
 *   - PC13: LED Built-in (Output Push-Pull, Active LOW)
 *   - PB0:  LED Eksternal 1 (Output Push-Pull, Active HIGH)
 *   - PB1:  LED Eksternal 2 (Output Push-Pull, Active HIGH)
 *   - PA0:  Tombol (Input dengan Pull-Up)
 * 
 * CATATAN: Pada Blue Pill, PC13 memiliki keterbatasan arus (3mA max)
 * karena terhubung melalui resistor ke LED on-board.
 */
static void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Aktifkan clock untuk semua port GPIO yang digunakan */
    LED_BUILTIN_CLK_EN();   /* GPIOC untuk LED built-in  */
    LED_EXT1_CLK_EN();      /* GPIOB untuk LED eksternal */
    BTN_CLK_EN();           /* GPIOA untuk tombol        */

    /* ---- Konfigurasi LED Built-in PC13 ---- */
    GPIO_InitStruct.Pin   = LED_BUILTIN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;     /* Push-Pull output    */
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;     /* Low speed cukup     */
    HAL_GPIO_Init(LED_BUILTIN_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET); /* OFF */

    /* ---- Konfigurasi LED Eksternal PB0 ---- */
    GPIO_InitStruct.Pin   = LED_EXT1_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_EXT1_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_EXT1_PORT, LED_EXT1_PIN, GPIO_PIN_RESET);  /* OFF */

    /* ---- Konfigurasi LED Eksternal PB1 ---- */
    GPIO_InitStruct.Pin   = LED_EXT2_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_EXT2_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_EXT2_PORT, LED_EXT2_PIN, GPIO_PIN_RESET);  /* OFF */

    /* ---- Konfigurasi Tombol PA0 ---- */
    GPIO_InitStruct.Pin  = BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;          /* Mode input          */
    GPIO_InitStruct.Pull = GPIO_PULLUP;              /* Internal pull-up    */
    HAL_GPIO_Init(BTN_PORT, &GPIO_InitStruct);
}

/* ======================== INISIALISASI UART ============================== */

/**
 * Inisialisasi USART1 untuk komunikasi serial
 * 
 * Konfigurasi:
 *   - Baudrate: 115200 bps
 *   - Data bits: 8
 *   - Stop bits: 1
 *   - Parity: None
 *   - Mode: TX dan RX
 *   - Pin: PA9 (TX), PA10 (RX)
 * 
 * USART1 terhubung ke APB2 (72 MHz), sehingga dapat mencapai
 * baudrate tinggi dengan error rendah.
 */
static void UART1_Init(void) {
    /* Aktifkan clock USART1 dan GPIO */
    UART_CLK_EN();
    UART_GPIO_CLK_EN();

    /* Konfigurasi pin TX (PA9) sebagai Alternate Function Push-Pull */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;          /* AF Push-Pull       */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;     /* High speed         */
    HAL_GPIO_Init(UART_TX_PORT, &GPIO_InitStruct);

    /* Konfigurasi pin RX (PA10) sebagai Input Floating */
    GPIO_InitStruct.Pin  = UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;         /* AF Input           */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(UART_RX_PORT, &GPIO_InitStruct);

    /* Konfigurasi parameter USART1 */
    huart1.Instance          = UART_INSTANCE;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        for (;;);
    }
}

/* ========================= UTILITY FUNCTIONS ============================= */

/**
 * Cetak garis pemisah dekoratif
 * @param cChar    Karakter yang digunakan (misalnya '=', '-', '*')
 * @param ucLength Panjang garis (jumlah karakter)
 */
static void vPrintSeparator(char cChar, uint8_t ucLength) {
    for (uint8_t i = 0; i < ucLength; i++) {
        printf("%c", cChar);
    }
    printf("\r\n");
}

/**
 * Konversi state task ke string yang bisa dibaca
 * 
 * State task di FreeRTOS:
 *   - eRunning:   Task sedang berjalan di CPU
 *   - eReady:     Task siap berjalan, menunggu giliran
 *   - eBlocked:   Task menunggu event (delay, semaphore, dll)
 *   - eSuspended: Task di-suspend secara manual
 *   - eDeleted:   Task sudah dihapus, menunggu cleanup
 * 
 * @param eState  State task dari eTaskGetState()
 * @return        String deskripsi state
 */
static const char* pcTaskStateToString(eTaskState eState) {
    switch (eState) {
        case eRunning:   return "RUNNING";
        case eReady:     return "READY";
        case eBlocked:   return "BLOCKED";
        case eSuspended: return "SUSPENDED";
        case eDeleted:   return "DELETED";
        default:         return "UNKNOWN";
    }
}

/**
 * Cetak informasi detail sebuah task
 * 
 * Informasi yang dicetak:
 *   - Nama task
 *   - Prioritas saat ini
 *   - State task (Running/Ready/Blocked/dll)
 *   - High Water Mark (sisa stack minimum yang pernah tersisa)
 * 
 * High Water Mark penting untuk:
 *   - Mendeteksi apakah stack size cukup
 *   - Jika nilainya mendekati 0, stack bisa overflow
 *   - Idealnya minimal 10-20% stack tersisa
 * 
 * @param xTask     Handle task yang akan dicetak infonya
 * @param pcTaskName Nama task untuk display
 */
static void vPrintTaskInfo(TaskHandle_t xTask, const char *pcTaskName) {
    if (xTask == NULL) return;

    UBaseType_t uxPriority  = uxTaskPriorityGet(xTask);
    eTaskState  eState      = eTaskGetState(xTask);
    UBaseType_t uxHighWater = uxTaskGetStackHighWaterMark(xTask);

    /* Cetak info yang bisa dibaca manusia */
    printf("  %-15s | Prio: %lu | State: %-10s | Stack Free: %lu words\r\n",
           pcTaskName,
           (unsigned long)uxPriority,
           pcTaskStateToString(eState),
           (unsigned long)uxHighWater);

    /* Cetak data format untuk Python parser */
    printf("[DATA]TASK,%s,%lu,%lu,%lu,%lu\r\n",
           pcTaskName,
           (unsigned long)uxPriority,
           (unsigned long)uxHighWater,
           (pcTaskName[5] == 'L' && pcTaskName[9] == '1') ? 
               (unsigned long)ulTask1ToggleCount : (unsigned long)ulTask2ToggleCount,
           (unsigned long)xTaskGetTickCount());
    
    /* Peringatan jika stack hampir habis */
    if (uxHighWater < 20) {
        printf("  [PERINGATAN] Stack %s hampir habis! (%lu words tersisa)\r\n",
               pcTaskName, (unsigned long)uxHighWater);
    }
}

/**
 * Cetak informasi penggunaan heap FreeRTOS
 * 
 * Heap FreeRTOS (10KB) digunakan untuk:
 *   - TCB (Task Control Block) setiap task
 *   - Stack setiap task
 *   - Queue, Semaphore, dll (jika ada)
 * 
 * Memantau heap penting untuk:
 *   - Memastikan masih ada ruang untuk alokasi baru
 *   - Mendeteksi memory leak
 */
static void vPrintHeapInfo(void) {
    uint32_t ulFreeHeap = (uint32_t)xPortGetFreeHeapSize();
    
    /* Update minimum heap yang pernah tercatat */
    if (ulFreeHeap < ulMinFreeHeap) {
        ulMinFreeHeap = ulFreeHeap;
    }

    uint32_t ulUsedHeap = configTOTAL_HEAP_SIZE - ulFreeHeap;
    uint32_t ulUsagePercent = (ulUsedHeap * 100) / configTOTAL_HEAP_SIZE;

    printf("\r\n  === Informasi Heap FreeRTOS ===\r\n");
    printf("  Total Heap   : %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    printf("  Digunakan    : %lu bytes (%lu%%)\r\n", 
           (unsigned long)ulUsedHeap, (unsigned long)ulUsagePercent);
    printf("  Tersisa      : %lu bytes\r\n", (unsigned long)ulFreeHeap);
    printf("  Minimum      : %lu bytes (pernah tercatat)\r\n", 
           (unsigned long)ulMinFreeHeap);

    /* Output format untuk Python parser */
    printf("[DATA]HEAP,%lu,%lu,%lu\r\n",
           (unsigned long)ulFreeHeap,
           (unsigned long)ulMinFreeHeap,
           (unsigned long)xTaskGetTickCount());
    
    /* Peringatan jika heap hampir habis */
    if (ulUsagePercent > 90) {
        printf("  [PERINGATAN] Heap usage > 90%%! Hati-hati alokasi baru.\r\n");
    }
}

/**
 * Cetak informasi sistem secara keseluruhan
 * 
 * Termasuk:
 *   - Jumlah task yang ada
 *   - Uptime sistem
 *   - Status setiap task
 *   - Penggunaan heap
 */
static void vPrintSystemInfo(void) {
    TickType_t xCurrentTick = xTaskGetTickCount();
    uint32_t ulUptimeSec = xCurrentTick / configTICK_RATE_HZ;
    UBaseType_t uxTaskCount = uxTaskGetNumberOfTasks();

    vPrintSeparator('=', 65);
    printf("  MONITOR SISTEM - FreeRTOS Task Basic\r\n");
    printf("  Uptime: %lu detik | Tick: %lu | Jumlah Task: %lu\r\n",
           (unsigned long)ulUptimeSec,
           (unsigned long)xCurrentTick,
           (unsigned long)uxTaskCount);
    vPrintSeparator('-', 65);

    /* Cetak info setiap task */
    printf("\r\n  --- Status Task ---\r\n");
    vPrintTaskInfo(xTask1Handle, TASK1_NAME);
    vPrintTaskInfo(xTask2Handle, TASK2_NAME);
    vPrintTaskInfo(xMonitorHandle, MONITOR_TASK_NAME);

    /* Cetak toggle count */
    printf("\r\n  --- Counter Toggle LED ---\r\n");
    printf("  Task LED1 (PC13): %lu toggle\r\n", (unsigned long)ulTask1ToggleCount);
    printf("  Task LED2 (PB0) : %lu toggle\r\n", (unsigned long)ulTask2ToggleCount);

    /* Cetak heap info */
    vPrintHeapInfo();

    /* Data sistem untuk Python parser */
    printf("[DATA]SYSTEM,%lu,%lu,%lu\r\n",
           (unsigned long)uxTaskCount,
           (unsigned long)ulUptimeSec,
           (unsigned long)xCurrentTick);

    vPrintSeparator('=', 65);
}

/* ========================== TASK FUNCTIONS =============================== */

/**
 * Task LED1: Blink LED Built-in PC13 setiap 500ms
 * 
 * Task ini mendemonstrasikan:
 *   1. Penggunaan vTaskDelay() untuk timing periodik
 *   2. Toggle GPIO menggunakan HAL
 *   3. Pelaporan status via UART
 * 
 * Prioritas: 2 (sedang)
 * Stack: 256 words (1024 bytes)
 * 
 * Catatan tentang vTaskDelay():
 *   - Menempatkan task ke state BLOCKED selama waktu tertentu
 *   - Selama blocked, CPU bisa menjalankan task lain
 *   - Waktu dihitung dari saat vTaskDelay dipanggil
 *   - Tidak cocok untuk timing presisi (lihat vTaskDelayUntil)
 * 
 * @param pvParameters  Parameter yang diteruskan saat xTaskCreate (NULL)
 */
static void vTaskLED1(void *pvParameters) {
    (void)pvParameters;  /* Tidak menggunakan parameter */

    /* Variabel lokal task - setiap task punya stack sendiri */
    TickType_t xLastPrintTick = 0;
    const TickType_t xPrintInterval = pdMS_TO_TICKS(2000); /* Print tiap 2 detik */

    printf("[%s] Task dimulai - LED PC13 blink %d ms, Prioritas %d\r\n",
           TASK1_NAME, TASK1_BLINK_PERIOD_MS, TASK1_PRIORITY);

    /* ---- Loop utama task (berjalan selamanya) ---- */
    for (;;) {
        /* Toggle LED built-in PC13 */
        LED_BUILTIN_TOGGLE();
        ulTask1ToggleCount++;

        /* Cetak status periodik (tidak setiap toggle, hemat bandwidth) */
        TickType_t xCurrentTick = xTaskGetTickCount();
        if ((xCurrentTick - xLastPrintTick) >= xPrintInterval) {
            xLastPrintTick = xCurrentTick;

            UBaseType_t uxHighWater = uxTaskGetStackHighWaterMark(NULL);
            printf("[%s] Toggle #%lu | Stack free: %lu words | Tick: %lu\r\n",
                   TASK1_NAME,
                   (unsigned long)ulTask1ToggleCount,
                   (unsigned long)uxHighWater,
                   (unsigned long)xCurrentTick);

            /* Data untuk Python parser */
            printf("[DATA]TASK,%s,%d,%lu,%lu,%lu\r\n",
                   TASK1_NAME,
                   TASK1_PRIORITY,
                   (unsigned long)uxHighWater,
                   (unsigned long)ulTask1ToggleCount,
                   (unsigned long)xCurrentTick);
        }

        /* Delay 500ms - task masuk state BLOCKED */
        vTaskDelay(pdMS_TO_TICKS(TASK1_BLINK_PERIOD_MS));
    }
}

/**
 * Task LED2: Blink LED Eksternal PB0 setiap 200ms
 * 
 * Task ini berjalan bersamaan (concurrent) dengan Task LED1.
 * Karena prioritasnya lebih rendah (1 < 2), task ini hanya
 * berjalan ketika Task LED1 sedang dalam state BLOCKED.
 * 
 * Dalam praktiknya, kedua task tampak berjalan "bersamaan"
 * karena switching terjadi sangat cepat (< 1ms).
 * 
 * Prioritas: 1 (rendah)
 * Stack: 256 words (1024 bytes)
 * 
 * @param pvParameters  Parameter yang diteruskan saat xTaskCreate (NULL)
 */
static void vTaskLED2(void *pvParameters) {
    (void)pvParameters;

    TickType_t xLastPrintTick = 0;
    const TickType_t xPrintInterval = pdMS_TO_TICKS(2000);

    printf("[%s] Task dimulai - LED PB0 blink %d ms, Prioritas %d\r\n",
           TASK2_NAME, TASK2_BLINK_PERIOD_MS, TASK2_PRIORITY);

    for (;;) {
        /* Toggle LED eksternal PB0 */
        LED_EXT1_TOGGLE();
        ulTask2ToggleCount++;

        /* Cetak status periodik */
        TickType_t xCurrentTick = xTaskGetTickCount();
        if ((xCurrentTick - xLastPrintTick) >= xPrintInterval) {
            xLastPrintTick = xCurrentTick;

            UBaseType_t uxHighWater = uxTaskGetStackHighWaterMark(NULL);
            printf("[%s] Toggle #%lu | Stack free: %lu words | Tick: %lu\r\n",
                   TASK2_NAME,
                   (unsigned long)ulTask2ToggleCount,
                   (unsigned long)uxHighWater,
                   (unsigned long)xCurrentTick);

            printf("[DATA]TASK,%s,%d,%lu,%lu,%lu\r\n",
                   TASK2_NAME,
                   TASK2_PRIORITY,
                   (unsigned long)uxHighWater,
                   (unsigned long)ulTask2ToggleCount,
                   (unsigned long)xCurrentTick);
        }

        /* Delay 200ms - task masuk state BLOCKED */
        vTaskDelay(pdMS_TO_TICKS(TASK2_BLINK_PERIOD_MS));
    }
}

/**
 * Task Monitor: Memantau dan mencetak status seluruh sistem
 * 
 * Task ini memiliki prioritas tertinggi (3) agar selalu bisa
 * mengambil alih CPU untuk mencetak laporan status.
 * 
 * Fungsi utama:
 *   1. Mencetak status semua task (nama, prioritas, state, stack)
 *   2. Mencetak penggunaan heap
 *   3. Mencetak jumlah toggle setiap LED
 *   4. Membaca status tombol (PA0)
 * 
 * Prioritas: 3 (tinggi)
 * Stack: 512 words (2048 bytes) - lebih besar karena banyak printf
 * 
 * @param pvParameters  Parameter yang diteruskan saat xTaskCreate (NULL)
 */
static void vTaskMonitor(void *pvParameters) {
    (void)pvParameters;

    uint32_t ulCycleCount = 0;

    printf("[%s] Task monitor dimulai - Period %d ms, Prioritas %d\r\n",
           MONITOR_TASK_NAME, MONITOR_PERIOD_MS, MONITOR_PRIORITY);

    for (;;) {
        ulCycleCount++;

        /* Cetak header cycle */
        printf("\r\n");
        printf(">>> Monitor Cycle #%lu <<<\r\n", (unsigned long)ulCycleCount);

        /* Cetak informasi sistem lengkap */
        vPrintSystemInfo();

        /* Baca status tombol */
        GPIO_PinState btnState = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);
        printf("  Tombol PA0: %s\r\n", 
               (btnState == GPIO_PIN_RESET) ? "DITEKAN" : "DILEPAS");

        /* Toggle LED PB1 sebagai indikator monitor aktif */
        HAL_GPIO_TogglePin(LED_EXT2_PORT, LED_EXT2_PIN);

        /* Gunakan vTaskList untuk daftar task lengkap (jika tersedia) */
        if (ulCycleCount % 5 == 0) {
            /* Setiap 5 cycle, cetak tabel task detail */
            printf("\r\n  === Tabel Task FreeRTOS (vTaskList) ===\r\n");
            printf("  %-16s %-8s %-6s %-8s %-6s\r\n",
                   "Nama", "State", "Prio", "Stack", "Num");
            vPrintSeparator('-', 55);

            /* 
             * vTaskList menghasilkan tabel ASCII dari semua task
             * Format: Nama\tState\tPriority\tStack\tNum
             * State: X=Running, R=Ready, B=Blocked, S=Suspended, D=Deleted
             */
            char pcTaskListBuffer[512];
            vTaskList(pcTaskListBuffer);
            printf("%s\r\n", pcTaskListBuffer);
        }

        /* Delay 2 detik sebelum cycle berikutnya */
        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}

/* ============================ MAIN FUNCTION ============================== */

/**
 * Fungsi Utama Program
 * 
 * Urutan inisialisasi:
 *   1. HAL_Init()          - Inisialisasi HAL, SysTick, Flash prefetch
 *   2. SystemClock_Config() - Konfigurasi clock 72 MHz
 *   3. GPIO_Init()          - Konfigurasi pin LED dan tombol
 *   4. UART1_Init()         - Konfigurasi USART1 115200 baud
 *   5. xTaskCreate()        - Buat semua task FreeRTOS
 *   6. vTaskStartScheduler() - Mulai scheduler (tidak pernah return)
 * 
 * PENTING: Setelah vTaskStartScheduler(), kontrol sepenuhnya
 * diberikan ke kernel FreeRTOS. Kode setelahnya hanya dieksekusi
 * jika scheduler gagal dimulai (biasanya karena heap tidak cukup).
 */
int main(void) {
    /* ---- Fase 1: Inisialisasi Hardware ---- */
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();

    /* ---- Fase 2: Banner Startup ---- */
    printf("\r\n\r\n");
    vPrintSeparator('*', 65);
    printf("*   STM32F103 FreeRTOS - Program 01: Task Create Basic        *\r\n");
    printf("*   Demonstrasi pembuatan task dasar dengan xTaskCreate()      *\r\n");
    vPrintSeparator('*', 65);
    printf("\r\n");

    /* Cetak informasi sistem */
    printf("  [INFO] System Clock  : %lu MHz\r\n", 
           (unsigned long)(HAL_RCC_GetSysClockFreq() / 1000000));
    printf("  [INFO] AHB Clock     : %lu MHz\r\n", 
           (unsigned long)(HAL_RCC_GetHCLKFreq() / 1000000));
    printf("  [INFO] APB1 Clock    : %lu MHz\r\n", 
           (unsigned long)(HAL_RCC_GetPCLK1Freq() / 1000000));
    printf("  [INFO] APB2 Clock    : %lu MHz\r\n", 
           (unsigned long)(HAL_RCC_GetPCLK2Freq() / 1000000));
    printf("  [INFO] FreeRTOS Heap : %u bytes\r\n", 
           (unsigned int)configTOTAL_HEAP_SIZE);
    printf("  [INFO] Tick Rate     : %u Hz\r\n", 
           (unsigned int)configTICK_RATE_HZ);
    printf("\r\n");

    /* Cetak heap sebelum pembuatan task */
    printf("  [HEAP] Sebelum pembuatan task:\r\n");
    printf("  [HEAP] Free: %lu bytes\r\n\r\n", 
           (unsigned long)xPortGetFreeHeapSize());

    /* ---- Fase 3: Pembuatan Task FreeRTOS ---- */
    printf("  Membuat task FreeRTOS...\r\n\r\n");

    BaseType_t xResult;

    /**
     * xTaskCreate() - Membuat task baru secara dinamis
     * 
     * Parameter:
     *   1. pvTaskCode   - Pointer ke fungsi task
     *   2. pcName       - Nama task (untuk debugging, max configMAX_TASK_NAME_LEN)
     *   3. usStackDepth - Ukuran stack dalam words (bukan bytes!)
     *   4. pvParameters - Parameter yang diteruskan ke fungsi task (void*)
     *   5. uxPriority   - Prioritas task (0 = terendah)
     *   6. pxCreatedTask- Pointer untuk menyimpan handle task
     * 
     * Return: pdPASS jika berhasil, errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY jika gagal
     */

    /* Buat Task LED1 - Blink PC13 */
    xResult = xTaskCreate(
        vTaskLED1,              /* Fungsi task                           */
        TASK1_NAME,             /* Nama task: "Task_LED1"                */
        TASK1_STACK_SIZE,       /* Stack: 256 words = 1024 bytes         */
        NULL,                   /* Tidak ada parameter                   */
        TASK1_PRIORITY,         /* Prioritas: 2                          */
        &xTask1Handle           /* Simpan handle untuk monitoring        */
    );

    if (xResult == pdPASS) {
        printf("  [OK] Task '%s' berhasil dibuat (Prio: %d, Stack: %d words)\r\n",
               TASK1_NAME, TASK1_PRIORITY, TASK1_STACK_SIZE);
    } else {
        printf("  [GAGAL] Task '%s' gagal dibuat! Error: %ld\r\n",
               TASK1_NAME, (long)xResult);
    }

    /* Buat Task LED2 - Blink PB0 */
    xResult = xTaskCreate(
        vTaskLED2,
        TASK2_NAME,
        TASK2_STACK_SIZE,
        NULL,
        TASK2_PRIORITY,
        &xTask2Handle
    );

    if (xResult == pdPASS) {
        printf("  [OK] Task '%s' berhasil dibuat (Prio: %d, Stack: %d words)\r\n",
               TASK2_NAME, TASK2_PRIORITY, TASK2_STACK_SIZE);
    } else {
        printf("  [GAGAL] Task '%s' gagal dibuat! Error: %ld\r\n",
               TASK2_NAME, (long)xResult);
    }

    /* Buat Task Monitor - Pemantau sistem */
    xResult = xTaskCreate(
        vTaskMonitor,
        MONITOR_TASK_NAME,
        MONITOR_STACK_SIZE,
        NULL,
        MONITOR_PRIORITY,
        &xMonitorHandle
    );

    if (xResult == pdPASS) {
        printf("  [OK] Task '%s' berhasil dibuat (Prio: %d, Stack: %d words)\r\n",
               MONITOR_TASK_NAME, MONITOR_PRIORITY, MONITOR_STACK_SIZE);
    } else {
        printf("  [GAGAL] Task '%s' gagal dibuat! Error: %ld\r\n",
               MONITOR_TASK_NAME, (long)xResult);
    }

    /* Cetak heap setelah pembuatan task */
    printf("\r\n  [HEAP] Setelah pembuatan task:\r\n");
    printf("  [HEAP] Free: %lu bytes\r\n", 
           (unsigned long)xPortGetFreeHeapSize());
    printf("  [HEAP] Digunakan untuk task: %lu bytes\r\n\r\n",
           (unsigned long)(configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize()));

    /* ---- Fase 4: Mulai Scheduler FreeRTOS ---- */
    printf("  Memulai FreeRTOS Scheduler...\r\n");
    printf("  ================================================\r\n\r\n");

    /* Set flag sebelum memulai scheduler */
    ucSchedulerStarted = 1;

    /**
     * vTaskStartScheduler() - Memulai kernel FreeRTOS
     * 
     * Setelah fungsi ini dipanggil:
     *   - Idle task dibuat secara otomatis (prioritas 0)
     *   - Timer task dibuat jika configUSE_TIMERS = 1
     *   - Task dengan prioritas tertinggi mulai berjalan
     *   - Fungsi ini TIDAK PERNAH return (dalam kondisi normal)
     * 
     * Jika return, berarti ada masalah fatal:
     *   - Tidak cukup heap untuk idle task
     *   - Tidak cukup heap untuk timer task
     */
    vTaskStartScheduler();

    /* ---- Kode ini hanya dieksekusi jika scheduler gagal ---- */
    printf("\r\n[FATAL] Scheduler gagal dimulai! Heap tidak cukup?\r\n");
    printf("[FATAL] Free Heap: %lu bytes\r\n", (unsigned long)xPortGetFreeHeapSize());

    /* Loop error - blink LED sangat cepat */
    for (;;) {
        HAL_GPIO_TogglePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN);
        HAL_Delay(50);
    }
}

/* ====================== HAL CALLBACK OVERRIDES =========================== */

/**
 * HAL MSP Init - Dipanggil oleh HAL_Init()
 * Melakukan inisialisasi level rendah yang diperlukan HAL
 */
void HAL_MspInit(void) {
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Disable JTAG, enable SWD (membebaskan PB3, PB4, PA15) */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
}
