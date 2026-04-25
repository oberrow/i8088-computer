/*
 * src/main.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "io.h"
#include "mem.h"

#define PIC_BASE 0x100

void entry() {
    cli();
    while (1)
        ;
}