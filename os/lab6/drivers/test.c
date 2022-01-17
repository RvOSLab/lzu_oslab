#include <stddef.h>
#include <device.h>
#include <device/test.h>
#include <device/test/mapl-test.h>

struct driver_resource mapl_test_mmio_res = {
    .resource_start = 0x100000,
    .resource_end = 0x101000,
    .resource_type = DRIVER_RESOURCE_MEM
};

void mapl_test_shutdown(struct device *dev) {
    *((uint32_t *)mapl_test_mmio_res.map_address) = 0x5555;
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
    device_add_resource(dev, &mapl_test_mmio_res);
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
