#include <uart.h>
#include <stddef.h>
#include <trap.h>

static int8_t uart_16550a_read();
static void uart_16550a_directly_write(int8_t c);
static void uart_16550a_interrupt_handler();
static void uart_16550a_putc(int8_t c);
static void uart_16550a_init();
void uart_sunxi_init();

extern struct uart_class_device uart_device;

struct uart_sunxi_regs {
    uint32_t RBR_THR_DLL; // 0x00, Receiver Buffer Register/Transmitter Holding Register/Divisor Latch LSB
    uint32_t IER_DLM; // 0x04, Interrupt Enable Register/Divisor Latch MSB
    uint32_t IIR_FCR; // 0x08, Interrupt Identification Register/FIFO Control Register
    uint32_t LCR; // 0x0c, Line Control Register
    uint32_t MCR; // 0x10, Modem Control Register
    uint32_t LSR; // 0x14, Line Status Register
    uint32_t padding1[(0x007C-0x18)/4];
    uint32_t USR; // 0x007C, UART Status Register
    uint32_t padding2[(0x00A4-0x80)/4];
    uint32_t HALT; // 0x00A4, UART Halt TX Register
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
    USR_BUSY = 0,
    HALT_HALT_TX = 0,
    HALT_CHCFG_AT_BUSY = 1,
    HALT_CHANGE_UPDATE = 2,
    LSR_THRE = 5,
    LCR_DLAB = 7,
};

enum reg_other {
    UART_BUSY_VALUE = 0b0111,
    IIR_IID_MASK = 0b1111,
};

void uart_sunxi_init()
{
    uart_device.uart_start_addr = 0x02500000;
    uart_device.divisor = 13;
    uart_16550a_init();
}

static void uart_16550a_init()
{
    volatile struct uart_sunxi_regs *regs = (struct uart_sunxi_regs *)uart_device.uart_start_addr;
    regs->IER_DLM = 0; // 关闭 16550a 的所有中断，避免初始化未完成就发生中断
    while(!(regs->LSR & (1 << LSR_THRE))); // 等待发送缓冲区为空

    // 设置波特率
    uint8_t divisor_least = uart_device.divisor & 0xFF;
    uint8_t divisor_most = uart_device.divisor >> 8;
    regs->HALT |= 1 << HALT_HALT_TX; // 关闭发送（全志平台特有）
    regs->HALT |= 1 << HALT_CHCFG_AT_BUSY; // 先设置 HALT 中的 CHCFG_AT_BUSY 位，使得 DLL 和 DLM 可以被写入（全志平台特有）
    regs->LCR |= 1 << LCR_DLAB; // 设置 DLAB=1，进入波特率设置模式，访问 DLL 和 DLM 寄存器设置波特率
    while (regs->USR & 1 << USR_BUSY); // 等待 USR 中的 BUSY 位为 0，即空闲
    regs->RBR_THR_DLL = divisor_least; // 设置 DLL
    regs->IER_DLM = divisor_most; // 设置 DLM
    regs->HALT |= 1 << HALT_CHANGE_UPDATE; // 将 HALT 中的 CHANGE_UPDATE 位置 1，使得 DLL 和 DLM 被更新（全志平台特有）
    while (regs->HALT & (1 << HALT_CHANGE_UPDATE)); // 等待 HALT 中的 CHANGE_UPDATE 位置 0，表示 DLL 和 DLM 已经被读取（全志平台特有）
    regs->HALT &= ~(1 << HALT_HALT_TX); // 开启发送（全志平台特有）

    regs->LCR = 0b00000011; // 一次传输 8bit（1字节），无校验，1 位停止位，禁用 break 信号，设置 DLAB=0，进入数据传输/中断设置模式
    regs->IIR_FCR |= 0b00000001; // 设置 FCR[TL]=00，设置中断阈值为 1 字节，设置 FCR[FIFOE]=1，启动 FIFO
    regs->IER_DLM |= 1 << IER_ERBFI; // 设置 IER，启用接收数据时发生的中断

    uart_device.ops = uart_16550a_ops;
}

static int8_t uart_16550a_read()
{
    volatile struct uart_sunxi_regs *regs = (struct uart_sunxi_regs *)uart_device.uart_start_addr;
    return regs->RBR_THR_DLL; /* read RBR */
}

static void uart_16550a_directly_write(int8_t c)
{
    volatile struct uart_sunxi_regs *regs = (struct uart_sunxi_regs *)uart_device.uart_start_addr;
    regs->RBR_THR_DLL = c; /* write THR */
}

static void uart_16550a_putc(int8_t c)
{
    volatile struct uart_sunxi_regs *regs = (struct uart_sunxi_regs *)uart_device.uart_start_addr;
    while (!(regs->LSR & (1 << LSR_THRE)))
        ;
    uart_16550a_directly_write(c);
}

static void uart_16550a_interrupt_handler()
{
    volatile struct uart_sunxi_regs *regs = (struct uart_sunxi_regs *)uart_device.uart_start_addr;
    if ((regs->IIR_FCR & IIR_IID_MASK) == UART_BUSY_VALUE)     // 处理有时仍可能会发生的 UART busy 中断，标志是 UART_IIR 寄存器的低 4 位为 0111
        regs->USR;  // 处理方法是读取 regs->USR 以清除中断
    while (regs->LSR & (1 << LSR_DR)) {
        int8_t c = uart_read();
        if (c > -1)
            game_keyboard_update(c);
    }
}
