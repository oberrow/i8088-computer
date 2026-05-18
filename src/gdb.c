/*
 * src/gdb.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stdbool.h>
#include <stddef.h>

#include "frame.h"
#include "uart.h"
#include "extra.h"
#include "mem.h"
#include "io.h"

static bool gdbstub_initialized = false;

static uint8_t mod256(const char* data, int len) {
    uint8_t checksum = 0;
    for (int i = 0; i < len; i++)
        checksum += data[i];
    return checksum;
}

static uint32_t hex2bin(const char* str, unsigned size) {
    uint32_t ret = 0;
    str += *str == '\n';
    for (int i = size - 1, j = 0; i > -1; i--, j++) {
        char c = str[i];
        uint32_t digit = 0;
        switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                digit = c - '0';
                break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                digit = (c - 'A') + 10;
                break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                digit = (c - 'a') + 10;
                break;
            default:
                break;
        }
        ret |= digit << ((uint32_t)j * (uint32_t)4);
    }
    return ret;
}

__attribute__((nonstring)) char bin2hex[16] = "0123456789abcdef";

static void gdbstub_trap(struct irq_frame* frame);

__attribute__((alias("gdbstub_trap"))) void except_bp(struct irq_frame* frame);
__attribute__((alias("gdbstub_trap"))) void except_trap(struct irq_frame* frame);

bool gdbstub_write(const char* data, int len) {
    uint8_t chksum = mod256(data, len);
    uint8_t i = 0;
    char resp = 0;
    do {
        uart_writeb('$');
        uart_write(data, len);
        uart_writeb('#');
        uart_writeb(bin2hex[(chksum >> 4) & 0xf]);
        uart_writeb(bin2hex[chksum & 0xf]);
    } while((resp = uart_readb(false)) != '+' && i++ < 3);
    return resp == '+';
}
int gdbstub_receive(char* data, int maxlen) {
    uint8_t chksum, rchksum, tries = 0;
    int len;

    do {
        if (tries > 0)
            uart_writeb('-');

        len = 0;
        chksum = 0;
        rchksum = 0;

        while (uart_readb(false) != '$')
            ;
    
        for (; len < maxlen; len++) {
            data[len] = uart_readb(false);
            if (data[len] == '#') {
                // len--;
                break;
            }
        }

        chksum = mod256(data, len);

        char rsum[2] = {};
        rsum[0] = uart_readb(false);
        rsum[1] = uart_readb(false);
    
        rchksum = hex2bin(rsum, 2);

        // outb(0xe9, 'r');    
        // outb(0xe9, bin2hex[(rchksum >> 4) & 0xf]);
        // outb(0xe9, bin2hex[(rchksum >> 0) & 0xf]);
        // outb(0xe9, 'l');    
        // outb(0xe9, bin2hex[(chksum >> 4) & 0xf]);
        // outb(0xe9, bin2hex[(chksum >> 0) & 0xf]);
        // outb(0xe9, 0xa);
    } while (chksum != rchksum && tries++ < 3);
    
    if (chksum != rchksum)
        return -1;

    uart_writeb('+');

    return len;
}

#define g_set_register(str, val, idx) \
do {\
    uint8_t _idx = (idx);\
    uint32_t _val = (val);\
    str[(_idx * 8) + 6] = bin2hex[(_val >> 28) & 0xf];\
    str[(_idx * 8) + 7] = bin2hex[(_val >> 24) & 0xf];\
    str[(_idx * 8) + 4] = bin2hex[(_val >> 20) & 0xf];\
    str[(_idx * 8) + 5] = bin2hex[(_val >> 16) & 0xf];\
    str[(_idx * 8) + 2] = bin2hex[(_val >> 12) & 0xf];\
    str[(_idx * 8) + 3] = bin2hex[(_val >> 8) & 0xf];\
    str[(_idx * 8) + 0] = bin2hex[(_val >> 4) & 0xf];\
    str[(_idx * 8) + 1] = bin2hex[(_val >> 0) & 0xf];\
} while(0)
#define g_read_register(str, idx) ((uint16_t)__builtin_bswap32(hex2bin(&str[(idx)*8], 8)))

static void gdbstub_read_registers(struct irq_frame* frame) {
    char resp[128] = {};
    uint8_t idx = 0;
    g_set_register(resp, frame->ax, idx++);
    g_set_register(resp, frame->cx, idx++);
    g_set_register(resp, frame->dx, idx++);
    g_set_register(resp, frame->bx, idx++);
    g_set_register(resp, (uint16_t)(&frame->flags + 1), idx++);
    g_set_register(resp, frame->bp, idx++);
    g_set_register(resp, frame->si, idx++);
    g_set_register(resp, frame->di, idx++);
    g_set_register(resp, ((uint32_t)frame->ip + (frame->cs*0x10UL)), idx++);
    g_set_register(resp, frame->flags, idx++);
    g_set_register(resp, frame->cs, idx++);
    g_set_register(resp, frame->ss, idx++);
    g_set_register(resp, frame->ds, idx++);
    g_set_register(resp, frame->es, idx++);
    g_set_register(resp, 0, idx++);
    g_set_register(resp, 0, idx++);

    gdbstub_write(resp, sizeof(resp));
}

static void gdbstub_read_register(const char* payload, int pckt_len, struct irq_frame* frame)
{
    char resp[8] = {};
    int idx = hex2bin(payload, pckt_len);
    uint32_t reg = 0;
    switch (idx) {
        case 0: reg = frame->ax; break;
        case 1: reg = frame->cx; break;
        case 2: reg = frame->dx; break;
        case 3: reg = frame->bx; break;
        case 4: reg = (uint16_t)(&frame->flags + 1); break;
        case 5: reg = frame->bp; break;
        case 6: reg = frame->si; break;
        case 7: reg = frame->di; break;
        case 8: reg = ((uint32_t)frame->ip + (frame->cs*0x10UL)); break;
        case 9: reg = frame->flags; break;
        case 10: reg = frame->cs; break;
        case 11: reg = frame->ss; break;
        case 12: reg = frame->ds; break;
        case 13: reg = frame->es; break;
        case 14: reg = 0; break;
        case 15: reg = 0; break;
        default: reg = 0xffff; break;
    }

    g_set_register(resp, reg, 0);

    gdbstub_write(resp, sizeof(resp));
}

static void gdbstub_write_registers(const char* payload, struct irq_frame* frame) {
    uint8_t idx = 0;
    frame->ax = g_read_register(payload, idx++);
    frame->cx = g_read_register(payload, idx++);
    frame->dx = g_read_register(payload, idx++);
    frame->bx = g_read_register(payload, idx++);
    idx++;
    frame->bp = g_read_register(payload, idx++);
    frame->si = g_read_register(payload, idx++);
    frame->di = g_read_register(payload, idx++);
    frame->ip = g_read_register(payload, idx++);
    frame->flags = g_read_register(payload, idx++);
    frame->cs = g_read_register(payload, idx++);
    idx++;
    // frame->ss = g_read_register(payload, idx++);
    frame->ds = g_read_register(payload, idx++);
    frame->es = g_read_register(payload, idx++);
    
    gdbstub_write("OK", 2);
}

static void gdbstub_continue(const char* payload, int len, struct irq_frame* frame) {
    if (len > 1) {
        uintptr_t linear_address = hex2bin(payload+1, len-1);
        frame->ip = linear_address & 0xffff;
        frame->cs = (linear_address & ~0xffff) >> 4;
    }

    frame->flags &= ~BIT(8);
}

static void gdbstub_step(const char* payload, int len, struct irq_frame* frame) {
    if (len > 1) {
        uintptr_t linear_address = hex2bin(payload+1, len-1);
        frame->ip = linear_address & 0xffff;
        frame->cs = (linear_address & ~0xffff) >> 4;
    }

    frame->flags |= BIT(8);
}

static void gdbstub_memory_read(const char* payload, int len) {
    int addr_len = -1, size_len = -1;
    uint32_t address = 0, size = 0;

    for (int i = 0; i < len && addr_len == -1; i++)
        if (payload[i] == ',')
            addr_len = i;
    
    if (addr_len == -1) {
        gdbstub_write("E01", 3);
        return;
    }

    size_len = len - addr_len - 1;
    address = hex2bin(payload, addr_len);
    size = hex2bin(payload + addr_len + 1, size_len);

    char response[0x400] = {};
    if (size > 0x200) {
        gdbstub_write("E01", 3);
        return;
    }

    for (uint32_t i = 0; i < size; i++) {
        uint8_t b = 0;
        memcpy_far(&b, 0, (void*)((address + i) & 0xffff), FP_SEG(address + i), 1);
        response[i*2+0] = bin2hex[b >> 4];
        response[i*2+1] = bin2hex[b & 0xf];
    }

    gdbstub_write(response, size*2);
}


static void gdbstub_memory_write(const char* payload, int len) {
    int addr_len = -1, size_len = -1;
    uint32_t address = 0, size = 0;
    const char* data = NULL;

    for (int i = 0; i < len && addr_len == -1; i++)
        if (payload[i] == ',')
            addr_len = i;
    
    if (addr_len == -1) {
        gdbstub_write("E01", 3);
        return;
    }

    for (int i = addr_len + 1; i < len; i++) {
        if (payload[i] == ':') {
            size_len = i - addr_len - 1;
            break;
        }
    }    

    address = hex2bin(payload, addr_len);
    size = hex2bin(payload + addr_len + 1, size_len);
    data = payload + addr_len + 1 + size_len + 1;

    if (address >= 0xc0000 || (address + size) > 0xc0000) {
        gdbstub_write("E01", 3);
        return;
    }

    for (uint32_t i = 0; i < size; i++) {
        uint8_t b = hex2bin(&data[i*2], 2);
        memcpy_far((void*)((address + i) & 0xffff), FP_SEG(address + i), &b, 0, 1);
    }

    gdbstub_write("OK", 2);
}

static void gdbstub_trap(struct irq_frame* frame) {
    if (!gdbstub_initialized) {
        if (frame->vector == 0x3)
            uart_write("INT3 Received!", 15);
        else
            uart_write("TRAP Exception!", 16);

        cli();
        while (1)
            hlt();
    }

    sti();
    
    gdbstub_write("S05", 3);

    char pckt[512];
    int pckt_len = 0;

    while ((pckt_len = gdbstub_receive(pckt, sizeof(pckt))) > 0) {
        char* payload = pckt + 1;
        switch (pckt[0]) {
            case 'g': gdbstub_read_registers(frame); break;
            case 'G': gdbstub_write_registers(payload, frame); break;
            case 'c': gdbstub_continue(payload, pckt_len - 1, frame); break;
            case 's': gdbstub_step(payload, pckt_len - 1, frame); break;
            case 'm': gdbstub_memory_read(payload, pckt_len - 1); break;
            case 'M': gdbstub_memory_write(payload, pckt_len - 1); break;
            case 'p': gdbstub_read_register(payload, pckt_len - 1, frame); break;
            case 'r':
            case 'R': trigger_reset(); break;
            case 'k': trigger_reset(); break;
            // Only one signal ever returned.
            case '?': gdbstub_write("S05", 3); break;
            default: gdbstub_write("", 0); break;
        }
        if (pckt[0] == 'c')
            break;
        if (pckt[0] == 's')
            break;
    }
}

void gdbstub_initialize() {
    gdbstub_initialized = true;
}