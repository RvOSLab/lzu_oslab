/**
 * @file plic.c
 * @note 访问 MMIO 内存必须使用 volatile 关键字，详情见 **Effectice Modern C++**
 *       Item 40: Use std::atomic for concurrency, volatile for special memory
 */
#include <plic.h>
#include <riscv.h>
#include <dtb.h>
#include <string.h>
#include <kdebug.h>

struct plic_class_device plic_device;
extern struct device_node node[100];
extern struct property prop[100];
extern int64_t node_used;
extern int64_t prop_used;

static uint64_t plic_probe()
{
    for (size_t i = 0; i < prop_used; i++) {
        if (strcmp(prop[i].name, "compatible") == 0) {
            if (strcmp(prop[i].value, "allwinner,sun20i-d1-plic") == 0) {
                kprintf("plic: sun20i-d1-plic\n");
                return SUNXI_PLIC;
            }
            if (strcmp(prop[i].value, "sifive,plic-1.0.0") == 0) {
                return QEMU_PLIC;
            }
        }
    }
    return -1;
}

void plic_init()
{
    plic_device.id = plic_probe();
    switch (plic_device.id) {
    case QEMU_PLIC:
        plic_device.plic_start_addr = 0x0c000000;
        break;
    case SUNXI_PLIC:
        plic_device.plic_start_addr = 0x10000000;
        break;
    }
    plic_set_threshold(0);
    plic_enable_interrupt(SUNXI_UART_IRQ);
    plic_enable_interrupt(GOLDFISH_RTC_IRQ);
    plic_set_priority(SUNXI_UART_IRQ, 1);
    plic_set_priority(GOLDFISH_RTC_IRQ, 2);
    set_csr(sie, 1 << IRQ_S_EXT);
}

void plic_enable_interrupt(uint32_t id)
{
    volatile uint32_t *plic_enable_address =
            (volatile uint32_t *)(plic_device.plic_start_addr + PLIC_ENABLE);
    *plic_enable_address |= 0xffffffff;
}

// QEMU Virt machine support 7 priority (1 - 7),
// The "0" is reserved, and the lowest priority is "1".
void plic_set_priority(uint32_t id, uint8_t priority)
{
    ((volatile uint32_t *)(plic_device.plic_start_addr + PLIC_PRIORITY))[id] =
            priority & 7;
}

void plic_set_threshold(uint8_t threshold)
{
    *(volatile uint32_t *)(plic_device.plic_start_addr + PLIC_PENDING +
                           PLIC_THRESHOLD) = threshold & 7;
}

/**
 * @brief 获取外中断号
 *
 * @return 最高优先级的待处理中断号，不存在则返回 0
 */
uint32_t plic_claim()
{
    return *(volatile uint32_t *)(plic_device.plic_start_addr + PLIC_PENDING +
                                  PLIC_CLAIM);
}

void plic_complete(uint32_t id)
{
    *(volatile uint32_t *)(plic_device.plic_start_addr + PLIC_PENDING +
                           PLIC_COMPLETE) = id;
}

/*
 *
 * @brief 返回待处理终端向量
 *
 * @note 可以先 enable 再 claim 来清空 pending
 */
uint64_t plic_pending()
{
    return *(volatile uint64_t *)(plic_device.plic_start_addr + PLIC_PENDING);
}

/**
 * @brief 判断某中断是否待处理
 */
uint32_t plic_is_pending(uint32_t id)
{
    uint64_t pending = plic_pending();
    volatile uint32_t *p = (volatile uint32_t *)&pending;
    return p[id / 32] & (id % 32);
}
