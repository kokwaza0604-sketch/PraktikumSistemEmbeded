#ifndef CONFIG_H
#define CONFIG_H

#define LED_GPIO_PIN               2
#define TASK_STACK_SIZE            2048
#define TASK_PRIORITY              1

#define EVENT_BIT_0                0x01
#define EVENT_BIT_1                0x02
#define EVENT_BIT_2                0x04

// I2C LCD (PCF8574) settings
#define I2C_MASTER_SCL_IO          22
#define I2C_MASTER_SDA_IO          21
#define I2C_MASTER_NUM             I2C_NUM_0
#define I2C_MASTER_FREQ_HZ         100000
#define I2C_MASTER_TX_BUF_DISABLE  0
#define I2C_MASTER_RX_BUF_DISABLE  0
#define PCF8574_ADDR               0x27

// ADC (potentiometer) settings
// Define GPIO number for pot (e.g., GPIO36 on ADC1)
#define POT_ADC_GPIO               36
#define POT_SAMPLE_PERIOD_MS       100

// Servo settings
#define SERVO_GPIO                 18
#define SERVO_LEDC_TIMER           LEDC_TIMER_0
#define SERVO_LEDC_CHANNEL         LEDC_CHANNEL_0
#define SERVO_LEDC_SPEED_MODE      LEDC_HIGH_SPEED_MODE
#define SERVO_FREQ_HZ              50
#define SERVO_MIN_PULSE_US         500
#define SERVO_MAX_PULSE_US         2500

// HC-SR04 ultrasonic sensor pins
#define ULTRASONIC_TRIG_GPIO       5
#define ULTRASONIC_ECHO_GPIO       4
#define ULTRASONIC_MAX_ECHO_US     25000 // 25 ms

// LCD update period (ms)
#define LCD_UPDATE_PERIOD_MS       250

#endif
