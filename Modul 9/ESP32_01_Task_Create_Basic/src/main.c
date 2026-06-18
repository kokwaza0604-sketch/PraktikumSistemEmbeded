/* ============================================================
 * ESP32_01_Task_Create_Basic - Pembuatan Task FreeRTOS Dasar
 * ============================================================
 * Program ini mendemonstrasikan pembuatan task FreeRTOS pada ESP32
 * menggunakan xTaskCreate(). Dua task dibuat untuk mengedipkan LED
 * pada frekuensi berbeda, menunjukkan multitasking konkuren pada
 * prosesor dual-core ESP32.
 *
 * Konsep yang dipelajari:
 * 1. xTaskCreate() - membuat task baru di heap
 * 2. Task handle (TaskHandle_t) - referensi ke task
 * 3. Prioritas task - menentukan urutan eksekusi
 * 4. Stack size - memori yang dialokasikan untuk task
 * 5. Task info query - mendapatkan informasi runtime task
 * 6. Dual-core ESP32 - task bisa berjalan di core berbeda
 *
 * Hardware:
 * - ESP32 DOIT DevKit V1
 * - LED1 pada GPIO2 (built-in)
 * - LED2 pada GPIO4 (external)
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "config.h"

/* Tag untuk ESP_LOG */
static const char *TAG = "TASK_BASIC";

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 * Task handle digunakan untuk referensi task dari luar.
 * Berguna untuk suspend, resume, delete, atau query info task.
 * ============================================================ */
static TaskHandle_t xTask1Handle = NULL;    // Handle untuk task LED1
static TaskHandle_t xTask2Handle = NULL;    // Handle untuk task LED2
static TaskHandle_t xMonitorHandle = NULL;  // Handle untuk monitor task

/* Counter untuk tracking aktivitas task */
static volatile uint32_t task1_counter = 0;
static volatile uint32_t task2_counter = 0;
static volatile uint32_t monitor_counter = 0;

/* Timestamp awal program */
static int64_t start_time_us = 0;

/* ============================================================
 * FUNGSI HELPER
 * ============================================================ */

/**
 * @brief Mendapatkan waktu elapsed dalam milidetik sejak program mulai
 * @return Waktu dalam milidetik
 *
 * Menggunakan esp_timer_get_time() yang mengembalikan mikrodetik
 * dengan resolusi tinggi (64-bit timer hardware ESP32).
 */
static uint32_t get_elapsed_ms(void)
{
    return (uint32_t)((esp_timer_get_time() - start_time_us) / 1000);
}

/**
 * @brief Mendapatkan string nama state task
 * @param state eTaskState enum dari FreeRTOS
 * @return String deskripsi state
 *
 * FreeRTOS task memiliki beberapa state:
 * - eRunning: sedang dieksekusi CPU
 * - eReady: siap dieksekusi, menunggu giliran
 * - eBlocked: menunggu event (delay, semaphore, dll)
 * - eSuspended: di-suspend secara manual
 * - eDeleted: sudah dihapus tapi belum dibersihkan idle task
 */
static const char* get_task_state_string(eTaskState state)
{
    switch (state) {
        case eRunning:   return "RUNNING";
        case eReady:     return "READY";
        case eBlocked:   return "BLOCKED";
        case eSuspended: return "SUSPENDED";
        case eDeleted:   return "DELETED";
        default:         return "UNKNOWN";
    }
}

/**
 * @brief Inisialisasi GPIO untuk LED dan button
 *
 * Pada ESP32, konfigurasi GPIO menggunakan gpio_config_t struct
 * yang lebih fleksibel dibanding Arduino. Bisa set multiple pin
 * sekaligus menggunakan bit mask.
 */
static void init_gpio(void)
{
    /* Konfigurasi LED1 dan LED2 sebagai output */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_GPIO) | (1ULL << LED2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    /* Konfigurasi tombol BOOT sebagai input dengan pull-up */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_conf);

    /* Pastikan LED mati saat start */
    gpio_set_level(LED1_GPIO, 0);
    gpio_set_level(LED2_GPIO, 0);

    ESP_LOGI(TAG, "GPIO diinisialisasi: LED1=GPIO%d, LED2=GPIO%d, BTN=GPIO%d",
             LED1_GPIO, LED2_GPIO, BUTTON_GPIO);
}

