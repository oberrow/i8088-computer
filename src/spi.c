/*
 * src/spi.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "i8255.h"
#include "io.h"
#include "uart.h"

bool spi_exists = false;

void spi_initialize()
{
    if (!bus_info.i8255)
        return;
    if (spi_exists)
        return;
    spi_exists = true;
    i8255_port_mode(i8255_PORTC_lower, i8255_DIR_INPUT);
    i8255_port_mode(i8255_PORTC_upper, i8255_DIR_OUTPUT);
    i8255_port_mode(i8255_PORTB, i8255_DIR_OUTPUT);
    i8255_write_port(i8255_PORTB, 0xff);
}

spi_device spi_initialize_device(uint8_t cs_gpio) {
    if (cs_gpio < 8 || cs_gpio >= 16)
        return 0xff;
    if (!spi_exists)
        return 0xff;
    return 1 << (cs_gpio - 8);
}

// TODO: Rewrite in assembly?
uint8_t spi_tx(uint8_t x) {
    uint8_t resp = 0;
    
    for (int8_t i = 7; i >= 0; i--) {
        i8255_write_pin(SPI_MOSI, !!((x >> i) & 1));
        asm volatile ("nop;nop;":::"memory");
        i8255_write_pin(SPI_CLK, i8255_HIGH);
        
        if (i8255_read_pin(SPI_MISO))
            resp |= (1 << i);

        asm volatile ("nop;nop;":::"memory");
        
        i8255_write_pin(SPI_CLK, i8255_LOW);
        asm volatile ("nop;nop;":::"memory");
    }

    return resp;
}

void spi_pulse(int count) {
    for (int i = count; i >= 0; i--) {
        i8255_write_pin(SPI_MOSI, true);
        asm volatile ("nop;nop;":::"memory");
        i8255_write_pin(SPI_CLK, i8255_HIGH);
        asm volatile ("nop;nop;":::"memory");
        i8255_write_pin(SPI_CLK, i8255_LOW);
        asm volatile ("nop;nop;":::"memory");
    }
}

static bool spi_selected = false;
void spi_select(spi_device tgt) {
    i8255_write_port(i8255_PORTB, ~tgt);
    spi_selected = true;
    asm volatile ("nop;nop;":::"memory");
}

void spi_deselect(void) {
    spi_selected = false;
    i8255_write_port(i8255_PORTB, 0xff);
    asm volatile ("nop;nop;":::"memory");
}