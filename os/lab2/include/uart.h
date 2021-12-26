#ifndef __UART_H__
#define __UART_H__
#include <stddef.h>

struct uart_class_ops {
    void (*uart_directly_write)(int8_t c);
    void (*uart_putc)(int8_t c);
    int8_t (*uart_read)();
    void (*uart_interrupt_handler)();
};

struct uart_class_device {
    uint32_t id;
	uint64_t uart_start_addr;
	uint16_t divisor;
    struct uart_class_ops ops;
};
enum uart_device_type {
    UART_QEMU = 0,
    UART_SUNXI = 1,
};

void uart_init();
int8_t uart_read();
void uart_directly_write(int8_t c);
void uart_putc(int8_t c);
void uart_interrupt_handler();
void uart_qemu_init();
void uart_sunxi_init();
#endif /* end of include guard: __UART_H__ */
