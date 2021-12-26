#ifndef __RTC_H__
#define __RTC_H__
#include <stddef.h>

// 所有时间都以纳秒级时间戳表示

struct rtc_class_ops {
	//int (*ioctl)(struct device *, unsigned int, unsigned long);
	uint64_t (*read_time)();
	void (*set_time)(uint64_t now);
	uint64_t (*read_alarm)();
	void (*set_alarm)(uint64_t alarm);
	void (*rtc_interrupt_handler)();
	void (*clear_alarm)();
	//int (*proc)(struct device *, struct seq_file *);
	//int (*alarm_irq_enable)(struct device *, unsigned int enabled);
	//int (*read_offset)(struct device *, long *offset);
	//int (*set_offset)(struct device *, long offset);
};
struct rtc_class_device {
	uint32_t id;
	struct rtc_class_ops ops;
};

enum rtc_device_type {
	GOLDFISH_RTC = 0,
	SUNXI_RTC = 1,
};

uint64_t read_time();
void set_time(uint64_t now);
uint64_t read_alarm();
void set_alarm(uint64_t alarm);
void rtc_interrupt_handler();
void clear_alarm();
void rtc_init();

void goldfish_rtc_init();
void sunxi_rtc_init();
#endif /* end of include guard: __RTC_H__ */
