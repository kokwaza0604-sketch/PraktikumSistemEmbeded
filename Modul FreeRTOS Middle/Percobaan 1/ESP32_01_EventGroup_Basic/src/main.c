// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Event Group Basic + LCD I2C, ADC, HC-SR04, Servo
// Tampilan LCD menampilkan jarak (HC-SR04) dan sudut servo (dikontrol oleh potensiometer via ADC)

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "config.h"

static const char *TAG = "EventGroup_Basic";

// Event group handle (existing)
EventGroupHandle_t xEventGroup;

// Mutex to protect shared sensor/actuator data
static SemaphoreHandle_t xDataMutex = NULL;

// Shared data
static int g_servo_angle = 0;      // degrees 0-180
static float g_distance_cm = 0.0f; // measured distance

// LCD / PCF8574 control bits
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_RW        0x02
#define LCD_RS        0x01

// Forward prototypes for existing tasks
void vTask1(void *pvParameters);
void vTask2(void *pvParameters);
void vTask3(void *pvParameters);
void vEventMonitorTask(void *pvParameters);

// New task prototypes
void vAdcTask(void *pvParameters);
void vServoTask(void *pvParameters);
void vUltrasonicTask(void *pvParameters);
void vLcdTask(void *pvParameters);

// I2C helper: write single byte to PCF8574
static esp_err_t pcf8574_write(uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF8574_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Low level LCD helpers (4-bit mode via PCF8574)
static void lcd_pulse_enable(uint8_t data)
{
    pcf8574_write(data | LCD_ENABLE);
    esp_rom_delay_us(1);
    pcf8574_write(data & ~LCD_ENABLE);
    esp_rom_delay_us(50);
}

static void lcd_write4bits(uint8_t value)
{
    // value already aligned to high nibble (D7..D4)
    pcf8574_write(value | LCD_BACKLIGHT);
    lcd_pulse_enable(value | LCD_BACKLIGHT);
}

static void lcd_send(uint8_t value, uint8_t mode)
{
    uint8_t highnib = value & 0xF0;
    uint8_t lownib = (value << 4) & 0xF0;
    lcd_write4bits(highnib | mode);
    lcd_write4bits(lownib | mode);
}

static void lcd_command(uint8_t cmd)
{
    lcd_send(cmd, 0);
}

static void lcd_write_char(char c)
{
    lcd_send((uint8_t)c, LCD_RS);
}

static void lcd_print(const char *str)
{
    while (*str)
    {
        lcd_write_char(*str++);
    }
}

static void lcd_clear()
{
    lcd_command(0x01);
    esp_rom_delay_us(2000);
}

static void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t pos = (row == 0) ? 0x00 : 0x40;
    pos += col;
    lcd_command(0x80 | pos);
}

// Initialize I2C peripheral for LCD
static esp_err_t i2c_master_init_lcd()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {.clk_speed = I2C_MASTER_FREQ_HZ},
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    return err;
}

// Initialize the 16x2 LCD via PCF8574
static void lcd_init()
{
    // initialization sequence for 4-bit mode
    esp_rom_delay_us(50000);
    // according to HD44780 init sequence
    lcd_write4bits(0x30);
    esp_rom_delay_us(4500);
    lcd_write4bits(0x30);
    esp_rom_delay_us(150);
    lcd_write4bits(0x30);
    esp_rom_delay_us(150);
    lcd_write4bits(0x20); // set 4-bit mode
    esp_rom_delay_us(150);

    lcd_command(0x28); // function set: 4-bit, 2 line, 5x8 dots
    lcd_command(0x08); // display off
    lcd_clear();
    lcd_command(0x06); // entry mode set
    lcd_command(0x0C); // display on, cursor off
}

// Servo helpers
static void servo_init()
{
    ledc_timer_config_t timer = {
        .speed_mode = SERVO_LEDC_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = SERVO_LEDC_TIMER,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .speed_mode = SERVO_LEDC_SPEED_MODE,
        .channel = SERVO_LEDC_CHANNEL,
        .timer_sel = SERVO_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch);
}

static void servo_set_angle(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    const int max_duty = (1 << 13) - 1; // 13-bit resolution
    float period_us = 1000000.0f / SERVO_FREQ_HZ;
    float pulse_us = SERVO_MIN_PULSE_US + (angle / 180.0f) * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
    uint32_t duty = (uint32_t)((pulse_us / period_us) * max_duty);
    ledc_set_duty(SERVO_LEDC_SPEED_MODE, SERVO_LEDC_CHANNEL, duty);
    ledc_update_duty(SERVO_LEDC_SPEED_MODE, SERVO_LEDC_CHANNEL);
}

