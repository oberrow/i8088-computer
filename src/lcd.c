/*
 * src/lcd.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stdint.h>
#include <stdbool.h>

#include "lcd.h"
#include "io.h"
#include "extra.h"

// Drives the HD44780U, assuming it is driving an 16x2 LCD
// Probably not correct, we'll see when we actually get this together.

// some people might be stuck in this loop forever...
static bool wait_for_bf()
{
    uint32_t ms_passed = 0;
    uint16_t flags = clis();
    // Wait 20ms for initialization
    do {
        delay_cycles(US_IN_CYCLES(1000));
    } while (ms_passed++ < 10 && inb(LCD_BASE) & BIT(7) /* Busy flag */);
    resf(flags);
    return ~inb(LCD_BASE) & BIT(7);
}

void lcd_init() {
    delay_cycles(US_IN_CYCLES(15*1000));
    outb(LCD_BASE, 0x30);
    delay_cycles(US_IN_CYCLES(41*100));
    outb(LCD_BASE, 0x30);
    delay_cycles(US_IN_CYCLES(100));
    outb(LCD_BASE, 0x30);
    lcd_function_set(LCD_8BIT_IFACE, LCD_TWO_LINES, LCD_5x10_FONT);
    lcd_display_ctrl(false, false, false);
    lcd_entry_mode_set(LCD_CURSOR_DIR_INCREMENT, LCD_SHIFT_CURSOR);

    // Initialization sequence done, set sensible values.

    lcd_display_ctrl(true, true, false);

}

void lcd_clear() {
    wait_for_bf();
    outb(LCD_BASE, 0x1);
}

void lcd_return_home() {
    wait_for_bf();
    outb(LCD_BASE, 0x2);
}

void lcd_entry_mode_set(bool cursor_direction, bool shift) {
    uint8_t flags = 0;
    if (cursor_direction) 
        flags |= BIT(1);
    if (shift) 
        flags |= BIT(0);
    wait_for_bf();
    outb(LCD_BASE, 0x4 | flags);
}

void lcd_display_ctrl(bool display, bool cursor, bool blink) {
    uint8_t flags = 0;
    if (display) 
        flags |= BIT(2);
    if (cursor) 
        flags |= BIT(1);
    if (blink)
        flags |= BIT(0);
    wait_for_bf();
    outb(LCD_BASE, 0x8 | flags);
}

void lcd_cursor_shift(bool dir, bool shift_what) {
    uint8_t flags = 0;
    if (shift_what)
        flags |= BIT(3);
    if (dir) 
        flags |= BIT(2);
    wait_for_bf();
    outb(LCD_BASE, 0x10 | flags);
}

void lcd_function_set(bool data_length, bool line_count, bool font_type) {
    uint8_t flags = 0;
    if (data_length) 
        flags |= BIT(4);
    if (line_count) 
        flags |= BIT(3);
    if (font_type)
        flags |= BIT(2);
    wait_for_bf();
    outb(LCD_BASE, 0x20 | flags);
}


void lcd_write_byte(uint8_t address, char byte, bool ddram) {
    wait_for_bf();
    if (ddram)
        outb(LCD_BASE, (address & 0x3f) | BIT(7));
    else
        outb(LCD_BASE, (address & 0x1f) | BIT(6));

    wait_for_bf();
    outb(LCD_BASE+1, byte);
}

char lcd_read_byte(uint8_t address, bool ddram) {
    wait_for_bf();
    if (ddram)
        outb(LCD_BASE, (address & 0x3f) | BIT(7));
    else
        outb(LCD_BASE, (address & 0x1f) | BIT(6));

    wait_for_bf();
    return inb(LCD_BASE+1);
}