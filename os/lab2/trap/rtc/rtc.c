#include <rtc.h>
#include <dtb.h>
#include <string.h>

struct rtc_class_device rtc_device;
extern struct device_node node[100];
extern struct property prop[100];
extern int64_t node_used;
extern int64_t prop_used;

static uint64_t rtc_probe()
{
    for (size_t i = 0; i < node_used+20; i++) {
        if (is_begin_with(node[i].name, "rtc")) {
            for (struct property *prop_ptr = node[i].properties; prop_ptr;
                 prop_ptr = prop_ptr->next) {
                if (strcmp(prop_ptr->name, "compatible") == 0) {
                    if (strcmp(prop_ptr->value, "google,goldfish-rtc") == 0) {
                        return GOLDFISH_RTC;
                    } else if (strcmp(prop_ptr->value,
                                      "allwinner,sun20iw1-rtc") == 0) {
                        return SUNXI_RTC;
                    }
                }
            }
        }
    }
    return -1;
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