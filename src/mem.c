/*
 * src/mem.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stddef.h>
#include "mem.h"

__far void *memcpy(void __far *dest, const void __far *src, size_t len) {
    void __far* const destc = dest;
    while (--len > 0)
        *(((char*)(dest++))) = *(((char*)(src++)));
    return destc;
}
__far void *memset(void __far *dest, char c, size_t size) {
    void __far* const destc = dest;
    while (--size > 0)
        *(((char*)(dest++))) = c;
    return destc;
}
int memcmp(const void __far *src1p, const void __far *src2p, size_t len) {
    const char __far* src1 = src1p;
    const char __far* src2 = src2p;
    while (--len > 0)
    {
        if (*src1 < *src2) return -1;
        else if (*src1 > *src2) return 1;
        src1++;
        src2++;
    }
    return 0;
}