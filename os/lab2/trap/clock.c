/**
 * @file clock.c
 * @author Hanabichan (93yutf@gmail.com)
 * @brief 实现时钟中断
 */
#include <sbi.h>
#include <riscv.h>
#include <kdebug.h>

/** ticks 是时钟中断发生次数的计数 */
volatile size_t ticks;
/** timebase 代表每隔 timebase 次时钟发生一次时钟中断 */
static uint64_t timebase;

/**
 * @brief 从时钟硬件获取当前 cycles 
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
 * 包括设置时钟响应的频率与开启时钟中断
 */
void clock_init()
{
	/** QEMU 的时钟频率为 10MHz，设置timebase = 100000表示时钟中断频率为100Hz */
	timebase = 100000;
	ticks = 0;
	/** 开启时钟中断（设置CSR_MIE） */
	//set_csr(CSR_MIE, MIP_MTIP);
	set_csr(0x104, 1 << 5);
	clock_set_next_event();
	kputs("Setup Timer!");
}

/**
 * @brief 每次时钟中断发生时都要设置下一次时钟中断的发生
 */
void clock_set_next_event()
{
	sbi_set_timer(get_cycles() + timebase);
}
