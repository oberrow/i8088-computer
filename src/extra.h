/*
 * src/extra.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifndef __ia16__
#define __far
#define __builtin_ia16_FP_OFF(x) (x)
#endif

#define BIT(n) BIT_TYPE(n, )
#define BIT_TYPE(n, t) (1##t << (n))

#define BP() do { asm volatile("int3"); } while(0)