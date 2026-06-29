/*
 * src/sd.h
 *
 * Copyright (c) 2026 Omar Berrow
 *
 * SPI SD Card Driver
*/

#pragma once

#include <stdint.h>
#include "spi.h"
#include "extra.h"
#include "uart.h"

#define SD_SECTOR_SIZE (512U)

typedef struct sd_card {
    spi_device dev;
    uint8_t resv;
    uint32_t sd_card_blk_count;
    bool ccs : 1;
    bool sd_ver_1x : 1;
    bool write_protected : 1;
    // MSB FIELD!!!
    uint8_t csd[16];
} sd_card;

enum {
    SD_R1_IDLE = BIT(0),
    SD_R1_ERASE_RESET = BIT(1),
    SD_R1_ILLEGAL_COMMAND = BIT(2),
    SD_R1_COM_CRC_ERROR = BIT(4),
    SD_R1_ERASE_SEQ_ERROR = BIT(5),
    SD_R1_ADDRESS_ERROR = BIT(6),
};

enum {
    // 2.7-3.6V
    OCR_LEGAL_VOLTAGE_RANGES = 0xFF8000,
    OCR_S18A = BIT_TYPE(24, UL),
    OCR_UHS_II_STATUS = BIT_TYPE(29, UL),
    // Card capacity status
    OCR_CCS = BIT_TYPE(30, UL),
    // Card power up status bit
    OCR_CPUB = BIT_TYPE(31, UL),
};

int sd_initialize(spi_device dev, sd_card* out);
// Flip sign of negative return to get R1 response
// Returns >=0 for success
int16_t sd_read_sector(const sd_card* card, uint32_t lba, void* buf);

// Write error word
enum {
    SD_WRITE_R1_ERR = 0xff,
    SD_WRITE_ERROR = BIT(8),
    SD_WRITE_TIMEOUT = BIT(9),
};

// Returns zero on success, otherwise a positive error word as defined above.
int16_t sd_write_sector(const sd_card* card, uint32_t lba, const void* buf);
int16_t sd_erase_sectors(const sd_card* card, uint32_t start_lba, uint32_t end_lba);