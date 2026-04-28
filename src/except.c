/*
 * src/except.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "frame.h"

void except_div0(struct irq_frame* frame) {
    while (1)
        asm volatile("hlt" :::"memory");
}
void except_trap(struct irq_frame* frame) {
    while (1)
        asm volatile("hlt" :::"memory");
}
void except_nmi(struct irq_frame* frame) {
    while (1)
        asm volatile("hlt" :::"memory");
}
void except_bp(struct irq_frame* frame) {
    while (1)
        asm volatile("hlt" :::"memory");
}
void except_overflow(struct irq_frame* frame) {
    while (1)
        asm volatile("hlt" :::"memory");
}