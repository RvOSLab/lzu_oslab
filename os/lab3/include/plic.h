/**
 * @file plic.h
 * @brief 所有和 PLIC(Platform Level Interrupt Controller) 有关的声明
 */
#ifndef __PLIC_H__
#define __PLIC_H__
#include <stddef.h>

/// @{ @name PLIC 内存映射 IO 地址
/** PLIC 的内存映射为 [0x0c000000, 0x1000_0000), 仅使用 hart 0 相关的 PLIC */
#define PLIC_START 0x0c000000               /**< PLIC 起始物理地址 */
#define PLIC_LENGTH 0x04000000              /**< PLIC MMIO 内存大小 */
#define PLIC_END PLIC_START + PLIC_LENGTH   /**< PLIC 结束物理地址 */
#define PLIC_START_ADDR PLIC_START          /**< PLIC 起始地址（为虚拟分页预留） */
/// @}

/// @{ @name PLIC 偏移
#define PLIC_PRIORITY 0x0000000           /**< 设置中断优先级 */
#define PLIC_PENDING 0x00001000           /**< 待处理的中断位图 */
#define PLIC_ENABLE 0x00002080            /**< 使能中断 */
#define PLIC_THRESHOLD 0x00200000         /**< 发生中断的阈值，当且仅当中断优先级高于此阈值时发生中断 */
#define PLIC_CLAIM 0x00200004             /**< 获取优先级最高的待处理中断 */
#define PLIC_COMPLETE 0x00200004          /**< 告知 PLIC 已完成中断处理 */
/// @}

void plic_enable_interrupt(uint32_t id);
void plic_set_priority(uint32_t id, uint8_t priority);
void plic_set_threshold(uint8_t threshold);
uint32_t plic_claim();
void plic_complete(uint32_t id);
void plic_init();
int plic_is_pending(uint32_t id);
uint64_t plic_pending();
#endif /* end of include guard: __PLIC_H__ */
