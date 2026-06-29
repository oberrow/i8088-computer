/*
 * src/spi.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>

typedef uint8_t spi_device;

// PORTC Upper
#define SPI_MOSI (23)
// PORTC Lower
#define SPI_MISO (19)
// PORTC Upper
#define  SPI_CLK (22)

void spi_initialize();
// cs_gpio should be on PORTB
spi_device spi_initialize_device(uint8_t cs_gpio);
uint8_t spi_tx(spi_device tgt, uint8_t x);