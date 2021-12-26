#ifndef __UART_H__
#define __UART_H__
#include <stddef.h>
#include <plic.h>

/// @{ @name UART MMIO 地址
/**
 * qemu 模拟器包含一个 UART 实例，MMIO 地址分别为 [0x10000000, 0x10000100)
 */
#define UART_START      0x10000000                 /**< UART MMIO 起始物理地址 */
#define UART_LENGTH     0x00000100                 /**< UART MMIO 内存大小 */
#define UART_END        (UART_START + UART_LENGTH) /**< UART MMIO 结束物理地址 */
#define UART_START_ADDR PLIC_END_ADDR              /**< UART MMIO 起始虚拟地址 */
#define UART_END_ADDR (PLIC_END_ADDR + UART_LENGTH)   /**< UART MMIO 结束虚拟地址 */
/// @}

/// @{ @name UART 寄存器偏移
#define UART_RBR    0x00 /** Receiver Buffer Register **/
#define UART_THR    0x00 /** Transmitter Holding Register **/
#define UART_DLL    0x00 /** Divisor Latch (Least Significant Byte) Register **/
#define UART_DLM    0x01 /** Divisor Latch (Most Significant Byte) Register **/
#define UART_IER    0x01 /** Interrupt Enable Register **/
#define UART_IIR    0x02 /** Interrupt Identification Register **/
#define UART_FCR    0x02 /** FIFO Control Register **/
#define UART_LCR    0x03 /** Line Control Register **/
#define UART_MCR    0x04 /** Modem Control Register **/
#define UART_LSR    0x05 /** Line Status Register **/
#define UART_MSR    0x06 /** Modem Status Register **/
#define UART_SCR    0x07 /** Scratch Register **/
/// @}
//
void uart_init();
int8_t uart_read();
void uart_write(int8_t c);
#endif /* end of include guard: __UART_H__ */
