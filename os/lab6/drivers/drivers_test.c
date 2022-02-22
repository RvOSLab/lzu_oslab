#include <device.h>
#include <device/reset/sifive_test.h>
#include <device/serial/uart8250.h>
#include <kdebug.h>

void drivers_test() {
    struct device *dev = get_dev_by_major_minor(SIFIVE_TEST_MAJOR, 1);
    struct reset_device * test = dev->get_interface(dev, RESET_INTERFACE_BIT);
    // test->shutdown(dev);
}

void char_dev_test() {
    struct device *dev = get_dev_by_major_minor(UART8250_MAJOR, 1);
    struct serial_device *test = dev->get_interface(dev, SERIAL_INTERFACE_BIT);
    const char *test_str = "mapl char dev test\n";
    test->request(dev, test_str, strlen(test_str), 0);

    char buffer[256];
    test->request(dev, buffer, 4, 1);
    buffer[4] = '\0';
    kputs(buffer);
}
