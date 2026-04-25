/*
 * src/pic.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>

struct irq_frame {
    uint16_t ss, es, ds;
    uint16_t di, si, bp, bx, dx, cx;
    uint16_t vector;
    uint16_t ax;
    uint16_t ip, cs, flags;
};