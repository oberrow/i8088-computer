/*
 * src/main.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "io.h"
#include "mem.h"
#include "pic.h"
#include "uart.h"
#include "lcd.h"

extern char __data_loadaddr, __data_start, __data_end;

__attribute__((noreturn)) void entry() {
    memcpy_far(&__data_loadaddr, 0xf000, &__data_start, 0x0000, &__data_end - &__data_start);
    ivt_init();
    pic_init();
    pic_mask(0xff);
    sti();

    uart_init(9600, ONE_STOPBIT, EIGHT_DATABITS, PARITYBIT_NONE);
    lcd_init();

    char buf[] = "Hello, world!\r\n";
    uart_write(buf, sizeof(buf));
    for (int addr = 0; addr < (int)sizeof(buf); addr++)
        lcd_write_byte(addr, buf[addr], LCD_ACCESS_DDRAM);
    
    while (1)
        hlt();
}