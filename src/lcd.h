/*
 * src/lcd.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Drives the HD44780U, assuming it is driving an 16x2 LCD

void lcd_init();
void lcd_clear();
void lcd_return_home();

#define LCD_CURSOR_DIR_INCREMENT (true)
#define LCD_CURSOR_DIR_DECREMENT (false)
void lcd_entry_mode_set(bool cursor_dir, bool shift);

void lcd_display_ctrl(bool display, bool cursor, bool blink);

#define LCD_SHIFT_RIGHT (true)
#define LCD_SHIFT_LEFT (false)
#define LCD_SHIFT_DISPLAY (true)
#define LCD_SHIFT_CURSOR (false)
void lcd_cursor_shift(bool dir, bool shift_what);

#define LCD_8BIT_IFACE (true)
#define LCD_4BIT_IFACE (false)
#define LCD_ONE_LINE (false)
#define LCD_TWO_LINES (true)
#define LCD_5x8_FONT (false)
#define LCD_5x10_FONT (true)
void lcd_function_set(bool data_length, bool line_count, bool font_type);

#define lcd_get_line_address(x,y) (x + y*0x40)

#define LCD_ACCESS_CGRAM (false)
#define LCD_ACCESS_DDRAM (true)
void lcd_write_byte(uint8_t address, char byte, bool ddram);
char lcd_read_byte(uint8_t address, bool ddram);