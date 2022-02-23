#include <device/serial/uart8250.h>
#include <device/irq.h>
#include <kdebug.h>
#include <riscv.h>
#include <sched.h>

struct driver_resource uart8250_mmio_res = {
    .resource_start = 0x10000000,
    .resource_end = 0x10000100,
    .resource_type = DRIVER_RESOURCE_MEM
};

#define UART8250_BUFF_LEN 1024
uint8_t uart8250_rx_buffer[UART8250_BUFF_LEN];
uint64_t uart8250_rx_buffer_start = 0;
uint64_t uart8250_rx_buffer_end = 0;
uint64_t uart8250_rx_buffer_empty = 1;
struct task_struct *uart8250_rx_buffer_wait = NULL;

void uart8250_rx_irq_handler(struct device *dev) {
    struct uart_qemu_regs *regs = (struct uart_qemu_regs *)uart8250_mmio_res.map_address;
    wake_up(&uart8250_rx_buffer_wait);
    while (regs->LSR & (1 << LSR_DR)) {
        uart8250_rx_buffer_empty = 0;
        uart8250_rx_buffer[uart8250_rx_buffer_end] = regs->RBR_THR_DLL;
        uart8250_rx_buffer_end = (uart8250_rx_buffer_end + 1) % UART8250_BUFF_LEN;
        if (uart8250_rx_buffer_start == uart8250_rx_buffer_end) { // full
            kprintf("buffer: ");
            for(uint64_t i=0;i<UART8250_BUFF_LEN;i++) {
                kputchar(uart8250_rx_buffer[(i + uart8250_rx_buffer_end) % UART8250_BUFF_LEN]);
            }
            kputchar('\n');
            uart8250_rx_buffer_start = (uart8250_rx_buffer_start + 1) % UART8250_BUFF_LEN;
        }
    }
}

struct irq_descriptor uart8250_rx_irq = {
    .name = "uart8250 rx irq handler",
    .handler = uart8250_rx_irq_handler
};

void uart8250_init(struct device *dev) {
    device_add_resource(dev, &uart8250_mmio_res);
    struct uart_qemu_regs *regs = (struct uart_qemu_regs *)uart8250_mmio_res.map_address;

    regs->IER_DLM = 0; // 关闭 16550a 的所有中断，避免初始化未完成就发生中断
    while(!(regs->LSR & (1 << LSR_THRE))); // 等待发送缓冲区为空

    // 设置波特率
    uint8_t divisor_least = 592 & 0xFF;
    uint8_t divisor_most = 592 >> 8;
    regs->LCR |= 1 << LCR_DLAB; // 设置 DLAB=1，进入波特率设置模式，访问 DLL 和 DLM 寄存器设置波特率
    regs->RBR_THR_DLL = divisor_least; // 设置 DLL
    regs->IER_DLM = divisor_most; // 设置 DLM

    regs->LCR = 0b00000011; // 一次传输 8bit（1字节），无校验，1 位停止位，禁用 break 信号，设置 DLAB=0，进入数据传输/中断设置模式
    regs->IIR_FCR |= 0b00000001; // 设置 FCR[TL]=00，设置中断阈值为 1 字节，设置 FCR[FIFOE]=1，启动 FIFO
    regs->IER_DLM |= 1 << IER_ERBFI; // 设置 IER，启用接收数据时发生的中断

    irq_add(0, 0x0a, &uart8250_rx_irq);
}

uint64_t uart8250_request(struct device *dev, void *buffer, uint64_t size, uint64_t is_read) {
    char *char_buffer = (char *)buffer;
    if (is_read) {
        for (uint64_t i = 0; i < size; i += 1) {
            if (uart8250_rx_buffer_empty) sleep_on(&uart8250_rx_buffer_wait);
            char_buffer[i] = uart8250_rx_buffer[uart8250_rx_buffer_start];
            uart8250_rx_buffer_start = (uart8250_rx_buffer_start + 1) % UART8250_BUFF_LEN;
            if (uart8250_rx_buffer_start == uart8250_rx_buffer_end) { // empty
                uart8250_rx_buffer_empty = 1;
            }
        }
        return size;
    } else {// !is_read
        struct uart_qemu_regs *regs = (struct uart_qemu_regs *)uart8250_mmio_res.map_address;
        for (uint64_t i = 0; i < size; i += 1) {
            while (!(regs->LSR & (1 << LSR_THRE)));
            regs->RBR_THR_DLL = char_buffer[i];
        }
        return size;
    }
}

struct serial_device uart8250_serial_device = {
    .request = uart8250_request
};

void *uart8250_get_interface(struct device *dev, uint64_t flag) {
    if(flag & SERIAL_INTERFACE_BIT) return &uart8250_serial_device;
    return NULL;
}

uint64_t uart8250_device_probe(struct device *dev) {
    device_init(dev);
    uart8250_init(dev);
    device_set_data(dev, NULL);
    uart8250_serial_device.dev = dev;
    device_set_interface(dev, SERIAL_INTERFACE_BIT, uart8250_get_interface);
    device_register(dev, "uart8250", UART8250_MAJOR, NULL);
    device_add_resource(dev, &uart8250_mmio_res);
    return 0;
}

struct driver_match_table uart8250_match_table[] = {
    { .compatible = "ns16550a" },
    { NULL }
};

struct device_driver uart8250_driver = {
    .driver_name = "MaPl UART8250 driver",
    .match_table = uart8250_match_table,
    .device_probe = uart8250_device_probe
};
