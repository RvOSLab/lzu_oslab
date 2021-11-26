#ifndef __RTC_H__
#define __RTC_H__
#include <stddef.h>

struct rtc_class_ops {
	//int (*ioctl)(struct device *, unsigned int, unsigned long);
	uint64_t (*read_time)();
	void (*set_time)(uint64_t now);
	uint64_t (*read_alarm)();
	void (*set_alarm)(uint64_t alarm);
	//int (*proc)(struct device *, struct seq_file *);
	//int (*alarm_irq_enable)(struct device *, unsigned int enabled);
	//int (*read_offset)(struct device *, long *offset);
	//int (*set_offset)(struct device *, long offset);
};

void goldfish_rtc_interrupt_handler();
//void goldfish_rtc_enable_alarm_interrupt();
uint64_t goldfish_rtc_read_alarm();
void goldfish_rtc_set_alarm(uint64_t alarm); // alarm为毫秒时间戳
void goldfish_rtc_set_time(uint64_t now); // now为毫秒时间戳
uint64_t goldfish_rtc_read_time(); // 取得毫秒时间戳

#endif /* end of include guard: __RTC_H__ */
