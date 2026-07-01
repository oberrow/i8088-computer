/*
* src/sd.c
*
* Copyright (c) 2026 Omar Berrow
*
* SPI SD Card Driver
*/

#include <stdint.h>
#include <stdbool.h>
#include "sd.h"
#include "io.h"
#include "spi.h"
#include "extra.h"

extern uint8_t getCRC7(uint8_t *message, int length);

static void sd_cmd(uint8_t cmd_idx, uint32_t argument) {
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
    cmd_bytes[5] |= crc7<<1;
    for (int8_t i = 0; i < 6; i++)
        spi_tx(cmd_bytes[i]);
}

static void sd_read_resp(uint8_t* buf, int resp_len, bool blk) {
    for (int i = 0; i < resp_len; i++) {
        buf[i] = spi_tx(0xff);
        if (!blk) {
            for (int j = 10; j >= 0 && buf[i] == 0xff; j--)
                buf[i] = spi_tx(0xff); // Dummy bytes to read response 
        }
    }
}

static uint8_t sd_read_r1(void) {
    uint8_t r1 = 0;
    sd_read_resp(&r1, 1, false);
    return r1;
}

static void sd_discard(uint8_t count) {
    for (int i = 0; i < count; i++) {
        uint8_t ch = spi_tx(0xff);
        for (int j = 10; j >= 0 && ch == 0xff; j--)
            ch = spi_tx(0xff); // Dummy bytes to read response
    }
}
static __attribute__((alias("sd_discard"))) uint8_t sd_read_1b(void);

static void sd_acmd(void) {
    sd_cmd(55, 0);
    sd_discard(1);
}

static uint16_t crc16(const uint8_t *data, int len);

int sd_initialize(spi_device dev, sd_card* out) {
    /* "The host shall supply power to the card so that the voltage is
    * reached to VDD_min within 250ms and start to supply at least 74
    * SD clocks to the SD card with keeping CMD line to high. In case
    * of SPI mode, CS shall be held to high during 74 clock cycles"
    * (SD Specification, 6.4.1.1)
    */
    spi_pulse(80);
    
    spi_select(dev);
    
    // Do SD Card initialization sequence.
    
    // CMD0
    sd_cmd(0, 0);
    
    uint8_t r1 = 0;
    
    for (int8_t i = 0; i < 10 && (r1 = sd_read_r1()) == 0xff; i++)
        ;
    
    if (~r1 & SD_R1_IDLE || r1 == 0xff) {
        spi_deselect();
        return -1;
    }
    bool CMD8_supported = false;
    uint8_t r7[4] = {};
    
    for (int i = 0; i < 3; i++) {
        // CMD8
        // Supply voltage: 3.3V
        // Check pattern: 0xaa
        sd_cmd(8, 0x1aa);
        
        r1 = sd_read_r1();
        if (r1 & SD_R1_COM_CRC_ERROR)
            continue;
        if (r1 & SD_R1_ILLEGAL_COMMAND) {
            break; // CMD8 not supported.
        }
        
        CMD8_supported = true;
        
        sd_read_resp(r7, 4, true);
        break;
    }
    
    if (CMD8_supported) {
        if (r7[3] != 0xaa || (r7[2] & 0xf) != 0x1) {
            spi_deselect();
            return -1;
        }
        out->ccs = true;
        out->sd_ver_1x = false;
    } else {
        out->sd_ver_1x = false;
        out->ccs = false;
    }
    
    sd_cmd(58, 0);
    r1 = sd_read_r1();
    if (r1 & SD_R1_ILLEGAL_COMMAND) {
        spi_deselect();
        return -1;
    }
    
    uint8_t r5[4] = {};
    sd_read_resp(r5, 4, true);
    
    bool card_ready = false;
    
    for (int i = 0; i < 5; i++) {
        sd_acmd();
        
        // ACMD41
        sd_cmd(41, out->ccs ? BIT_TYPE(30, UL) : 0);
        
        r1 = sd_read_r1();
        if (r1 & SD_R1_IDLE)
            continue;
        if (r1 & SD_R1_ILLEGAL_COMMAND)
            return -1;
        
        card_ready = true;
        break;
    }
    
    if (!card_ready) {
        spi_deselect();
        return -1;
    }
    
    sd_cmd(58, 0);
    r1 = sd_read_r1();
    if (r1 & SD_R1_ILLEGAL_COMMAND) {
        spi_deselect();
        return -1;
    }
    
    sd_read_resp(r5, 4, true);
    
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
        sd_cmd(16, SD_SECTOR_SIZE);
        r1 = sd_read_r1();
        if (r1 != 0) {
            spi_deselect();
            return -1;
        }
    }
    
    // Get SD card size
    sd_cmd(9, 0);
    r1 = sd_read_r1();
    if (r1 != 0) {
        spi_deselect();
        return -1;
    }
    
    uint8_t blk_tok = 0;
    while (blk_tok != 0xfe)
        blk_tok = sd_read_1b();
    
    sd_read_resp(out->csd, 16, true);
    
    sd_discard(2);
    
    uint64_t size = 0;
    
    if (((out->csd[0] >> 6) & 0b11) == 0) {
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
        size = (c_size+1) * 524288UL;
    }
    out->sd_card_blk_count = size / 512UL;
    out->write_protected = (out->csd[14] >> 5) & BIT(0);
    
    out->dev = dev;

    spi_deselect();
    
    return 0;
}

