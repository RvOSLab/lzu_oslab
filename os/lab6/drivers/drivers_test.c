#include <device.h>
#include <device/reset/sifive_test.h>
#include <device/serial/uart8250.h>
#include <kdebug.h>

uint64_t char_dev_test(uint64_t c) {
    struct device *dev = get_dev_by_major_minor(UART8250_MAJOR, 1);
    struct serial_device *char_test = dev->get_interface(dev, SERIAL_INTERFACE_BIT);
    char_test->request(dev, &c, 1, !c);
    return c;
}
