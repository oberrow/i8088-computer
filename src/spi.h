/*
 * src/spi.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>

typedef uint8_t spi_device;

#ifdef SPI_BITBANG
// PORTC Upper
#define SPI_MOSI (22)
// PORTC Lower
#define SPI_MISO (19)
// PORTC Upper
#define  SPI_CLK (21)
#endif

void spi_initialize();
// cs_idx should be on PORTB if SPI_BITBANG is defined
// cs_idx is otherwise a value from 0 to 4 for chip select
spi_device spi_initialize_device(uint8_t cs_idx);
uint8_t spi_tx(uint8_t x);
void spi_pulse(int count);
void spi_select(spi_device tgt);
void spi_deselect(void);