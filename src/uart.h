/*
 * src/uart.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "frame.h"
#include "extra.h"

enum uart_registers {
    UART_IO_BUFFER = 0,
    UART_DIVISOR_LOW_BYTE = 0, // When FIFO_CTRL.DLAB = 1
    UART_IRQ_ENABLE = 1,
    UART_DIVISOR_HIGH_BYTE = 1, // When FIFO_CTRL.DLAB = 1
    UART_INTERRUPT_IDENTIFICATION = 2,
    UART_FIFO_CTRL = 2,
    UART_LINE_CTRL = 3,
    UART_MODEM_CTRL = 4,
    UART_LINE_STATUS = 5,
    UART_MODEM_STATUS = 6,
    UART_SCRATCH = 7,
};

enum parity_bit {
    PARITYBIT_NONE,
    PARITYBIT_ODD = 0b10000,
    PARITYBIT_EVEN = 0b11000,
    PARITYBIT_MARK = 0b10100,
    PARITYBIT_SPACE = 0b11100,
};

enum data_bits {
    FIVE_DATABITS,
    SIX_DATABITS,
    SEVEN_DATABITS,
    EIGHT_DATABITS,
};

enum stop_bits {
    ONE_STOPBIT,
    ONE_HALF_STOPBIT = 0b0100,
    TWO_STOPBITS = 0b0100,
};

int uart_init(int baud_rate, uint8_t stop_bits, uint8_t data_bits, uint8_t parity_bit);
char uart_readb(bool async);
void uart_writeb(char c);
// Can only read upto 512 bytes at once.
// Returns the amount of bytes read.
int uart_read(__far void* buf, int len, bool async);
int uart_write(__far const void* buf, int len);
void uart_irq(struct irq_frame* frame);