#ifndef UART8250_H
#define UART8250_H

#include <device/serial.h>

#define UART8250_MAJOR 0x8250

struct uart_qemu_regs {
    volatile uint8_t RBR_THR_DLL; // 0x00, Receiver Buffer Register/Transmitter Holding Register/Divisor Latch LSB
    volatile uint8_t IER_DLM; // 0x01, Interrupt Enable Register/Divisor Latch MSB
    volatile uint8_t IIR_FCR; // 0x02, Interrupt Identification Register/FIFO Control Register
    volatile uint8_t LCR; // 0x03, Line Control Register
    volatile uint8_t MCR; // 0x04, Modem Control Register
    volatile uint8_t LSR; // 0x05, Line Status Register
};

enum uart8250_reg_bit {
    IER_ERBFI = 0,
    LSR_DR = 0,
    LSR_THRE = 5,
    LCR_DLAB = 7,
};

extern struct device_driver uart8250_driver;

#endif
