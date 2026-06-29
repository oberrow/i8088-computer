/*
 * src/main.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "io.h"
#include "mem.h"
#include "pic.h"
#include "uart.h"
#include "extra.h"
#include "i8255.h"

extern char __data_loadaddr, __data_start, __data_end;

extern void gdbstub_initialize();
// #define GDBSTUB 1

void *memcpy_far(void *dest, uint16_t es, const void *src, uint16_t ds, size_t len);

void int2hex(char* out, int len, uint32_t x) {
    extern char bin2hex[16];
    int i = 0;
    do {
        out[i] = bin2hex[x & 0xf];
        x >>= 4;
    } while(i++ < len && x);
    out[i] = 0;
    for (int x = 0, y = i-1; x < y; x++, y--) {
        char c = out[x];
        out[x] = out[y];
        out[y] = c;
    }
}

__attribute__((noreturn)) void entry() {
    memcpy_far(&__data_start, 0x0000, &__data_loadaddr, CODE_SEGMENT, &__data_end - &__data_start);
    ivt_init();

    pic_init();
    pic_mask(0xff);
    sti();

    probe_io_bus();
    
#if GDBSTUB
    gdbstub_initialize();
    BP();
#else
    char hex_string[6] = {};
    int2hex(hex_string, 5, bus_info.ram_size);

    uart_write("Initialized and probed motherboard.\0x", 37);
    uart_write(hex_string, strlen(hex_string));
    uart_write(" bytes of RAM detected\nDetected ", 32);
    uart_writeb(bus_info.active_slot_count + '0');
    uart_write("active I/O slots\n", 17);
#endif

    while (1)
        hlt();
}

void outs(uint16_t addr, const char* str) {
    while (*str)
        outb(addr, *str++);
}