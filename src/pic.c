/*
 * src/pic.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stdint.h>

#include "io.h"
#include "pic.h"

void pic_init()
{
    // ICW1
    outb(PIC_BASE, 0x13);
    // ICW2
    outb(PIC_BASE+1, 0x20); // Vectors start at 0x20
    // ICW4 (ICW3 skipped)
    outb(PIC_BASE+1, 0x01);
}

void pic_eoi()
{
    outb(PIC_BASE, 0x20);
}

void pic_mask(uint8_t mask)
{
    pic_set_mask(mask|pic_get_mask());
}

void pic_set_mask(uint8_t mask)
{
    outb(PIC_BASE+1, mask);
}

uint8_t pic_get_mask(void)
{
    return inb(PIC_BASE+1);
}

uint8_t pic_get_irr(void)
{
    outb(PIC_BASE, 0b1010);
    return inb(PIC_BASE);
}

uint8_t pic_get_isr(void)
{
    outb(PIC_BASE, 0b1011);
    return inb(PIC_BASE);
}