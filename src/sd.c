/*
 * src/sd.c
 *
 * Copyright (c) 2026 Omar Berrow
 *
 * SPI SD Card Driver
*/

#include <stdint.h>
#include <stdbool.h>
#include "io.h"
#include "spi.h"
#include "sd.h"
#include "spi.h"
#include "extra.h"

uint8_t getCRC7(uint8_t *message, int length);

static void sd_cmd(spi_device dev, uint8_t cmd_idx, uint32_t argument) {
    uint8_t crc7 = 0;
    uint8_t cmd_bytes[6] = {
        cmd_idx | 0x40,
        (argument >> 24) & 0xff,
        (argument >> 16) & 0xff,
        (argument >> 8) & 0xff,
        (argument >> 0) & 0xff,
        BIT(0),
    };
    crc7 = getCRC7(cmd_bytes, 5);
    cmd_bytes[5] |= crc7;

    for (int8_t i = 0; i < 6; i++)
        spi_tx(dev, cmd_bytes[i]);
}

static void sd_read_resp(spi_device dev, uint8_t* buf, int resp_len) {
    for (int i = 0; i < resp_len; i++)
        buf[i] = spi_tx(dev, 0xff); // Dummy bytes to read response
}

static void sd_discard(spi_device dev, uint8_t count) {
    for (int i = 0; i < count; i++)
        spi_tx(dev, 0xff); // Dummy bytes to read response
}

static uint8_t sd_read_r1(spi_device dev) {
    uint8_t r1 = 0;
    sd_read_resp(dev, &r1, 1);
    return r1;
}
static __attribute__((alias("sd_read_r1"))) uint8_t sd_read_1b(spi_device dev);

static void sd_acmd(spi_device dev) {
    sd_cmd(dev, 55, 0);
    sd_discard(dev, 1);
}

static uint16_t crc16(const uint8_t *data, int len);

int sd_initialize(spi_device dev, sd_card* out) {
    /* "The host shall supply power to the card so that the voltage is
     * reached to VDD_min within 250ms and start to supply at least 74
     * SD clocks to the SD card with keeping CMD line to high. In case
     * of SPI mode, CS shall be held to high during 74 clock cycles"
     * (SD Specification, 6.4.1.1)
     */
    spi_pulse(dev, 74);
    
    // Do SD Card initialization sequence.
    
    // CMD0
    sd_cmd(dev, 0, 0);

    uint8_t r1 = 0;
    
    for (int8_t i = 0; i < 10 && ~(r1 = sd_read_r1(dev)) & SD_R1_IDLE; i++)
        ;

    if (~r1 & SD_R1_IDLE)
        return -1;

    bool CMD8_supported = false;
    uint8_t r7[4] = {};

    for (int i = 0; i < 3; i++) {
        // CMD8
        // Supply voltage: 3.3V
        // Check pattern: 0xaa
        sd_cmd(dev, 8, 0x1aa);

        r1 = sd_read_r1(dev);
        if (r1 & SD_R1_COM_CRC_ERROR)
            continue;
        if (r1 & SD_R1_ILLEGAL_COMMAND)
            break; // CMD8 not supported.

        CMD8_supported = true;

        sd_read_resp(dev, r7, 4);
        break;
    }

    if (CMD8_supported) {
        if (r7[3] != 0xaa || (r7[2] & 0xf) != 0x1)
            return -1;
        out->ccs = true;
        out->sd_ver_1x = false;
    } else {
        out->sd_ver_1x = false;
        out->ccs = false;
    }

    sd_cmd(dev, 58, 0);
    r1 = sd_read_r1(dev);
    if (r1 & SD_R1_ILLEGAL_COMMAND)
        return -1;
    
    uint8_t r5[4] = {};
    sd_read_resp(dev, r5, 4);

    bool card_ready = false;

    for (int i = 0; i < 5; i++) {
        sd_acmd(dev);

        // ACMD41
        sd_cmd(dev, 41, out->ccs ? BIT_TYPE(30, UL) : 0);

        r1 = sd_read_r1(dev);
        if (r1 & SD_R1_IDLE)
            continue;
        if (r1 & SD_R1_ILLEGAL_COMMAND)
            return -1;

        card_ready = true;
        break;
    }

    if (!card_ready)
        return -1;

    sd_cmd(dev, 58, 0);
    r1 = sd_read_r1(dev);
    if (r1 & SD_R1_ILLEGAL_COMMAND)
        return -1;
    
    sd_read_resp(dev, r5, 4);

    uint32_t ocr = 0;
    ocr |=  (uint32_t)(r5[3]) << 0;
    ocr |=  (uint32_t)(r5[2]) << 8;
    ocr |= (uint32_t)(r5[1]) << 16;
    ocr |= (uint32_t)(r5[0]) << 24;
    
    if (ocr & OCR_CCS)
        out->ccs = true;
    else
        out->ccs = false;

    if (!out->ccs) {
        // Set sector size
        sd_cmd(dev, 16, SD_SECTOR_SIZE);
        r1 = sd_read_r1(dev);
        if (r1 != 0)
            return -1;
    }

    // Get SD card size
    sd_cmd(dev, 9, 0);
    r1 = sd_read_r1(dev);
    if (r1 != 0)
        return -1;
    
    sd_read_resp(dev, out->csd, 16);

    uint64_t size = 0;

    if ((out->csd[0] & 0b11) == 0) {
        uint64_t c_size = out->csd[8] >> 6;
        c_size |= ((uint16_t)out->csd[7]) << 2;
        c_size |= ((uint16_t)out->csd[6] & 0b11) << 10;
    
        uint64_t c_size_mult = out->csd[10] >> 7;
        c_size_mult |= (out->csd[9] & 0b11) << 1;

        uint64_t read_bl_len = out->csd[5] & 0xf;

        uint32_t total_blocks = (c_size + 1) * (1<<(c_size_mult+2));
        uint32_t block_size_bytes = (1<<read_bl_len);
        size = total_blocks * block_size_bytes;
    } else {
        uint64_t c_size = out->csd[9];
        c_size |= (uint64_t)out->csd[8] << 8;
        c_size |= (uint64_t)(out->csd[7] & 0x7F) << 16;
        size = c_size * 524288UL;
    }
    out->sd_card_blk_count = size / 512;
    out->write_protected = (out->csd[14] >> 5) & BIT(0);

    out->dev = dev;

    return 0;
}

