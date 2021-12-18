#include <uart.h>
#include <stddef.h>
int8_t uart_16550a_read();
void uart_16550a_write(int8_t c);
void uart_16550a_interrupt_handler();

/**
 * qemu 模拟器包含一个 UART 实例，MMIO 地址分别为 [0x10000000, 0x10000100)
 */
#define UART_START 0x10000000 /**< UART MMIO 起始物理地址 */
#define UART_LENGTH 0x00000100 /**< UART MMIO 内存大小 */
#define UART_END (UART_START + UART_LENGTH) /**< UART MMIO 结束物理地址 */
#define UART_START_ADDR UART_START /**< UART MMIO 起始地址(为虚拟分页预留) */

struct uart_16550a_regs {
    uint8_t RBR_THR_DLL; // 0x00, Receiver Buffer Register/Transmitter Holding Register/Divisor Latch LSB
    uint8_t IER_DLM; // 0x01, Interrupt Enable Register/Divisor Latch MSB
    uint8_t IIR_FCR; // 0x02, Interrupt Identification Register/FIFO Control Register
    uint8_t LCR; // 0x03, Line Control Register
    uint8_t MCR; // 0x04, Modem Control Register
    uint8_t LSR; // 0x05, Line Status Register
    uint8_t MSR; // 0x06, Modem Status Register
    uint8_t SCR; // 0x07, Scratch Register
};

static volatile struct uart_16550a_regs *regs = (struct uart_16550a_regs *)UART_START_ADDR;

static const struct uart_class_ops uart_16550a_ops = {
    .uart_interrupt_handler = uart_16550a_interrupt_handler,
    .uart_read = uart_16550a_read,
    .uart_write = uart_16550a_write,
};

void uart_16550a_init()
{
    regs->LCR = 0b11; /* 8bits char */
    regs->IIR_FCR = 1 << 0; /* set FCR, FIFO Enable */
    regs->IER_DLM = 1 << 0; /* set IER, Enable receiver data interrupt */

    uint16_t divisor = 592; /* 分频系数 */
    uint8_t divisor_least = divisor & 0xFF;
    uint8_t divisor_most = divisor >> 8;
    uint8_t lcr = regs->LCR;
    regs->LCR = lcr | 1 << 7; /* set LCR_DLAB to access divisor latch */
    regs->RBR_THR_DLL = divisor_least; /* set divisor latch LSB */
    regs->IER_DLM = divisor_most; /* set divisor latch MSB */
    regs->LCR = lcr; /* back to normal */

    extern struct uart_class_device uart_device;
    uart_device.ops = uart_16550a_ops;
}

int8_t uart_16550a_read()
{
    if ((regs->LSR & 1) == 0) {
        return -1;
    } else {
        return regs->RBR_THR_DLL; /* read THR */
    }
}

void uart_16550a_write(int8_t c)
{
    regs->RBR_THR_DLL = c; /* write THR */
}

void uart_16550a_interrupt_handler()
{
    int8_t c = uart_read();
    if (c > -1)
        uart_write(c);
}