/**
 * @brief Print informasi detail tentang sebuah task
 * @param handle Task handle yang akan di-query
 * @param task_name Nama task untuk display
 *
 * Menggunakan berbagai API FreeRTOS untuk query task info:
 * - pcTaskGetName(): nama task (max 16 char di ESP-IDF)
 * - uxTaskPriorityGet(): prioritas saat ini
 * - uxTaskGetStackHighWaterMark(): sisa stack minimum
 * - eTaskGetState(): state task saat ini
 * - xTaskGetAffinity(): core assignment (ESP32 SMP specific)
 */
static void print_task_info(TaskHandle_t handle, const char *task_name)
{
    if (handle == NULL) {
        ESP_LOGW(TAG, "Task handle NULL untuk %s", task_name);
        return;
    }

    /* Query informasi task */
    const char *name = pcTaskGetName(handle);
    UBaseType_t priority = uxTaskPriorityGet(handle);
    UBaseType_t stack_hwm = uxTaskGetStackHighWaterMark(handle);
    eTaskState state = eTaskGetState(handle);

    /*
     * xTaskGetAffinity() adalah API spesifik ESP32 SMP FreeRTOS.
     * Mengembalikan core yang ditugaskan:
     * - 0: Core 0 (Protocol CPU - WiFi/BT)
     * - 1: Core 1 (Application CPU)
     * - tskNO_AFFINITY: bisa berjalan di core manapun
     *
     * Pada ESP32, xTaskCreate() default ke tskNO_AFFINITY.
     * Gunakan xTaskCreatePinnedToCore() untuk pin ke core tertentu.
     */
    BaseType_t core_id = xTaskGetAffinity(handle);

    ESP_LOGI(TAG, "Task Info [%s]: prio=%d, stack_hwm=%d bytes, state=%s, core=%s",
             name, priority, stack_hwm * sizeof(StackType_t),
             get_task_state_string(state),
             (core_id == tskNO_AFFINITY) ? "ANY" :
             (core_id == 0) ? "0" : "1");

    /* Print data terformat untuk Python parser */
    printf("[DATA] TASK_INFO,%s,%lu,%d,%lu,%s,%d\n",
           name,
           (unsigned long)get_elapsed_ms(),
           (int)priority,
           (unsigned long)(stack_hwm * sizeof(StackType_t)),
           get_task_state_string(state),
           (int)core_id);
}

/* ============================================================
 * TASK FUNCTIONS
 * ============================================================
 *
 * Setiap task FreeRTOS adalah fungsi dengan signature:
 *   void task_function(void *pvParameters)
 *
 * Parameter pvParameters memungkinkan passing data ke task
 * saat pembuatan. Task harus berisi infinite loop atau
 * menghapus dirinya sendiri dengan vTaskDelete(NULL).
 *
 * PENTING: Task tidak boleh return! Jika task selesai tanpa
 * delete, behavior undefined (biasanya crash).
 * ============================================================ */

/**
 * @brief Task 1 - Mengedipkan LED1 setiap 500ms
 * @param pvParameters Parameter dari xTaskCreate (tidak digunakan)
 *
 * Task ini berjalan dengan prioritas tinggi (3).
 * Menggunakan vTaskDelay() untuk timing - ini adalah
 * cooperative multitasking dimana task secara sukarela
 * menyerahkan CPU ke scheduler.
 *
 * Saat vTaskDelay() dipanggil, task masuk state BLOCKED
 * dan scheduler menjalankan task lain yang READY.
 */
