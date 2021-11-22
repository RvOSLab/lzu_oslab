#include <uart.h>
#include <stddef.h>

void uart_init()
{
    volatile uint8_t *base = (uint8_t *)UART_START_ADDR;
    *(base + UART_LCR) = 0b11; /* 8bits char */
    *(base + UART_FCR) = 1 << 0; /* FIFO Enable */
    *(base + UART_IER) = 1 << 0; /* Enable receiver data interrupt */

    uint16_t divisor = 592; /* 分频系数 */
    uint8_t divisor_least = divisor & 0xFF;
    uint8_t divisor_most = divisor >> 8;
    uint8_t lcr = *(base + UART_LCR);
    *(base + UART_LCR) = lcr | 1 << 7; /* set LCR_DLAB to access divisor latch */
    *(base + UART_DLL) = divisor_least;
    *(base + UART_DLM) = divisor_most;
    *(base + UART_LCR) = lcr; /* back to normal */
}

int8_t uart_read()
{
    volatile int8_t *base = (int8_t *)UART_START_ADDR;
    if ((*(base + UART_LSR) & 1) == 0) {
        return -1;
    } else {
        return *(base + UART_THR);
    }
}

void uart_write(int8_t c)
{
    volatile int8_t *base = (int8_t *)UART_START_ADDR;
    *(base + UART_THR) = c;
}