// Ultrasonic sensor init and measure
static void ultrasonic_init()
{
    gpio_reset_pin(ULTRASONIC_TRIG_GPIO);
    gpio_set_direction(ULTRASONIC_TRIG_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ULTRASONIC_TRIG_GPIO, 0);

    gpio_reset_pin(ULTRASONIC_ECHO_GPIO);
    gpio_set_direction(ULTRASONIC_ECHO_GPIO, GPIO_MODE_INPUT);
}

static float ultrasonic_measure_cm()
{
    // Trigger 10us pulse
    gpio_set_level(ULTRASONIC_TRIG_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_level(ULTRASONIC_TRIG_GPIO, 1);
    esp_rom_delay_us(10);
    gpio_set_level(ULTRASONIC_TRIG_GPIO, 0);

    // wait for echo rising edge
    int64_t start_time = esp_timer_get_time();
    int64_t timeout = start_time + ULTRASONIC_MAX_ECHO_US;
    while (gpio_get_level(ULTRASONIC_ECHO_GPIO) == 0)
    {
        if (esp_timer_get_time() > timeout) return -1.0f;
    }
    int64_t t1 = esp_timer_get_time();

    // wait for echo falling edge
    while (gpio_get_level(ULTRASONIC_ECHO_GPIO) == 1)
    {
        if (esp_timer_get_time() > timeout) return -1.0f;
    }
    int64_t t2 = esp_timer_get_time();

    int64_t diff = t2 - t1; // microseconds
    float distance_cm = (float)diff / 58.0f; // approximate conversion
    return distance_cm;
}

// TASKS

// Implementasi Task1 - memberi sinyal bit 0 secara periodik
void vTask1(void *pvParameters)
{
    TickType_t xDelay = pdMS_TO_TICKS(1000);
    while (1)
    {
        gpio_set_level(LED_GPIO_PIN, 1);
        printf("Task1: Memberi sinyal EVENT_BIT_0\n");
        xEventGroupSetBits(xEventGroup, EVENT_BIT_0);
        vTaskDelay(xDelay);
        gpio_set_level(LED_GPIO_PIN, 0);
        printf("Task1: Menunggu 2 detik...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Implementasi Task2 - memberi sinyal bit 1 secara periodik
void vTask2(void *pvParameters)
{
    TickType_t xDelay = pdMS_TO_TICKS(1500);
    while (1)
    {
        printf("Task2: Memberi sinyal EVENT_BIT_1\n");
        xEventGroupSetBits(xEventGroup, EVENT_BIT_1);
        vTaskDelay(xDelay);
        printf("Task2: Menunggu 2.5 detik...\n");
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

// Implementasi Task3 - memberi sinyal bit 2 secara periodik
void vTask3(void *pvParameters)
{
    while (1)
    {
        printf("Task3: Memberi sinyal EVENT_BIT_2\n");
        xEventGroupSetBits(xEventGroup, EVENT_BIT_2);
        vTaskDelay(pdMS_TO_TICKS(3000));
        printf("Task3: Menunggu 2 detik...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Implementasi Event Monitor Task - menunggu event dengan AND dan OR condition
void vEventMonitorTask(void *pvParameters)
{
    EventBits_t uxBits;
    while (1)
    {
        printf("\nMonitor: Menunggu event AND (BIT0 DAN BIT1)...\n");
        uxBits = xEventGroupWaitBits(xEventGroup, EVENT_BIT_0 | EVENT_BIT_1, pdTRUE, pdTRUE, portMAX_DELAY);
        printf("Monitor: Event AND terpenuhi! Bit: 0x%lx\n", uxBits);

        printf("Monitor: Menunggu event OR (BIT1 ATAU BIT2)...\n");
        uxBits = xEventGroupWaitBits(xEventGroup, EVENT_BIT_1 | EVENT_BIT_2, pdTRUE, pdFALSE, portMAX_DELAY);
        printf("Monitor: Event OR terpenuhi! Bit: 0x%lx\n", uxBits);

        printf("Monitor: Siklus selesai, mengulang...\n\n");
    }
}

// ADC task: read potentiometer and update servo angle
void vAdcTask(void *pvParameters)
{
    adc_oneshot_unit_handle_t adc_handle = NULL;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .clk_src = 0,
        .ulp_mode = 0,
    };
    if (adc_oneshot_new_unit(&init_cfg, &adc_handle) != ESP_OK)
    {
        printf("ADC oneshot init failed\n");
        vTaskDelete(NULL);
        return;
    }

    adc_unit_t unit_id;
    adc_channel_t channel;
    if (adc_oneshot_io_to_channel(POT_ADC_GPIO, &unit_id, &channel) != ESP_OK)
    {
        printf("Invalid ADC GPIO %d\n", POT_ADC_GPIO);
        adc_oneshot_del_unit(adc_handle);
        vTaskDelete(NULL);
        return;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adc_handle, channel, &chan_cfg);

    while (1)
    {
        int raw = 0;
        if (adc_oneshot_read(adc_handle, channel, &raw) == ESP_OK)
        {
            int angle = (raw * 180) / 4095;
            if (angle < 0) angle = 0;
            if (angle > 180) angle = 180;
            if (xDataMutex && xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
            {
                g_servo_angle = angle;
                xSemaphoreGive(xDataMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(POT_SAMPLE_PERIOD_MS));
    }

    adc_oneshot_del_unit(adc_handle);
}

// Servo task: read desired angle and set PWM
void vServoTask(void *pvParameters)
{
    int last_angle = -1;
    while (1)
    {
        int angle = 0;
        if (xDataMutex && xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            angle = g_servo_angle;
            xSemaphoreGive(xDataMutex);
        }
        if (angle != last_angle)
        {
            servo_set_angle(angle);
            last_angle = angle;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Ultrasonic task: measure distance and update shared variable
void vUltrasonicTask(void *pvParameters)
{
    ultrasonic_init();
    while (1)
    {
        float d = ultrasonic_measure_cm();
        if (d < 0) d = -1.0f;
        if (xDataMutex && xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            g_distance_cm = d;
            xSemaphoreGive(xDataMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// LCD task: update display with distance and servo angle
void vLcdTask(void *pvParameters)
{
    // initialize I2C and LCD
    if (i2c_master_init_lcd() != ESP_OK)
    {
        printf("I2C init failed\n");
        vTaskDelete(NULL);
        return;
    }
    lcd_init();

    char line[17];
    while (1)
    {
        int angle = 0;
        float dist = 0.0f;
        if (xDataMutex && xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            angle = g_servo_angle;
            dist = g_distance_cm;
            xSemaphoreGive(xDataMutex);
        }

        lcd_clear();
        lcd_set_cursor(0, 0);
        if (dist < 0)
            snprintf(line, sizeof(line), "Dist: -- cm");
        else
            snprintf(line, sizeof(line), "Dist:%6.2fcm", dist);
        lcd_print(line);

        lcd_set_cursor(1, 0);
        snprintf(line, sizeof(line), "Servo:%3d deg", angle);
        lcd_print(line);

        vTaskDelay(pdMS_TO_TICKS(LCD_UPDATE_PERIOD_MS));
    }
}

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Basic LED init
    gpio_reset_pin(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO_PIN, 0);

    // create event group
    xEventGroup = xEventGroupCreate();
    if (xEventGroup == NULL)
    {
        printf("Gagal membuat Event Group! Memori tidak cukup.\n");
        return;
    }

    printf("=== ESP32 FreeRTOS Event Group Basic + Sensors/Actuators ===\n");

    // create data mutex
    xDataMutex = xSemaphoreCreateMutex();

    // init servo hardware
    servo_init();

    // create core tasks
    xTaskCreate(vTask1, "Task1", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    xTaskCreate(vTask2, "Task2", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    xTaskCreate(vTask3, "Task3", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    xTaskCreate(vEventMonitorTask, "EventMonitor", TASK_STACK_SIZE, NULL, TASK_PRIORITY + 1, NULL);

    // create sensor/actuator tasks
    xTaskCreate(vAdcTask, "ADC", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    xTaskCreate(vServoTask, "Servo", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    xTaskCreate(vUltrasonicTask, "Ultrasonic", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    xTaskCreate(vLcdTask, "LCD", TASK_STACK_SIZE * 2, NULL, TASK_PRIORITY, NULL);

    printf("Semua task telah dibuat dan berjalan\n");
}