static void task1_led_blink(void *pvParameters)
{
    /* State LED untuk toggle */
    uint8_t led_state = 0;

    /* Task number dari parameter (demonstrasi passing data) */
    int task_num = (int)(intptr_t)pvParameters;

    ESP_LOGI(TAG, "Task1 dimulai! Task number=%d, Priority=%d",
             task_num, (int)uxTaskPriorityGet(NULL));

    /*
     * Infinite loop - setiap task FreeRTOS HARUS memiliki ini
     * atau menghapus dirinya sendiri. Loop ini akan berjalan
     * selamanya, di-interleave dengan task lain oleh scheduler.
     */
    while (1) {
        /* Toggle LED1 */
        led_state = !led_state;
        gpio_set_level(LED1_GPIO, led_state);

        /* Increment counter */
        task1_counter++;

        /* Print data periodik untuk monitoring */
        if (task1_counter % DATA_PRINT_INTERVAL == 0) {
            printf("[DATA] TASK1,%lu,%d,%lu,%d\n",
                   (unsigned long)get_elapsed_ms(),
                   led_state,
                   (unsigned long)task1_counter,
                   (int)xPortGetCoreID());  // Core mana yang menjalankan

            ESP_LOGD(TAG, "Task1: LED1=%d, count=%lu, core=%d",
                     led_state, (unsigned long)task1_counter,
                     (int)xPortGetCoreID());
        }

        /*
         * vTaskDelay() - Delay relatif
         * Parameter: jumlah tick. pdMS_TO_TICKS() konversi ms ke tick.
         *
         * Selama delay, task masuk BLOCKED state dan TIDAK
         * menggunakan CPU sama sekali. Ini berbeda dengan
         * busy-wait loop yang membuang CPU cycles.
         *
         * Default tick rate ESP-IDF = 100 Hz (10ms per tick).
         * pdMS_TO_TICKS(500) = 50 ticks
         */
        vTaskDelay(pdMS_TO_TICKS(LED1_BLINK_MS));
    }

    /* Code ini tidak akan pernah tercapai karena infinite loop di atas.
     * Tapi jika task perlu dihentikan, gunakan vTaskDelete(NULL). */
    vTaskDelete(NULL);
}

/**
 * @brief Task 2 - Mengedipkan LED2 setiap 200ms
 * @param pvParameters Parameter dari xTaskCreate (tidak digunakan)
 *
 * Task ini berjalan dengan prioritas lebih rendah (2) dari Task1.
 * Namun karena keduanya menggunakan vTaskDelay(), keduanya
 * tetap bisa berjalan "bersamaan" (time-sliced atau parallel
 * pada dual-core ESP32).
 *
 * Pada ESP32 dual-core:
 * - Core 0: biasanya menjalankan WiFi/BT protocol stack
 * - Core 1: application tasks
 * - Dengan tskNO_AFFINITY, scheduler bebas memilih core
 */
static void task2_led_blink(void *pvParameters)
{
    uint8_t led_state = 0;
    int task_num = (int)(intptr_t)pvParameters;

    ESP_LOGI(TAG, "Task2 dimulai! Task number=%d, Priority=%d",
             task_num, (int)uxTaskPriorityGet(NULL));

    while (1) {
        led_state = !led_state;
        gpio_set_level(LED2_GPIO, led_state);
        task2_counter++;

        if (task2_counter % (DATA_PRINT_INTERVAL * 2) == 0) {
            printf("[DATA] TASK2,%lu,%d,%lu,%d\n",
                   (unsigned long)get_elapsed_ms(),
                   led_state,
                   (unsigned long)task2_counter,
                   (int)xPortGetCoreID());
        }

        /* LED2 berkedip lebih cepat (200ms vs 500ms Task1) */
        vTaskDelay(pdMS_TO_TICKS(LED2_BLINK_MS));
    }

    vTaskDelete(NULL);
}

/**
 * @brief Monitor Task - Melaporkan status semua task
 * @param pvParameters Tidak digunakan
 *
 * Task ini berjalan dengan prioritas terendah dan secara
 * periodik melaporkan:
 * - Informasi setiap task (prioritas, stack, state, core)
 * - Free heap memory
 * - Uptime
 *
 * Ini adalah pola umum dalam embedded systems: task khusus
 * untuk monitoring dan diagnostik sistem.
 */
