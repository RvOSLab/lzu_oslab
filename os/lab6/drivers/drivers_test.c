#include <device.h>
#include <device/test.h>

void drivers_test() {
    struct device *dev = get_dev_by_major_minor(0x1234, 1);
    struct test_device * test = dev->get_interface(dev, TEST_INTERFACE_BIT);
    test->shutdown(dev);
}
