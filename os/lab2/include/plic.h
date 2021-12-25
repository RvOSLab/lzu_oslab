/**
 * @file plic.h
 * @brief 所有和 PLIC(Platform Level Interrupt Controller) 有关的声明
 */
#ifndef __PLIC_H__
#define __PLIC_H__
#include <stddef.h>

/// @{ @name PLIC 内存映射 IO 地址
/** PLIC 的内存映射为 [0x0c000000, 0x1000_0000), 仅使用 hart 0 相关的 PLIC */
//#define PLIC_START 0x10000000               /**< PLIC 起始物理地址 */
//#define PLIC_LENGTH 0x04000000              /**< PLIC MMIO 内存大小 */
//#define PLIC_END PLIC_START + PLIC_LENGTH   /**< PLIC 结束物理地址 */
//#define PLIC_START_ADDR PLIC_START          /**< PLIC 起始地址（为虚拟分页预留） */
/// @}

struct plic_regs {
    volatile uint32_t PLIC_PRIO_REG[1024]; /**< 中断优先级寄存器，0保留 */
    volatile uint32_t PLIC_IP_REG[32]; /**< 待处理中断寄存器 */
    volatile uint32_t PLIC_PADDING1[(0x2000 - 0x1000 - 4 * 32) >> 2];
    volatile uint32_t PLIC_IE_REG[15871][32]; /**< 中断使能寄存器 */
    volatile uint32_t PLIC_PADDING2[(0x1FFFFC - 0x2000 - 4 * 32 * 15871) >> 2];
    volatile uint32_t PLIC_CTRL_REG; /**< 0x1FFFFC 中断访问权限控制寄存器（D1专属） */
    volatile struct {
        volatile uint32_t PLIC_TH_REG; /**< 中断阈值寄存器 */
        volatile uint32_t PLIC_CLAIM_REG; /**< 中断声明与完成寄存器 */
        volatile uint32_t PADDING[(0x1000 - 2 * 4) >> 2];
    } PLIC_TH_CLAIM_COMPLITE_REG[15871];
};

struct plic_class_device {
    uint32_t id;
    volatile struct plic_regs *plic_start_addr;
};

enum plic_device_type {
    QEMU_PLIC = 0,
    SUNXI_PLIC = 1,
};

enum qemu_irq {
    UART_QEMU_IRQ = 10,
    RTC_GOLDFISH_IRQ = 11,
    VIRTIO_IRQ = 1, /* 1 to 8 */
    VIRTIO_COUNT = 8,
    PCIE_IRQ = 0x20, /* 32 to 35 */
    VIRTIO_NDEV = 0x35, /* Arbitrary maximum number of interrupts */
};

enum nezha_irq {
    RTC_SUNXI_IRQ = 160,
    UART_SUNXI_IRQ = 0x12,
};

void plic_enable_interrupt(uint32_t id);
void plic_set_priority(uint32_t id, uint8_t priority);
void plic_set_threshold(uint8_t threshold);
uint32_t plic_claim();
void plic_complete(uint32_t id);
void plic_init();
uint32_t plic_is_pending(uint32_t id);
uint64_t plic_pending();

#endif /* end of include guard: __PLIC_H__ */
