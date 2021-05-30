#include <uart.h>
#include <mm.h>
#include <stddef.h>

void uart_init()
{
    volatile uint8_t *base = (uint8_t *)UART_START_ADDRESS;
    uint8_t lcr = (1 << 0) | (1 << 1);
    *(base + 3) = lcr;
    *(base + 2) = 1 << 0;
    *(base + 1) = 1 << 0;

    uint16_t divisor = 592;
    uint8_t divisor_least = divisor & 0xFF;
    uint8_t divisor_most = divisor >> 8;
    *(base + 3) = lcr | (1 << 7);
    *(base + 0) = divisor_least;
    *(base + 1) = divisor_most;
    *(base + 3) = lcr;
}

int8_t uart_read()
{
    volatile int8_t *base = (int8_t *)UART_START_ADDRESS;
    if ((*(base + 5) & 1) == 0) {
        return -1;
    } else {
        return *base;
    }
}
