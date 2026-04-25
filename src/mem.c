/*
 * src/mem.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stddef.h>
#include "mem.h"

void *memcpy(void *dest, const void *src, size_t len) {
    void* const destc = dest;
    while (--len > 0)
        *(((char*)(dest++))) = *(((char*)(src++)));
    return destc;
}
void *memset(void *dest, char c, size_t size) {
    void* const destc = dest;
    while (--size > 0)
        *(((char*)(dest++))) = c;
    return destc;
}
int memcmp(const void *src1p, const void *src2p, size_t len) {
    const char* src1 = src1p;
    const char* src2 = src2p;
    while (--len > 0)
    {
        if (*src1 < *src2) return -1;
        else if (*src1 > *src2) return 1;
        src1++;
        src2++;
    }
    return 0;
}