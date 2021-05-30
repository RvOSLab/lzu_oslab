#ifndef __UART_H__
#define __UART_H__
#include <stddef.h>

/// @{ @name UART MMIO 地址
/**
 * qemu 模拟器包含一个 UART 实例，MMIO 地址分别为 [0x10000000, 0x10000100)
 */
#define UART_START    0x10000000                 /**< UART MMIO 起始物理地址 */
#define UART_LENGTH   0x00000100                 /**< UART MMIO 内存大小 */
#define UART_END      (UART_START + UART_LENGTH) /**< UART MMIO 结束物理地址 */
/// @}

/// @{ @name UART 寄存器偏移
#define UART_RBR    0x00
#define UART_THR    0x00
#define UART_DLL    0x00
#define UART_IER    0x01
#define UART_DLM    0x01
#define UART_IIR    0x02
#define UART_FCR    0x02
#define UART_LCR    0x03
#define UART_MCR    0x04
#define UART_LSR    0x05
#define UART_MSR    0x06
#define UART_SCR    0x07
/// @}
//
void uart_init();
int8_t uart_read();
#endif /* end of include guard: __UART_H__ */
