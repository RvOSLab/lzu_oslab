#include <device.h>
#include <device/reset.h>
#include <device/reset/sifive_test.h>

void drivers_test() {
    struct device *dev = get_dev_by_major_minor(SIFIVE_TEST_MAJOR, 1);
    struct reset_device * test = dev->get_interface(dev, RESET_INTERFACE_BIT);
    test->shutdown(dev);
}
