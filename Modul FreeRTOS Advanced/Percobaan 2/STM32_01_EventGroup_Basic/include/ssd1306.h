#ifndef SSD1306_H
#define SSD1306_H

#include "main.h"
#include <stdint.h>

void ssd1306_init(I2C_HandleTypeDef *hi2c);
void ssd1306_clear(void);
void ssd1306_set_cursor(uint8_t x, uint8_t y);
void ssd1306_write_string(const char *str);
void ssd1306_display(void);

#endif
