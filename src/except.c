/*
 * src/except.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "frame.h"
#include "uart.h"

void except_div0(struct irq_frame* frame) {
    uart_write("DIV0 Exception!", 16);
    while (1)
        asm volatile("hlt" :::"memory");
}
void except_trap(struct irq_frame* frame) {
    uart_write("TRAP Exception!", 16);
    while (1)
        asm volatile("hlt" :::"memory");
}
void except_nmi(struct irq_frame* frame) {
    uart_write("NMI Received!", 14);
    while (1)
        asm volatile("hlt" :::"memory");
}
void except_bp(struct irq_frame* frame) {
    uart_write("INT3 Received!", 15);
    while (1)
        asm volatile("hlt" :::"memory");
}
void except_overflow(struct irq_frame* frame) {
    uart_write("OVERFLOW Exception!", 20);
    while (1)
        asm volatile("hlt" :::"memory");
}