static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor task dimulai!");

    /* Tunggu sebentar agar task lain sempat start */
    vTaskDelay(pdMS_TO_TICKS(500));

    while (1) {
        monitor_counter++;

        ESP_LOGI(TAG, "===== SYSTEM MONITOR (Report #%lu) =====",
                 (unsigned long)monitor_counter);

        /* Print info setiap task */
        print_task_info(xTask1Handle, "LED1_Blinker");
        print_task_info(xTask2Handle, "LED2_Blinker");
        print_task_info(xMonitorHandle, "Monitor");

        /*
         * esp_get_free_heap_size() - mendapatkan free heap memory
         * Penting untuk monitoring memory leak pada embedded system.
         *
         * esp_get_minimum_free_heap_size() - minimum free heap yang
         * pernah tercapai sejak boot (water mark).
         */
        uint32_t free_heap = esp_get_free_heap_size();
        uint32_t min_free_heap = esp_get_minimum_free_heap_size();

        ESP_LOGI(TAG, "Heap: free=%lu bytes, min_free=%lu bytes",
                 (unsigned long)free_heap, (unsigned long)min_free_heap);

        printf("[DATA] HEAP,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)free_heap,
               (unsigned long)min_free_heap);

        /* Print jumlah task yang sedang berjalan */
        UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
        printf("[DATA] SYSTEM,%lu,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)num_tasks,
               (unsigned long)task1_counter,
               (unsigned long)task2_counter);

        ESP_LOGI(TAG, "Total tasks: %d, T1_count=%lu, T2_count=%lu",
                 (int)num_tasks,
                 (unsigned long)task1_counter,
                 (unsigned long)task2_counter);

        /*
         * uxTaskGetSystemState() - mendapatkan snapshot dari semua tasks
         * Ini API yang powerful untuk system-wide monitoring.
         */
        TaskStatus_t *task_status_array;
        UBaseType_t array_size = uxTaskGetNumberOfTasks();
        uint32_t total_runtime;

        task_status_array = pvPortMalloc(array_size * sizeof(TaskStatus_t));
        if (task_status_array != NULL) {
            array_size = uxTaskGetSystemState(task_status_array, array_size,
                                              &total_runtime);

            ESP_LOGI(TAG, "--- All Tasks Snapshot ---");
            for (UBaseType_t i = 0; i < array_size; i++) {
                ESP_LOGD(TAG, "  [%d] %s: prio=%d, state=%s, hwm=%d",
                         (int)task_status_array[i].xTaskNumber,
                         task_status_array[i].pcTaskName,
                         (int)task_status_array[i].uxCurrentPriority,
                         get_task_state_string(task_status_array[i].eCurrentState),
                         (int)task_status_array[i].usStackHighWaterMark);
            }
            vPortFree(task_status_array);
        }

        ESP_LOGI(TAG, "=========================================");

        /* Delay monitor - period lebih panjang */
        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }

    vTaskDelete(NULL);
}

/* ============================================================
 * APP_MAIN - Entry Point
 * ============================================================
 *
 * app_main() adalah entry point utama ESP-IDF application.
 * Dipanggil oleh FreeRTOS setelah sistem boot.
 *
 * PENTING tentang ESP32 SMP FreeRTOS:
 * - app_main() berjalan di "main task" pada Core 0
 * - Main task memiliki prioritas 1 dan stack 8KB default
 * - Setelah app_main() return, main task dihapus otomatis
 * - Kita perlu membuat task sendiri sebelum return
 *
 * xTaskCreate() parameters:
 * 1. pvTaskCode: pointer ke fungsi task
 * 2. pcName: nama task (max configMAX_TASK_NAME_LEN)
 * 3. usStackDepth: ukuran stack dalam bytes (ESP-IDF) atau words
 * 4. pvParameters: parameter yang di-pass ke task
 * 5. uxPriority: prioritas task (0 = terendah)
 * 6. pxCreatedTask: pointer ke task handle (output)
 *
 * Return: pdPASS jika sukses, errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY jika gagal
 * ============================================================ */
