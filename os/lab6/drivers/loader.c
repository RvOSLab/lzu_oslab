#include <string.h>
#include <kdebug.h>
#include <mm.h>
#include <device.h>
#include <device/reset/sifive_test.h>
#include <device/irq/plic.h>
#include <device/serial/uart8250.h>

struct device_driver *driver_list[] = {
    &test_driver,
    &plic_driver,
    &uart8250_driver,
    NULL
};

const char * device_list[] = {
    "sifive,test1",
    "sifive,plic-1.0.0",
    "ns8250",
    NULL
};

void load_drivers() {
    for (uint64_t device_idx = 0; device_list[device_idx]; device_idx += 1) {
        const char * device_compatible = device_list[device_idx];
        uint64_t is_device_matched = 0;
        for (uint64_t driver_idx = 0; driver_list[driver_idx]; driver_idx += 1) {
            struct device_driver *driver = driver_list[driver_idx];
            for (uint64_t driver_match = 0; ; driver_match += 1){
                struct driver_match_table match_table = driver->match_table[driver_match];
                const char * driver_match_str = match_table.compatible;
                if (!driver_match_str) break;
                if (!strcmp(driver_match_str, device_compatible)) {
                    struct device *dev = kmalloc(sizeof(struct device));
                    dev->match_data = match_table.match_data;
                    driver->device_probe(dev);
                    is_device_matched = 1;
                    kprintf("load device %s\n\tdriver: %s\n", device_compatible, driver->driver_name);
                    break;
                }
            }
            if (is_device_matched) break;
        }
    }
}
