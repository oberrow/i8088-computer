/*
 * src/mem.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stddef.h>
#include "mem.h"

void *memcpy(void *destp, const void *srcp, size_t len) {
    char* dest = destp;
    const char* src = srcp;
    for (size_t i = 0; i < len; i++)
        dest[i] = src[i];
    return dest;
}
void *memset(void *destp, char c, size_t size) {
    char* dest = destp;
    for (size_t i = 0; i < size; i++)
        dest[i] = c;
    return dest;
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
int strlen(const char* str)
{
    int len = 0;
    while(str[len++])
        ;
    return len;
}