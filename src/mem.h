/*
 * src/mem.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t len);
void *memset(void *dest, char c, size_t size);
int memcmp(const void *src1, const void *src2, size_t len);