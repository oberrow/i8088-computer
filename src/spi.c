/*
 * src/spi.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#ifndef SPI_BITBANG
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "io.h"
#include "uart.h"
#include "frame.h"
#include "pic.h"

// Slave Data Register
#define SDR 0x0
// Status & Control Register
#define SCR 0x1

// Read-only bit
#define SCR_BUSY BIT(7)

// Write-only bits
#define SCR_LOOPBACK BIT(7)

// RW Bits
#define SCR_FLOW_CTRL BIT(0)
#define SCR_INTM BIT(1)
#define SCR_CS_MASK 0x7c

static uint8_t base = 0;

static void spi_irq(struct irq_frame* frame) {
    pic_eoi();
}

void spi_initialize() {
    if (!bus_info.spi)
        return;
    base = bus_info.spi->base;
    irq_handlers[bus_info.spi->irq_line + 0x20] = spi_irq;
    uart_write("Initialized SPI controller\n", 60);
}

// cs_idx is otherwise a value from 0 to 4 for chip select
spi_device spi_initialize_device(uint8_t cs_idx) {
    if (cs_idx >= 5)
        return (spi_device)-1;
    return 1 << (cs_idx+2);
}

uint8_t spi_tx(uint8_t x) {
    cli();
    uint8_t scr = inb(base+SCR);
    outb(base + SDR, x);
    outb(base + SCR, scr |= SCR_FLOW_CTRL);
    sti();
    while (inb(base + SCR) & SCR_BUSY)
        hlt();
    outb(base + SCR, scr &= ~SCR_FLOW_CTRL);
    return inb(base + SDR);
}

void spi_pulse(int count) {
    count = (count + 7) & ~7;

    for (int i = 0; i < count; i++)
        spi_tx(0xff);
}

void spi_select(spi_device tgt) {
    outb(base + SCR, tgt);
}

void spi_deselect(void) {
    outb(base + SCR, 0);
}

#endif