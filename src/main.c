/*
 * src/main.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "io.h"
#include "mem.h"
#include "pic.h"
#include "sd.h"
#include "spi.h"
#include "uart.h"
#include "extra.h"
#include "sd.h"
#include "mmap.h"

sd_card sd_cards[2];
// Bitfield
uint8_t sd_cards_present;

extern char __data_loadaddr, __data_start, __data_end;

extern void gdbstub_initialize();
// #define GDBSTUB 1

void *memcpy_far(void *dest, uint16_t es, const void *src, uint16_t ds, size_t len);

void int2hex(char* out, int len, uint32_t x) {
    extern char bin2hex[16];
    int i = 0;
    do {
        out[i] = bin2hex[x & 0xf];
        x >>= 4;
    } while(i++ < len && x);
    out[i] = 0;
    for (int x = 0, y = i-1; x < y; x++, y--) {
        char c = out[x];
        out[x] = out[y];
        out[y] = c;
    }
}

__attribute__((section(".btldr"))) uint8_t sector[512];

struct boot_table {
    void* mmap;
    void* io_map;
    int mmap_len;
    int io_map_len;
    uint8_t boot_device;
    int(*read_sector)(uint8_t device, uint32_t lba, void* buf);
    int(*write_sector)(uint8_t device, uint32_t lba, const void* buf);
};

extern __attribute__((noreturn)) void btldr_handoff(struct boot_table *tbl);

extern char stack_bottom[];
extern char stack_top[];
struct mmap_entry mmap[6] = {};
int mmap_length = 0;
struct mmap_entry io_map[8] = {};
int io_map_length = 0;

int read_sector(uint8_t device, uint32_t lba, void* buf) {
    if (device > 8 || ~sd_cards_present & BIT(device))
        return -1;
    
    return sd_read_sector(&sd_cards[device], lba, buf) >= 0 ? 0 : -1;
}

int write_sector(uint8_t device, uint32_t lba, const void* buf) {
    if (device > 8 || ~sd_cards_present & BIT(device))
        return -1;

    return sd_write_sector(&sd_cards[device], lba, buf) == 0 ? 0 : -1;
}

struct boot_table boot_tbl = {
    .mmap = mmap,
    .io_map = io_map,
    .read_sector = read_sector,
    .write_sector = write_sector
};

static void memory_map_initialize();
static void io_map_initialize();

__attribute__((noreturn)) void entry() {
    memcpy_far(&__data_start, 0x0000, &__data_loadaddr, CODE_SEGMENT, &__data_end - &__data_start);
    ivt_init();

    sti();

    probe_io_bus();
    
#if GDBSTUB
    gdbstub_initialize();
    BP();
#else
    char hex_string[6] = {};
    int2hex(hex_string, 5, bus_info.ram_size);

    uart_write("Initialized and probed motherboard.\n0x", 38);
    uart_write(hex_string, strlen(hex_string));
    uart_write(" bytes of RAM detected\nDetected ", 32);
    uart_writeb(bus_info.active_slot_count + '0');
    uart_write(" active I/O slots\n", 18);
#endif

    // Initialize SPI
    spi_initialize();
    
    memory_map_initialize();
    io_map_initialize();

    if (sd_initialize(spi_initialize_device(8), &sd_cards[0]) == 0)
        sd_cards_present |= BIT(0);
    if (sd_initialize(spi_initialize_device(9), &sd_cards[1]) == 0)
        sd_cards_present |= BIT(1);

    int boot_idx = -1;
    for (int i = 0; i < 8; i++) {
        if (~sd_cards_present & BIT(i))
            continue;

        sd_read_sector(&sd_cards[i], 0, sector);
        if (sector[0xfe] == 0x88 && sector[0xff] == 0x80) {
            boot_idx = i;
            break;
        }
    }

    if (boot_idx == -1) {
#if !GDBSTUB
        uart_write("No boot devices found! Booting into firmware shell.\n", 52);
#else
        BP();
#endif
        while (1)
            hlt();
    }
    
    boot_tbl.boot_device = boot_idx;
    boot_tbl.io_map = io_map;
    boot_tbl.io_map_len = io_map_length;
    boot_tbl.mmap = mmap;
    boot_tbl.mmap_len = mmap_length;

#if GDBSTUB
    BP();
#endif

    pic_mask(0xff);

    btldr_handoff(&boot_tbl);
}

static void memory_map_initialize() {
    mmap[mmap_length++] = (struct mmap_entry){
        .base = 0,
        .length = (uint32_t)&stack_bottom,
        .type = MEMORY_FIRMWARE_RECLAIMABLE
    };
    mmap[mmap_length++] = (struct mmap_entry){
        .base = (uint32_t)&stack_bottom,
        .length = 0xfe00 - (uint32_t)&stack_bottom,
        .type = MEMORY_USABLE
    };
    mmap[mmap_length++] = (struct mmap_entry){
        .base = 0xfe00,
        .length = 0x100,
        .type = MEMORY_BTLDR
    };
    
    if (bus_info.ram_size > 0x10000) {
        mmap[mmap_length++] = (struct mmap_entry){
            .base = 0x10000,
            .length = bus_info.ram_size - 0x10000,
            .type = MEMORY_USABLE
        };
    }

    mmap[mmap_length++] = (struct mmap_entry){
        .base = bus_info.ram_size,
        .length = ((uint32_t)CODE_SEGMENT << 4) - bus_info.ram_size,
        .type = MEMORY_RESERVED
    };
    mmap[mmap_length++] = (struct mmap_entry){
        .base = ((uint32_t)CODE_SEGMENT << 4UL),
        .length = ROM_SIZE,
        .type = MEMORY_FLASH
    };
}
static void io_map_initialize() {
    for (int i = 0; i < 7; i++) {
        if (!bus_info.slots[i].type)
            continue;
        io_map[io_map_length].base = bus_info.slots[i].base;
#ifdef NO_PROBE
        io_map[io_map_length].length = 0x100;
#else
        io_map[io_map_length].length = 0x1000;
#endif
        io_map[io_map_length++].type = bus_info.slots[i].type;
    }
#ifndef NO_PROBE
    io_map[io_map_length].base = SYSTEM_PROBE_BASE;
    io_map[io_map_length].length = 0x1000;
    io_map[io_map_length].type = DEVICE_CLASS_PROBE;
#endif
}

void outs(uint16_t addr, const char* str) {
    while (*str)
        outb(addr, *str++);
}