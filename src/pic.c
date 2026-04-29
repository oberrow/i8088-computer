/*
 * src/pic.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stdint.h>
#include <stddef.h>

#include "io.h"
#include "pic.h"
#include "frame.h"
#include "uart.h"
#include "mem.h"

void pic_init() {
    // ICW1
    outb(PIC_BASE, 0x13);
    // ICW2
    outb(PIC_BASE+1, 0x20); // Vectors start at 0x20
    // ICW4 (ICW3 skipped)
    outb(PIC_BASE+1, 0x01);
}

void pic_eoi() {
    outb(PIC_BASE, 0x20);
}

void pic_mask(uint8_t mask) {
    pic_set_mask(mask | pic_get_mask());
}
void pic_clear_mask(uint8_t mask) {
    pic_set_mask(pic_get_mask() & ~mask);
}

void pic_set_mask(uint8_t mask) {
    outb(PIC_BASE+1, mask);
}

uint8_t pic_get_mask(void) {
    return inb(PIC_BASE+1);
}

uint8_t pic_get_irr(void) {
    outb(PIC_BASE, 0b1010);
    return inb(PIC_BASE);
}

uint8_t pic_get_isr(void) {
    outb(PIC_BASE, 0b1011);
    return inb(PIC_BASE);
}

void except_div0(struct irq_frame* frame);
void except_trap(struct irq_frame* frame);
void except_nmi(struct irq_frame* frame);
void except_bp(struct irq_frame* frame);
void except_overflow(struct irq_frame* frame);

uint16_t pic_spurious_irq_count;

void spurious_irq(struct irq_frame* frame)
{
    (void)frame;
    pic_spurious_irq_count++;
}

__attribute__((section(".flash"))) void(*const irq_handlers[])(struct irq_frame* frame) = {
    except_div0,
    except_trap,
    except_nmi,
    except_bp,
    except_overflow,
    [32]=0, 0, 0, 0, 0, 0, 0, uart_irq, spurious_irq
};

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void timer_irq();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr38();
extern void isr39();
extern void isr40();

void ivt_init() {
    uint16_t *ivt = NULL;
    ivt[0*2+0] = (uint16_t)isr0;
    ivt[1*2+0] = (uint16_t)isr1;
    ivt[2*2+0] = (uint16_t)isr2;
    ivt[3*2+0] = (uint16_t)isr3;
    ivt[4*2+0] = (uint16_t)isr4;
    ivt[32*2+0] = (uint16_t)timer_irq;
    ivt[33*2+0] = (uint16_t)isr33;
    ivt[34*2+0] = (uint16_t)isr34;
    ivt[35*2+0] = (uint16_t)isr35;
    ivt[36*2+0] = (uint16_t)isr36;
    ivt[37*2+0] = (uint16_t)isr37;
    ivt[38*2+0] = (uint16_t)isr38;
    ivt[39*2+0] = (uint16_t)isr39;
    ivt[40*2+0] = (uint16_t)isr40;
    for (int i = 0; i <= 4; i++)
        ivt[i*2+1] = CODE_SEGMENT;
    for (int i = 32; i <= 40; i++)
        ivt[i*2+1] = CODE_SEGMENT;
}