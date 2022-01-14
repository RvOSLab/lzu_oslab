/**
 * @file clock.c
 * @author Hanabichan (93yutf@gmail.com)
 * @brief 实现时钟中断
 */
#include <clock.h>
#include <sbi.h>
#include <riscv.h>
#include <kdebug.h>

/** 时钟中断发生次数 */
volatile size_t ticks;

/** 每隔 timebase 次时钟周期发生一次时钟中断 */
static uint64_t timebase;

/**
 * @brief 获取开机后经过的时钟周期数
 * @return uint64_t
 */
static inline uint64_t get_cycles()
{
    uint64_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
}

/**
 * @brief 初始化时钟
 * 设置时钟响应的频率与开启时钟中断
 */
void clock_init()
{
    /* QEMU 的时钟频率为 10MHz，设置timebase = 100000表示时钟中断频率为100Hz */
    timebase = 100000;
    ticks = 0;
    /* 开启时钟中断（设置CSR_MIE） */
    set_csr(sie, 1 << IRQ_S_TIMER);
    clock_set_next_event();
    kputs("Setup Timer!");
}

/**
 * @brief 设置下一次时钟中断
 */
void clock_set_next_event()
{
    sbi_set_timer(get_cycles() + timebase);
}
