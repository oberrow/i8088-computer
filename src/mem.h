/*
 * src/mem.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "extra.h"

void *memcpy(void *dest, const void *src, size_t len);
void *memcpy_far(void *dest, uint16_t es, const void *src, uint16_t ds, size_t len);
void *memset(void *dest, char c, size_t size);
int memcmp(const void  *src1, const void *src2, size_t len);
int strlen(const char* str);

void set_ds(uint16_t segment);
void set_es(uint16_t segment);

#define CODE_SEGMENT 0xc000

#define FP_OFF(x) ((void*)__builtin_ia16_FP_OFF(x))
#define FP_SEG(x) ((uint32_t)(((uint32_t)(x) & ~0xffffUL) >> 4))