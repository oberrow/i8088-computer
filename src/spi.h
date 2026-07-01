/*
 * src/spi.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>

typedef uint8_t spi_device;

// PORTC Upper
#define SPI_MOSI (22)
// PORTC Lower
#define SPI_MISO (19)
// PORTC Upper
#define  SPI_CLK (21)

void spi_initialize();
// cs_gpio should be on PORTB
spi_device spi_initialize_device(uint8_t cs_gpio);
uint8_t spi_tx(uint8_t x);
void spi_pulse(int count);
void spi_select(spi_device tgt);
void spi_deselect(void);