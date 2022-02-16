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
            if (strcmp(prop[i].value, "thead,c900-plic") == 0) {
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
        plic_device.plic_start_addr = (volatile struct plic_regs *)0x0c000000;
        plic_enable_interrupt(RTC_GOLDFISH_IRQ);
        plic_enable_interrupt(UART_QEMU_IRQ);
        plic_set_priority(RTC_GOLDFISH_IRQ, 1);
        plic_set_priority(UART_QEMU_IRQ, 1);
        break;
    case SUNXI_PLIC:
        plic_device.plic_start_addr = (volatile struct plic_regs *)0x10000000;
        plic_enable_interrupt(UART_SUNXI_IRQ);
        plic_enable_interrupt(RTC_SUNXI_IRQ);
        plic_set_priority(UART_SUNXI_IRQ, 1);
        plic_set_priority(RTC_SUNXI_IRQ, 1);
        break;
    }
    plic_set_threshold(0);
    set_csr(sie, 1 << IRQ_S_EXT);
}

void plic_enable_interrupt(uint32_t id)
{
    plic_device.plic_start_addr->PLIC_IE_REG[1][id / 32] |= (1 << (id % 32));
}

// QEMU Virt machine support 7 priority (1 - 7),
// The "0" is reserved, and the lowest priority is "1".
void plic_set_priority(uint32_t id, uint8_t priority)
{
    plic_device.plic_start_addr->PLIC_PRIO_REG[id] = priority & 7;
}

void plic_set_threshold(uint8_t threshold)
{
    plic_device.plic_start_addr->PLIC_TH_CLAIM_COMPLITE_REG[1].PLIC_TH_REG = threshold & 7;
}

/**
 * @brief 主动获取外中断号
 *
 * @return 最高优先级的待处理中断号，不存在则返回 0
 */
uint32_t plic_claim()
{
    return plic_device.plic_start_addr->PLIC_TH_CLAIM_COMPLITE_REG[1].PLIC_CLAIM_REG;
}

void plic_complete(uint32_t id)
{
    plic_device.plic_start_addr->PLIC_TH_CLAIM_COMPLITE_REG[1].PLIC_CLAIM_REG = id;
    //plic_enable_interrupt(id);
}

/*
 *
 * @brief 返回待处理终端向量
 *
 * @note 可以先 enable 再 claim 来清空 pending
 
uint64_t plic_pending()
{
    return plic_device.plic_start_addr->PLIC_IP_REG;
}
*/

/**
 * @brief 判断某中断是否待处理
 */
uint32_t plic_is_pending(uint32_t id)
{
    volatile uint32_t *p = (volatile uint32_t *)&plic_device.plic_start_addr->PLIC_IP_REG;
    return p[id / 32] & (1 << (id % 32));
}
