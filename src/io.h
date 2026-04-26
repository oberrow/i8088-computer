/*
 * src/io.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>

void outb(uint16_t addr, uint8_t val);
void outw(uint16_t addr, uint16_t val);

uint8_t inb(uint16_t addr);
uint16_t inw(uint16_t addr);

void cli();
void sti();

uint16_t clis();
void resf(uint16_t old_flags); // restore flags
uint16_t get_flags();

void hlt();

#define US_IN_CYCLES(us) (((us) - 15 - 2 - 16 - 2 - 12 - 20 - 5) / 17)
void delay_cycles(uint16_t cycles);

#define PIC_BASE 0x100
#define UART_BASE 0x200
#define LCD_BASE 0x400