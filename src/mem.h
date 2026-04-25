/*
 * src/mem.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef __ia16__
#define __far
#endif

__far void *memcpy(void __far *dest, const void __far *src, size_t len);
void *memcpy_far(void *dest, uint16_t es, const void *src, uint16_t ds, size_t len);
__far void *memset(void __far *dest, char c, size_t size);
int memcmp(const void  __far *src1, const void __far *src2, size_t len);

void set_ds(uint16_t segment);
void set_es(uint16_t segment);