int16_t sd_read_sector(const sd_card* card, uint32_t lba, void* buf) {
    spi_select(card->dev);

    uint32_t arg = card->ccs ? lba : lba*SD_SECTOR_SIZE;
    sd_cmd(17, arg);
    uint8_t r1 = sd_read_r1();
    if (r1 != 0) {
        spi_deselect();
        return -r1;
    }
    
    uint8_t blk_tok = 0;
    while (blk_tok != 0xfe)
        blk_tok = sd_read_1b();
    
    sd_read_resp(buf, SD_SECTOR_SIZE, true);
    
    uint16_t crc16h = crc16((const uint8_t*)buf, SD_SECTOR_SIZE);
    uint8_t crc_high = sd_read_1b();
    uint8_t crc_low  = sd_read_1b();
    uint16_t crc16d = (crc_high << 8) | crc_low;
    
    spi_deselect();
    
    if (crc16h != crc16d)
        return -SD_R1_COM_CRC_ERROR;
    
    return 0;
}

int16_t sd_write_sector(const sd_card* card, uint32_t lba, const void* vbuf) {
    spi_select(card->dev);

    uint32_t arg = card->ccs ? lba : lba*SD_SECTOR_SIZE;
    const uint8_t *buf = vbuf;
    uint16_t crc16h = crc16(buf, SD_SECTOR_SIZE);
    
    sd_cmd(24, arg);
    
    uint8_t r1 = sd_read_r1();
    if (r1 != 0) {
        spi_deselect();
        return r1;
    }
    
    spi_tx(0xfe);
    for (unsigned i = 0; i < SD_SECTOR_SIZE; i++)
        spi_tx(buf[i]);
    spi_tx(crc16h >> 8);
    spi_tx(crc16h & 0xff);
    
    uint8_t data_resp_tok = 0;
    for (int i = 0; i < 10 && ((data_resp_tok & 0b10001) != 0b1); i++)
        data_resp_tok = sd_read_1b();
    uint8_t status = (data_resp_tok >> 1) & 0x7;
    switch (status) {
        case 0b010: break;
        case 0b101: spi_deselect(); return -SD_R1_COM_CRC_ERROR;
        case 0b110: spi_deselect(); return -SD_WRITE_ERROR; // Write error.
        default: spi_deselect(); return -SD_WRITE_ERROR; 
    }
    
    // Poll card
    uint32_t deadline = current_tick_ms + 5000;
    uint8_t b = 0;
    do {
        sd_read_resp(&b, 1, true);
        if (b == 0) 
            break;
    } while (current_tick_ms < deadline);

    spi_deselect();
    
    if (b == 0x00)
        return 0;

    return SD_WRITE_TIMEOUT;
}

int16_t sd_erase_sectors(const sd_card* card, uint32_t start_lba, uint32_t end_lba) {
    uint8_t r1 = 0;

    spi_select(card->dev);
    
    // ERASE_WR_BLK_START_ADDR
    sd_cmd(32, start_lba);
    r1 = sd_read_r1();
    if (r1 != 0) {
        spi_deselect();
        return r1;
    }
    // ERASE_WR_BLK_END_ADDR
    sd_cmd(33, end_lba);
    r1 = sd_read_r1();
    if (r1 != 0) {
        spi_deselect();
        return r1;
    }
    
    // ERASE
    sd_cmd(38, 0);
    r1 = sd_read_r1();
    if (r1 != 0) {
        spi_deselect();
        return r1;
    }
    
    // Poll card
    uint32_t deadline = current_tick_ms + 5000;
    uint8_t b = 0;
    do {
        sd_read_resp(&b, 1, true);
        if (b == 0) 
            break;
    } while (current_tick_ms < deadline);

    spi_deselect();
    
    if (b == 0x00)
        return 0;

    return SD_WRITE_TIMEOUT;
}

// Magic CRC16 table...
static const uint16_t crc16_sd_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

static uint16_t crc16(const uint8_t *data, int len) {
    uint16_t crc = 0x0000;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t index = (uint8_t)((crc >> 8) ^ data[i]);
        crc = (crc << 8) ^ crc16_sd_table[index];
    }
    return crc;
}