void app_main(void)
{
    /* Catat waktu mulai untuk elapsed time tracking */
    start_time_us = esp_timer_get_time();

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 FreeRTOS Task Create Basic Demo");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "FreeRTOS Version: %s", tskKERNEL_VERSION_NUMBER);
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Tick Rate: %d Hz", configTICK_RATE_HZ);
    ESP_LOGI(TAG, "Number of Cores: %d", portNUM_PROCESSORS);
    ESP_LOGI(TAG, "Free Heap at start: %lu bytes",
             (unsigned long)esp_get_free_heap_size());

    printf("[DATA] INIT,%lu,%d,%lu\n",
           (unsigned long)get_elapsed_ms(),
           portNUM_PROCESSORS,
           (unsigned long)esp_get_free_heap_size());

    /* Inisialisasi hardware GPIO */
    init_gpio();

    /* Record heap sebelum pembuatan task */
    uint32_t heap_before = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Heap sebelum create tasks: %lu bytes",
             (unsigned long)heap_before);

    /*
     * Membuat Task 1 - LED1 Blinker (Prioritas Tinggi)
     *
     * xTaskCreate() mengalokasikan TCB (Task Control Block) dan stack
     * dari heap. TCB menyimpan context task (register, state, dll).
     *
     * Parameter pvParameters = (void*)1 menunjukkan ini task number 1.
     * Cast melalui intptr_t untuk portabilitas.
     */
    BaseType_t ret;

    ESP_LOGI(TAG, "Membuat Task1 (LED1 Blinker)...");
    ret = xTaskCreate(
        task1_led_blink,            // Fungsi task
        "LED1_Blink",               // Nama task (untuk debug)
        TASK_STACK_SIZE,            // Stack size (bytes pada ESP-IDF)
        (void *)(intptr_t)1,        // Parameter: task number = 1
        TASK1_PRIORITY,             // Prioritas = 3
        &xTask1Handle              // Output: task handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "GAGAL membuat Task1! Error: %d", ret);
    } else {
        ESP_LOGI(TAG, "Task1 berhasil dibuat. Handle: %p", (void*)xTask1Handle);
    }

    uint32_t heap_after_t1 = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Heap setelah Task1: %lu bytes (used: %lu bytes)",
             (unsigned long)heap_after_t1,
             (unsigned long)(heap_before - heap_after_t1));

    printf("[DATA] CREATE,Task1,%lu,%lu,%lu\n",
           (unsigned long)get_elapsed_ms(),
           (unsigned long)heap_after_t1,
           (unsigned long)(heap_before - heap_after_t1));

    /*
     * Membuat Task 2 - LED2 Blinker (Prioritas Lebih Rendah)
     *
     * Walaupun prioritas lebih rendah dari Task1, Task2 tetap berjalan
     * karena:
     * 1. Kedua task menggunakan vTaskDelay() sehingga sering BLOCKED
     * 2. Saat Task1 BLOCKED, scheduler menjalankan Task2
     * 3. Pada ESP32 dual-core, keduanya bisa berjalan truly parallel
     */
    ESP_LOGI(TAG, "Membuat Task2 (LED2 Blinker)...");
    ret = xTaskCreate(
        task2_led_blink,
        "LED2_Blink",
        TASK_STACK_SIZE,
        (void *)(intptr_t)2,
        TASK2_PRIORITY,
        &xTask2Handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "GAGAL membuat Task2! Error: %d", ret);
    } else {
        ESP_LOGI(TAG, "Task2 berhasil dibuat. Handle: %p", (void*)xTask2Handle);
    }

    uint32_t heap_after_t2 = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Heap setelah Task2: %lu bytes (used: %lu bytes)",
             (unsigned long)heap_after_t2,
             (unsigned long)(heap_after_t1 - heap_after_t2));

    printf("[DATA] CREATE,Task2,%lu,%lu,%lu\n",
           (unsigned long)get_elapsed_ms(),
           (unsigned long)heap_after_t2,
           (unsigned long)(heap_after_t1 - heap_after_t2));

    /*
     * Membuat Monitor Task
     *
     * Task ini berjalan di background dengan prioritas terendah,
     * secara periodik melaporkan status sistem. Pola ini umum
     * dalam embedded systems untuk health monitoring.
     */
    ESP_LOGI(TAG, "Membuat Monitor Task...");
    ret = xTaskCreate(
        monitor_task,
        "Monitor",
        TASK_STACK_SIZE,
        NULL,
        MONITOR_PRIORITY,
        &xMonitorHandle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "GAGAL membuat Monitor Task!");
    }

    uint32_t heap_final = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Semua task berhasil dibuat!");
    ESP_LOGI(TAG, "Total heap digunakan oleh tasks: %lu bytes",
             (unsigned long)(heap_before - heap_final));
    ESP_LOGI(TAG, "Heap sisa: %lu bytes", (unsigned long)heap_final);

    printf("[DATA] CREATE,AllTasks,%lu,%lu,%lu\n",
           (unsigned long)get_elapsed_ms(),
           (unsigned long)heap_final,
           (unsigned long)(heap_before - heap_final));

    /*
     * Setelah app_main() return, main task akan dihapus oleh FreeRTOS.
     * Semua task yang sudah dibuat di atas akan terus berjalan
     * secara independen di bawah kontrol FreeRTOS scheduler.
     *
     * Scheduler ESP-IDF sudah berjalan sebelum app_main() dipanggil,
     * jadi task-task di atas sudah mulai berjalan bahkan sebelum
     * app_main() return (karena mereka memiliki prioritas lebih tinggi).
     */
    ESP_LOGI(TAG, "app_main() selesai. Tasks berjalan secara independen.");
    ESP_LOGI(TAG, "============================================");
}
