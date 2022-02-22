#include <string.h>
#include <kdebug.h>
#include <mm.h>
#include <device/loader.h>
#include <device/reset/sifive_test.h>
#include <device/irq/plic.h>
#include <device/serial/uart8250.h>
#include <device/virtio.h>

struct device_driver *driver_list[] = {
    &test_driver,
    &plic_driver,
    &uart8250_driver,
    &virtio_driver,
    NULL
};
