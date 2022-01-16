#include <stddef.h>
#include <device.h>
#include <device/test.h>
#include <device/test/mapl-test.h>

void mapl_test_shutdown(struct device *dev) {
    while (1) ;
}

struct test_device mapl_test_device = {
    .shutdown = mapl_test_shutdown
};

void * mapl_test_get_interface(struct device *dev, uint64_t flag) {
    if(flag & TEST_INTERFACE_BIT) return &mapl_test_device;
    return NULL;
}

uint64_t test_device_probe(struct device *dev) {
    device_set_data(dev, NULL);
    device_set_interface(dev, TEST_INTERFACE_BIT, mapl_test_get_interface);
    device_register(dev, "sifive test", SIFIVE_TEST_MAJOR, NULL);
    return 0;
}

struct driver_match_table test_match_table[] = {
    { .compatible = "sifive,test1" },
    { NULL }
};

struct device_driver test_driver = {
    .driver_name = "mapl sifive test driver",
    .match_table = &test_match_table[0],
    .device_probe = test_device_probe
};
