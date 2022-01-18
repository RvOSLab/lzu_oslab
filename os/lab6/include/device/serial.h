#ifndef DEVICE_SERIAL_H
#define DEVICE_SERIAL_H

#include <stddef.h>
#include <device.h>

#define SERIAL_INTERFACE_BIT (1 << 6)

struct serial_device {
    struct device *dev;
    uint64_t (*write)(struct device *dev, void *buffer, uint64_t size);
    uint64_t (*read)(struct device *dev, void *buffer, uint64_t size);
};

#endif
