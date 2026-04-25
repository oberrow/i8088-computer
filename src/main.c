/*
 * src/main.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "io.h"
#include "mem.h"

extern char __data_loadaddr, __data_start, __data_end;

__attribute__((noreturn)) void entry() {
    memcpy_far(&__data_loadaddr, 0xf000, &__data_start, 0x0000, &__data_end - &__data_start);
    //const char* lol = "hello";
    
    while (1)
        ;
}