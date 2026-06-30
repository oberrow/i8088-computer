/*
 * src/mmap.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>

enum {
    MEMORY_RESERVED,
    MEMORY_USABLE,
    MEMORY_FLASH,
    MEMORY_BTLDR,
    MEMORY_FIRMWARE_RECLAIMABLE,
};

struct mmap_entry {
    uint32_t base;
    uint32_t length;
    uint8_t type;
};

struct iomap_entry {
    uint16_t base;
    uint16_t length;
    // See DEVICE_CLASS_* in io.h
    uint8_t type;
};