int16_t sd_read_sector(const sd_card* card, uint32_t lba, void* buf) {
    uint32_t arg = card->ccs ? lba : lba*SD_SECTOR_SIZE;
    sd_cmd(card->dev, 17, arg);
    uint8_t r1 = sd_read_r1(card->dev);
    if (r1 != 0)
        return -r1;

    uint8_t blk_tok = 0;
    while (blk_tok != 0xfe)
        blk_tok = sd_read_1b(card->dev);

    sd_read_resp(card->dev, buf, SD_SECTOR_SIZE);

    uint16_t crc16d = 0, crc16h = crc16(buf, SD_SECTOR_SIZE);
    sd_read_resp(card->dev, (void*)&crc16d, 2);
    crc16d = __builtin_bswap16(crc16d);
    if (crc16h != crc16d)
        return -SD_R1_COM_CRC_ERROR;

    return 0;
}

int16_t sd_write_sector(const sd_card* card, uint32_t lba, const void* vbuf) {
    uint32_t arg = card->ccs ? lba : lba*SD_SECTOR_SIZE;
    const uint8_t *buf = vbuf;
    uint16_t crc16h = crc16(buf, SD_SECTOR_SIZE);

    sd_cmd(card->dev, 24, arg);

    uint8_t r1 = sd_read_r1(card->dev);
    if (r1 != 0)
        return r1;

    spi_tx(card->dev, 0xfe);
    for (unsigned i = 0; i < SD_SECTOR_SIZE; i++)
        spi_tx(card->dev, buf[i]);
    spi_tx(card->dev, crc16h >> 8);
    spi_tx(card->dev, crc16h & 0xff);

    uint8_t data_resp_tok = 0;
    for (int i = 0; i < 10 && ((data_resp_tok & 0b10001) != 0b1); i++)
        data_resp_tok = sd_read_1b(card->dev);
    uint8_t status = (data_resp_tok >> 1) & 0x7;
    switch (status) {
        case 0b010: break;
        case 0b101: return -SD_R1_COM_CRC_ERROR;
        case 0b110: return -SD_WRITE_ERROR; // Write error.
        default: return -SD_WRITE_ERROR; 
    }

    // Poll card
    uint32_t deadline = current_tick_ms + 1000;
    while (current_tick_ms < deadline) {
        if (sd_read_1b(card->dev) == 0xff) 
            break;
    }
    
    if (sd_read_1b(card->dev) == 0xff)
        return 0;
    return SD_WRITE_TIMEOUT;
}

int16_t sd_erase_sectors(const sd_card* card, uint32_t start_lba, uint32_t end_lba) {
    uint8_t r1 = 0;

    // ERASE_WR_BLK_START_ADDR
    sd_cmd(card->dev, 32, start_lba);
    r1 = sd_read_r1(card->dev);
    if (r1 != 0)
        return r1;
    // ERASE_WR_BLK_END_ADDR
    sd_cmd(card->dev, 33, end_lba);
    r1 = sd_read_r1(card->dev);
    if (r1 != 0)
        return r1;
    
    // ERASE
    sd_cmd(card->dev, 38, 0);
    r1 = sd_read_r1(card->dev);
    if (r1 != 0)
        return r1;

    // Poll card
    uint32_t deadline = current_tick_ms + 10000 ;
    while (current_tick_ms < deadline) {
        if (sd_read_1b(card->dev) == 0xff) 
            break;
    }
    
    if (sd_read_1b(card->dev) == 0xff)
        return 0;
    return SD_WRITE_TIMEOUT;
}

// Magic CRC16 table...
static const uint16_t crc16_sd_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x9718, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CC0, 0x0CE1,
    0xEFAC, 0xFF8D, 0xCFEE, 0xDFCF, 0xAF28, 0xBF09, 0x8F6A, 0x9F4B,
    0x6E24, 0x7E05, 0x4E66, 0x5E47, 0x2EA0, 0x3E81, 0x0E02, 0x1E23
};

static uint16_t crc16(const uint8_t *data, int len) {
    uint16_t crc = 0x0000;

    for (int i = 0; i < len; i++) {
        uint8_t index = (uint8_t)((crc >> 8) ^ data[i]);
        crc = (crc << 8) ^ crc16_sd_table[index];
    }
    return crc;
}