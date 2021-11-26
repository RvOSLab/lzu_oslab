#include <rtc.h>
#include <stddef.h>
#include <kdebug.h>

/**
 * qemu 模拟器包含一个 GOLDFISH_RTC 实例，MMIO 物理地址分别为 [0x101000, 0x102000)
 */
#define GOLDFISH_RTC_START 0x101000 /**< RTC MMIO 起始物理地址 */
#define GOLDFISH_RTC_LENGTH 0x1000 /**< RTC MMIO 内存大小 */
#define GOLDFISH_RTC_END                                                       \
	(GOLDFISH_RTC_START + GOLDFISH_RTC_LENGTH) /**< RTC MMIO 结束物理地址 */
#define GOLDFISH_RTC_START_ADDR                                                \
	GOLDFISH_RTC_START /**< RTC MMIO 起始地址(为虚拟分页预留) */

struct rtc_regs {
	// time 先读低32位 https://github.com/qemu/qemu/blob/v6.2.0-rc2/hw/rtc/goldfish_rtc.c#L106-L113
	// alarm 先写高32位 https://github.com/qemu/qemu/blob/v6.2.0-rc2/hw/rtc/goldfish_rtc.c#L154-L160
	uint32_t time_low; //0x00   R/W: 低32位时间
	uint32_t time_high; //0x04  R/W: 高32位时间
	uint32_t alarm_low; //0x08  R/W: 低32位闹钟
	uint32_t alarm_high; //0x0c R/W: 高32位闹钟
	uint32_t irq_enabled; //0x10    W: 启用中断
	uint32_t clear_alarm; //0x14    W: 清除闹钟
	uint32_t alarm_status; //0x18   R: 闹钟状态(1:闹钟有效)
	uint32_t clear_interrupt; //0x1c W: 清除中断
};

uint64_t goldfish_rtc_read_time() // 取得纳秒时间戳
{
	volatile struct rtc_regs *rtc =
		(volatile struct rtc_regs *)GOLDFISH_RTC_START_ADDR;
	uint32_t low = rtc->time_low;
	uint32_t high = rtc->time_high;
	uint64_t ns = ((uint64_t)high << 32) | (uint64_t)low;
	return ns;
}

void goldfish_rtc_set_time(uint64_t now) // now为纳秒时间戳
{
	volatile struct rtc_regs *rtc =
		(volatile struct rtc_regs *)GOLDFISH_RTC_START_ADDR;
	rtc->time_high = (uint32_t)(now >> 32);
	rtc->time_low = (uint32_t)now;
}

void goldfish_rtc_set_alarm(uint64_t alarm) // alarm为纳秒时间戳
{
	volatile struct rtc_regs *rtc =
		(volatile struct rtc_regs *)GOLDFISH_RTC_START_ADDR;
	rtc->alarm_high = (uint32_t)(alarm >> 32);
	rtc->alarm_low = (uint32_t)alarm;
	rtc->irq_enabled = 1;
}

void goldfish_rtc_clear_alarm()
{
	volatile struct rtc_regs *rtc =
		(volatile struct rtc_regs *)GOLDFISH_RTC_START_ADDR;
	uint64_t rtc_status_reg = rtc->alarm_status;
	if (rtc_status_reg)
		rtc->clear_alarm = 1;
}

uint64_t goldfish_rtc_read_alarm()
{
	volatile struct rtc_regs *rtc =
		(volatile struct rtc_regs *)GOLDFISH_RTC_START_ADDR;
	uint32_t low = rtc->alarm_low;
	uint32_t high = rtc->alarm_high;
	uint64_t ns = ((uint64_t)high << 32) | (uint64_t)low;
	return ns;
}

/**
void goldfish_rtc_enable_alarm_interrupt()      // 先set再enable
{
	volatile struct rtc_regs *rtc =
		(volatile struct rtc_regs *)RTC_START_ADDR;
	if (rtc->alarm_status)
		rtc->irq_enabled = 1;
	else
		rtc->irq_enabled = 0;
}
*/

void goldfish_rtc_interrupt_handler()
{
	volatile struct rtc_regs *rtc =
		(volatile struct rtc_regs *)GOLDFISH_RTC_START_ADDR;
	rtc->clear_interrupt = 1;
	kprintf("!!RTC ALARM!!\n");
	goldfish_rtc_set_alarm(goldfish_rtc_read_time() + 1000000000);
	kprintf("alarm time: %u\n", goldfish_rtc_read_alarm());
	kprintf("timestamp now: %u\n", goldfish_rtc_read_time());
}

static const struct rtc_class_ops goldfish_rtc_ops = {
	.read_time = goldfish_rtc_read_time,
	.set_time = goldfish_rtc_set_time,
	.read_alarm = goldfish_rtc_read_alarm,
	.set_alarm = goldfish_rtc_set_alarm,
	//.alarm_irq_enable = goldfish_rtc_enable_alarm_interrupt
};
