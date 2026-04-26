/*
 * src/uart.c
 *
 * Copyright (c) 2026 Omar Berrow
*/

#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "io.h"
#include "extra.h"
#include "pic.h"
#include "mem.h"

static struct {
    bool initialized;
    char buf[512];
    uint16_t ptr : 9;
    uint16_t in_ptr : 9;
} uart_state;

int uart_init(int baud_rate, uint8_t stop_bits, uint8_t data_bits, uint8_t parity_bit) {
    uint16_t divisor = 0;
    switch (baud_rate) {
        case 50: divisor = 15360; break;
        case 75: divisor = 10240; break;
        case 150: divisor = 5120; break;
        case 200: divisor = 3840; break;
        case 300: divisor = 2560; break;
        case 600: divisor = 1280; break;
        case 1200: divisor = 640; break;
        case 2400: divisor = 320; break;
        case 4800: divisor = 160; break;
        case 9600: divisor = 80; break;
        case 19200: divisor = 40; break;
        case 38400: divisor = 20; break;
        default: return 1;
    }

    // Code taken from OBOS.
    outb(UART_BASE + UART_IRQ_ENABLE, 0);
    outb(UART_BASE + UART_LINE_CTRL, 0x80 /* LINE_CTRL.DLAB */);
    outb(UART_BASE + UART_DIVISOR_LOW_BYTE, divisor & 0xff);
    outb(UART_BASE + UART_DIVISOR_HIGH_BYTE, divisor >> 8);
    outb(UART_BASE + UART_LINE_CTRL, data_bits | stop_bits | parity_bit);
    while (inb(UART_BASE + UART_LINE_STATUS) & BIT(0))
        inb(UART_BASE + UART_IO_BUFFER);
    // Enter loop back mode.
    outb(UART_BASE + UART_MODEM_CTRL, 0x1B /* RTS+Out1+Out2+Loop */);
    outb(UART_BASE + UART_IO_BUFFER, 0xde);
    if (inb(UART_BASE + UART_IO_BUFFER) != 0xde)
        return 1;
    // Enter normal transmission mode.
    outb(UART_BASE + UART_FIFO_CTRL, 0x07 /* FIFO Enabled, IRQ when one byte is received, other flags */);
    outb(UART_BASE + UART_MODEM_CTRL, 0xF /* DTR+RTS+OUT2+OUT1*/);
    outb(UART_BASE + UART_IRQ_ENABLE, 1);

    // UART is on IRQ line 6.
    pic_clear_mask(BIT(6));

    uart_state.initialized = true;

    return 0;
}

char uart_readb(bool async) {
    char c = 0;
    if (uart_read(&c, 1, async) < 1)
        return -1;
    return c;
}
void uart_writeb(char c) {
    uart_write(&c, 1);
}

int uart_read(__far void* buf, int len, bool async) {
    if (len > (int)sizeof(uart_state.buf))
        len = (int)sizeof(uart_state.buf);
    if (async) {
        if ((uart_state.ptr - uart_state.in_ptr) < len)
            len = uart_state.ptr - uart_state.in_ptr;

        if (!len)
            return 0;

        memcpy(buf, &uart_state.buf[uart_state.in_ptr], len);
        uart_state.in_ptr += len;
        if (uart_state.in_ptr == uart_state.ptr)
            uart_state.ptr = uart_state.in_ptr = 0;

        return len;
    }

    // Abort if interrupts are disabled.
    if (get_flags() & BIT(9))
        return -1;

    while ((uart_state.ptr - uart_state.in_ptr) < len)
        hlt();

    memcpy(buf, &uart_state.buf[uart_state.in_ptr], len);
    uart_state.in_ptr += len;
    if (uart_state.in_ptr == uart_state.ptr)
        uart_state.ptr = uart_state.in_ptr = 0;

    return len;
}

int uart_write(__far const void* bufp, int len) {
    __far const char* buf = bufp;
    for (int i = 0; i < len; i++) {
        while (inb(UART_BASE+UART_LINE_STATUS) & BIT(5))
            asm volatile("rep nop"); // technically pause, but that doesn't exist on 8088
        outb(UART_BASE+UART_IO_BUFFER, buf[i]);
    }
    
    return len;
}

void uart_irq(struct irq_frame* frame) {
    (void)frame;
    uint16_t flags = clis();
    while (inb(UART_BASE + UART_LINE_STATUS) & BIT(0)) {
        char data = inb(UART_BASE + UART_IO_BUFFER);
        if (uart_state.ptr < sizeof(uart_state.buf))
            uart_state.buf[uart_state.ptr++] = data;
    }
    resf(flags);
    pic_eoi();
}