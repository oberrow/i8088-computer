/*
 * src/main.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "io.h"
#include "mem.h"
#include "pic.h"

extern char __data_loadaddr, __data_start, __data_end;

__attribute__((noreturn)) void entry() {
    memcpy_far(&__data_loadaddr, 0xf000, &__data_start, 0x0000, &__data_end - &__data_start);
    pic_init();
    pic_mask(0xff);
    sti();
    
    while (1)
        ;
}