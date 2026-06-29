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

static void uart_on_detach(void* x) {
    (void)(x);

    uart_state.initialized = false;
    uart_state.in_ptr = 0;
    uart_state.ptr = 0;
    memset(uart_state.buf, 0, 512);
}

int uart_init(int baud_rate, uint8_t stop_bits, uint8_t data_bits, uint8_t parity_bit) {
    if (!bus_info.uart)
        return 1;

    uint16_t divisor = 115200 / baud_rate;
    if (divisor == 0)
        return 1;

    // Code taken from OBOS.
    outb(bus_info.uart->base + UART_IRQ_ENABLE, 0);
    outb(bus_info.uart->base + UART_LINE_CTRL, 0x80 /* LINE_CTRL.DLAB */);
    outb(bus_info.uart->base + UART_DIVISOR_LOW_BYTE, divisor & 0xff);
    outb(bus_info.uart->base + UART_DIVISOR_HIGH_BYTE, divisor >> 8);
    outb(bus_info.uart->base + UART_LINE_CTRL, data_bits | stop_bits | parity_bit);
    while (inb(bus_info.uart->base + UART_LINE_STATUS) & BIT(0))
        inb(bus_info.uart->base + UART_IO_BUFFER);
    // Enter loop back mode.
    outb(bus_info.uart->base + UART_MODEM_CTRL, 0x1B /* RTS+Out1+Out2+Loop */);
    outb(bus_info.uart->base + UART_IO_BUFFER, 0xde);
    if (inb(bus_info.uart->base + UART_IO_BUFFER) != 0xde)
        return 1;
    // Enter normal transmission mode.
    outb(bus_info.uart->base + UART_FIFO_CTRL, 0x07 /* FIFO Enabled, IRQ when one byte is received, other flags */);
    outb(bus_info.uart->base + UART_MODEM_CTRL, 0xF /* DTR+RTS+OUT2+OUT1*/);
    outb(bus_info.uart->base + UART_IRQ_ENABLE, 1);

    pic_clear_mask(BIT(bus_info.uart->irq_line));
    irq_handlers[bus_info.uart->irq_line + 0x20] = uart_irq;

    uart_state.initialized = true;

    bus_info.uart->on_detach = uart_on_detach;

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

int uart_read(void* buf, int len, bool async) {
    if (!bus_info.uart)
        return -1;

    if (len > (int)sizeof(uart_state.buf))
        len = (int)sizeof(uart_state.buf);
    if (async) {
        if ((uart_state.ptr - uart_state.in_ptr) < len)
            len = uart_state.ptr - uart_state.in_ptr;

        if (!len)
            return 0;

        memcpy_far(FP_OFF(buf), 0, FP_OFF(&uart_state.buf[uart_state.in_ptr]), 0, len);
        uart_state.in_ptr += len;
        if (uart_state.in_ptr == uart_state.ptr)
            uart_state.ptr = uart_state.in_ptr = 0;

        return len;
    }

    // Abort if interrupts are disabled.
    if (~get_flags() & BIT(9))
        return -1;
    
    while ((uart_state.ptr - uart_state.in_ptr) < len)
        hlt();

    memcpy_far(FP_OFF(buf), 0, FP_OFF(&uart_state.buf[uart_state.in_ptr]), 0, len);
    uart_state.in_ptr += len;
    if (uart_state.in_ptr == uart_state.ptr)
        uart_state.ptr = uart_state.in_ptr = 0;

    return len;
}

int uart_write(const void* bufp, int len) {
    if (!bus_info.uart)
        return -1;

    __far const char* buf = bufp;
    for (int i = 0; i < len; i++) {
        // while (inb(bus_info.uart->base+UART_LINE_STATUS) & BIT(5))
        //     asm volatile("rep nop"); // technically pause, but that doesn't exist on 8088
        outb(bus_info.uart->base+UART_IO_BUFFER, buf[i]);
    }
    
    return len;
}

void uart_irq(struct irq_frame* frame) {
    (void)frame;
    uint16_t flags = clis();
    while (inb(bus_info.uart->base + UART_LINE_STATUS) & BIT(0)) {
        char data = inb(bus_info.uart->base + UART_IO_BUFFER);
        if (uart_state.ptr < sizeof(uart_state.buf))
            uart_state.buf[uart_state.ptr++] = data;
    }
    resf(flags);
    pic_eoi();
}