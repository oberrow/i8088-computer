/*
 * src/probe.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include "io.h"
#include "extra.h"
#include "mem.h"
#include "uart.h"
#include "pic.h"
#include "lcd.h"

struct sys_bus_info bus_info;

static void slot_create(uint8_t slot, uint8_t irq_line, uint16_t base, uint8_t type) {
    bus_info.slots[slot].slot_idx = slot;
    bus_info.slots[slot].irq_line = irq_line;
    bus_info.slots[slot].base = base;
    bus_info.slots[slot].type = type;
    switch (bus_info.slots[slot].type) {
        case DEVICE_CLASS_16550_UART:
            if (bus_info.uart) break;
            bus_info.uart = &bus_info.slots[slot];
            uart_init(9600, ONE_STOPBIT, EIGHT_DATABITS, PARITYBIT_NONE);
            break;
        case DEVICE_CLASS_LCD:
            if (bus_info.lcd) break;
            bus_info.lcd = &bus_info.slots[slot];
            lcd_init();
            break;
        case DEVICE_CLASS_i8255A_GPIO:
            if (bus_info.i8255) break;
            bus_info.i8255 = &bus_info.slots[slot];
            i8255_port_mode(i8255_PORTC_upper, i8255_DIR_OUTPUT);
            break;
        case DEVICE_CLASS_TIMER:
            if (bus_info.timer) break;
            bus_info.timer = &bus_info.slots[slot];
            // As of right now, there is no handler for the timer IRQ.
#ifdef NO_PROBE
            pic_clear_mask(BIT(bus_info.timer->irq_line));
#endif
            break;
        default: break;
    }
}

static void slot_present(uint8_t slot) {
    slot_create(slot, slot, IO_SLOT_ADDRESS(slot), inb(IO_SLOT_CLASS(slot)));
}

void probe_io_bus() {
#ifdef NO_PROBE
    slot_create(6, 0xff, 0x100, DEVICE_CLASS_i8259_PIC);
    slot_create(0, 0, 0, DEVICE_CLASS_TIMER);
    slot_create(1, 6, 0x200, DEVICE_CLASS_16550_UART);
    slot_create(2, 0xff, 0x400, DEVICE_CLASS_LCD);
    bus_info.ram_size = 0x10000;
    return;
#else
    uint8_t probe = inb(SYSTEM_PROBE_BASE) >> 2;
    
    slot_create(6, 0xff, IO_SLOT_ADDRESS(7), DEVICE_CLASS_i8259_PIC);

    for (int i = 0; i < 6; i++) {
        if (probe & BIT(i)) {
            // An I/O slot is present

            slot_present(i);

            bus_info.active_slot_count++;
        }
    }

    bus_info.ram_size = 0x8000;

    char *ch = NULL;

    while (bus_info.ram_size <= 0xc0000) {
        char cur = 0;
 
        // TODO: Write a byte into this address in case two places in RAM
        // have the same values, but are still separate
        memcpy_far(&cur, 0, (void*)(uint16_t)bus_info.ram_size, bus_info.ram_size >> 4, 1);
        
        // When the address space starts to repeat itself, assume the RAM has ended.
        if (cur == *ch)
            break;

        bus_info.ram_size += 0x8000;
    }

    bus_info.probe_port_value = probe;
#endif
}

static bool probe_slot(uint8_t slot) {
    uint8_t type = inb(IO_SLOT_CLASS(slot));
    if (type == bus_info.slots[slot].type)
        return false;

    if (bus_info.slots[slot].on_detach)
        bus_info.slots[slot].on_detach(bus_info.slots[slot].userdata);
    bus_info.slots[slot].on_detach = NULL;
    slot_create(slot, slot, IO_SLOT_ADDRESS(slot), type);

    return true;
}

bool reprobe_io_bus()
{
    uint8_t probe = inb(SYSTEM_PROBE_BASE) >> 2;
    bool status = false;
    
    // PIC is always active.
    bus_info.active_slot_count = 1;

    for (uint8_t i = 0; i < 6; i++) {
        if (probe & BIT(i)) {
            if (probe_slot(i)){
                status = true;
            }
            bus_info.active_slot_count++;
        }
    }

    bus_info.probe_port_value = probe;
    return status;
}