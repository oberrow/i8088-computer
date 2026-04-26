/*
 * src/pic.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>

void pic_init();
void pic_eoi();
void pic_mask(uint8_t mask);
void pic_clear_mask(uint8_t mask);
void pic_set_mask(uint8_t mask);
uint8_t pic_get_mask(void);
uint8_t pic_get_irr(void);
uint8_t pic_get_isr(void);

void ivt_init();

extern uint16_t pic_spurious_irq_count;