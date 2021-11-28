#include <rtc.h>
struct rtc_class_device rtc_device;

static uint64_t rtc_probe()
{
	// return GOLDFISH_RTC;
	return SUNXI_RTC;
}

void rtc_init()
{
	rtc_device.id = rtc_probe();
	switch (rtc_device.id) {
	case GOLDFISH_RTC:
		goldfish_rtc_init();
		break;
	case SUNXI_RTC:
		sunxi_rtc_init();
		break;
	}
}

uint64_t read_time()
{
	return (*rtc_device.ops.read_time)();
}
void set_time(uint64_t now)
{
	(*rtc_device.ops.set_time)(now);
}
uint64_t read_alarm()
{
	return (*rtc_device.ops.read_alarm)();
}
void set_alarm(uint64_t alarm)
{
	(*rtc_device.ops.set_alarm)(alarm);
}
void rtc_interrupt_handler()
{
    (*rtc_device.ops.rtc_interrupt_handler)();
}
void clear_alarm()
{
    (*rtc_device.ops.clear_alarm)();
}