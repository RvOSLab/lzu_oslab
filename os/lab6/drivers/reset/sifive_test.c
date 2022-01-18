#include <stddef.h>

#include <device/reset/sifive_test.h>

struct driver_resource sifive_test_mmio_res = {
    .resource_start = 0x100000,
    .resource_end = 0x101000,
    .resource_type = DRIVER_RESOURCE_MEM
};

void sifive_test_shutdown(struct device *dev) {
    *((uint32_t *)sifive_test_mmio_res.map_address) = 0x5555;
}

void sifive_test_reboot(struct device *dev) {
    *((uint32_t *)sifive_test_mmio_res.map_address) = 0x7777;
}

struct reset_device sifive_test_device = {
    .shutdown = sifive_test_shutdown,
    .reboot = sifive_test_reboot
};

void * sifive_test_get_interface(struct device *dev, uint64_t flag) {
    if(flag & RESET_INTERFACE_BIT) return &sifive_test_device;
    return NULL;
}

uint64_t test_device_probe(struct device *dev) {
    device_init(dev);
    device_set_data(dev, NULL);
    sifive_test_device.dev = dev;
    device_set_interface(dev, RESET_INTERFACE_BIT, sifive_test_get_interface);
    device_register(dev, "sifive test", SIFIVE_TEST_MAJOR, NULL);
    device_add_resource(dev, &sifive_test_mmio_res);
    return 0;
}

struct driver_match_table test_match_table[] = {
    { .compatible = "sifive,test1" },
    { NULL }
};

struct device_driver test_driver = {
    .driver_name = "MaPl SiFive test driver",
    .match_table = &test_match_table[0],
    .device_probe = test_device_probe
};
