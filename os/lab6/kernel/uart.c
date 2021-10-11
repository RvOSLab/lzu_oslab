#include <uart.h>
#include <mm.h>
#include <stddef.h>

void uart_init()
{
    volatile uint8_t *base = (uint8_t *)UART_START_ADDRESS;
    *(base + 0x08) = 1 << 0; /* FIFO Enable */
    *(base + 0x04) = 1 << 0; /* Enable receiver interrupt */
}

int8_t uart_read()
{
    volatile int8_t *base = (int8_t *)UART_START_ADDRESS;
    if ((*(base + 0x1F) & 4) == 0) {
        return -1;
    } else {
        return *(base + UART_THR);
    }
}

void uart_write(int8_t c)
{
    volatile int8_t *base = (int8_t *)UART_START_ADDRESS;
    while ((*(base + 0x1F) & 2) == 0);
    *(base + UART_THR) = c;
}