#include <uart.h>
#include <stddef.h>
static int8_t uart_16550a_read();
static void uart_16550a_directly_write(int8_t c);
static void uart_16550a_interrupt_handler();
static void uart_16550a_putc(int8_t c);
static void uart_16550a_init();
void uart_qemu_init();

extern struct uart_class_device uart_device;

struct uart_qemu_regs {
    uint8_t RBR_THR_DLL; // 0x00, Receiver Buffer Register/Transmitter Holding Register/Divisor Latch LSB
    uint8_t IER_DLM; // 0x01, Interrupt Enable Register/Divisor Latch MSB
    uint8_t IIR_FCR; // 0x02, Interrupt Identification Register/FIFO Control Register
    uint8_t LCR; // 0x03, Line Control Register
    uint8_t MCR; // 0x04, Modem Control Register
    uint8_t LSR; // 0x05, Line Status Register
};

static const struct uart_class_ops uart_16550a_ops = {
    .uart_interrupt_handler = uart_16550a_interrupt_handler,
    .uart_read = uart_16550a_read,
    .uart_directly_write = uart_16550a_directly_write,
    .uart_putc = uart_16550a_putc,
};

enum reg_bit {
    IER_ERBFI = 0,
    LSR_DR = 0,
    LSR_THRE = 5,
    LCR_DLAB = 7,
};

void uart_qemu_init()
{
    uart_device.uart_start_addr = 0x10000000;
    uart_device.divisor = 592;
    uart_16550a_init();
}

static void uart_16550a_init()
{
    volatile struct uart_qemu_regs *regs = (struct uart_qemu_regs *)uart_device.uart_start_addr;

    regs->IER_DLM = 0; // 关闭 16550a 的所有中断，避免初始化未完成就发生中断
    while(!(regs->LSR & (1 << LSR_THRE))); // 等待发送缓冲区为空

    // 设置波特率
    uint8_t divisor_least = uart_device.divisor & 0xFF;
    uint8_t divisor_most = uart_device.divisor >> 8;
    regs->LCR |= 1 << LCR_DLAB; // 设置 DLAB=1，进入波特率设置模式，访问 DLL 和 DLM 寄存器设置波特率
    regs->RBR_THR_DLL = divisor_least; // 设置 DLL
    regs->IER_DLM = divisor_most; // 设置 DLM

    regs->LCR = 0b00000011; // 一次传输 8bit（1字节），无校验，1 位停止位，禁用 break 信号，设置 DLAB=0，进入数据传输/中断设置模式
    regs->IIR_FCR |= 0b00000001; // 设置 FCR[TL]=00，设置中断阈值为 1 字节，设置 FCR[FIFOE]=1，启动 FIFO
    regs->IER_DLM |= 1 << IER_ERBFI; // 设置 IER，启用接收数据时发生的中断

    uart_device.ops = uart_16550a_ops;
}

static int8_t uart_16550a_read()
{
    volatile struct uart_qemu_regs *regs = (struct uart_qemu_regs *)uart_device.uart_start_addr;
    return regs->RBR_THR_DLL; /* read RBR */
}

static void uart_16550a_directly_write(int8_t c)
{
    volatile struct uart_qemu_regs *regs = (struct uart_qemu_regs *)uart_device.uart_start_addr;
    regs->RBR_THR_DLL = c; /* write THR */
}

static void uart_16550a_putc(int8_t c)
{
    volatile struct uart_qemu_regs *regs = (struct uart_qemu_regs *)uart_device.uart_start_addr;
    while (!(regs->LSR & (1 << LSR_THRE)))
        ;
    uart_16550a_directly_write(c);
}

static void uart_16550a_interrupt_handler()
{
    volatile struct uart_qemu_regs *regs = (struct uart_qemu_regs *)uart_device.uart_start_addr;
    while (regs->LSR & (1 << LSR_DR)) {
        int8_t c = uart_read();
        if (c > -1)
            uart_16550a_putc(c);
    }
}