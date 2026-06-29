/*
 * src/io.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "i8255.h"

void outb(uint16_t addr, uint8_t val);
void outw(uint16_t addr, uint16_t val);

void outs(uint16_t addr, const char* str);

uint8_t inb(uint16_t addr);
uint16_t inw(uint16_t addr);

void cli();
void sti();

uint16_t clis();
void resf(uint16_t old_flags); // restore flags
uint16_t get_flags();

void hlt();

void probe_io_bus();
bool reprobe_io_bus();

#define US_IN_CYCLES(us) (((us) - 15 - 2 - 16 - 2 - 12 - 20 - 5) / 17)
void delay_cycles(uint16_t cycles);

struct io_slot {
    uint16_t base;
    uint8_t type;
    uint8_t slot_idx;
    uint8_t irq_line;
    void(*on_detach)(void* userdata);
    void* userdata;
};

#define ROM_SIZE (0x40000)

extern struct sys_bus_info {
    uint32_t ram_size : 20;
    uint8_t active_slot_count : 4;
    uint8_t probe_port_value;
    
    struct io_slot slots[7];

    struct io_slot *pic;
    struct io_slot *uart;
    struct io_slot *lcd;
    struct io_slot *timer;
    struct io_slot *i8255;
} bus_info;

// Reading from an I/O slot that is not returned present by
// the system prob

#define IO_SLOT_ADDRESS(slot) (((slot & 0x7) << 13) | 0x1000)

// Reading one byte from here returns a device class ID 
#define IO_SLOT_CLASS(slot) (((slot & 0x7) << 13))

#define SYSTEM_PROBE_BASE IO_SLOT_ADDRESS(6)

enum {
    // Not a valid type from the motherboard.
    // Simply produced here so the io_slot.type
    // field makes sense.
    DEVICE_CLASS_i8259_PIC = 0xfe,
    DEVICE_CLASS_TIMER = 0x1,
    DEVICE_CLASS_16550_UART,
    DEVICE_CLASS_i8255A_GPIO,
    DEVICE_CLASS_LCD,
};

#define trigger_reset() i8255_write_pin(23, i8255_HIGH)

#ifdef NO_PROBE
extern uint32_t current_tick_ms;
#else
#define current_tick_ms ({\
    uint32_t tick = 0;\
    if (bus_info.timer) {\
        (void)inb(bus_info.timer->base + 4); /* Update RCLK */\
        tick = (uint32_t)inw(bus_info.timer->base + 2) << 16;\
        tick |= inw(bus_info.timer->base);\
    } else {\
        tick = 0xffffffff;\
    }\
    (tick);\
})
#endif

#define delay_ms(ms) do {\
    uint32_t deadline = current_tick_ms + ms;\
    while (current_tick_ms < deadline)\
        hlt();\
} while(0)
