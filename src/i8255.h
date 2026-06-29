/*
 * src/i8255.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

enum {
    i8255_ADDR_PORTA,
    i8255_ADDR_PORTB,
    i8255_ADDR_PORTC,
    i8255_ADDR_CTRL,
};

enum {
    i8255_PORTA,
    i8255_PORTB,
    i8255_PORTC_lower,
    i8255_PORTC_upper,
};

#define i8255_IO_MODE (1<<7)

enum {
    i8255_MODE1,
    i8255_MODE2,
    i8255_MODE3,
};

enum {
    i8255_DIR_OUTPUT,
    i8255_DIR_INPUT,
};

enum {
    i8255_LOW,
    i8255_HIGH,
};

void i8255_port_mode(uint8_t port, bool direction);
void i8255_write_port(uint8_t port, uint8_t val);
uint8_t i8255_read_port(uint8_t port);
void i8255_write_pin(uint8_t pin, bool val);
bool i8255_read_pin(uint8_t pin);