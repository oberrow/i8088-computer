/*
 * src/i8255.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stdbool.h>
#include <stdint.h>

#include "i8255.h"
#include "io.h"

void i8255_port_mode(uint8_t port, bool direction) {
    if (!bus_info.i8255)
        return;

    uint8_t ctrl_port = inb(bus_info.i8255->base + i8255_ADDR_CTRL) | i8255_IO_MODE;

    switch (port) {
        case i8255_PORTA:
            if (direction == i8255_DIR_INPUT)
                ctrl_port |= (1<<4);
            else
                ctrl_port &= ~(1<<4);
            break;
        case i8255_PORTB:
            if (direction == i8255_DIR_INPUT)
                ctrl_port |= (1<<1);
            else
                ctrl_port &= ~(1<<1);
            break;
        case i8255_PORTC_lower:
            if (direction == i8255_DIR_INPUT)
                ctrl_port |= (1<<0);
            else
                ctrl_port &= ~(1<<0);
            break;
        case i8255_PORTC_upper:
            if (direction == i8255_DIR_INPUT)
                ctrl_port |= (1<<3);
            else
                ctrl_port &= ~(1<<3);
            break;
        default:
            return;
    }

    outb(bus_info.i8255->base + i8255_ADDR_CTRL, ctrl_port);
}

void i8255_write_port(uint8_t port, uint8_t val) {
    if (!bus_info.i8255)
        return;

    uint16_t io = bus_info.i8255->base;
    if (port <= i8255_PORTC_lower) {
        io += port;
    } else {
        io += i8255_ADDR_PORTC;
    }

    outb(io, val);
}

uint8_t i8255_read_port(uint8_t port) {
    if (!bus_info.i8255)
        return 0xff;

    uint16_t io = bus_info.i8255->base;
    if (port <= i8255_PORTC_lower) {
        io += port;
    } else {
        io += i8255_ADDR_PORTC;
    }

    return inb(io);
}

void i8255_write_pin(uint8_t pin, bool val) {
    if (!bus_info.i8255)
        return;

    uint8_t port = pin >> 3;
    uint8_t bit = port & 7;

    uint8_t x = i8255_read_port(port);
    if (!val) {
        x &= ~(1<<bit);
    } else {
        x |= (1<<bit);
    }

    i8255_write_port(port, x);
}

bool i8255_read_pin(uint8_t pin) {
    if (!bus_info.i8255)
        return false;

    uint8_t port = pin >> 3;
    uint8_t bit = port & 7;

    return (i8255_read_port(port) >> bit) & 1;
}