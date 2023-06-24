/* Found in https://github.com/maxsydney/ESP32-HD44780/blob/master/components/HD44780/ */
/* Developed by: Max Sydney Smith */

#pragma once

void LCD_init(uint8_t addr, uint8_t dataPin, uint8_t clockPin, uint8_t cols, uint8_t rows);
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_home(void);
void LCD_clearScreen(void);
void LCD_writeChar(char c);
void LCD_writeStr(char* str); 