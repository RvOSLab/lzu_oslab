#ifndef DEVICE_SMHC_H
#define DEVICE_SMHC_H

#include <stddef.h>
#include <device.h>

#define SMHC_INTERFACE_BIT (1 << 7)

struct smhc_device {
    struct device *dev;
    uint64_t (*init)(struct device *dev);
    uint64_t (*request)(struct device *dev, void *buffer, uint64_t size, uint64_t opration);
};

#endif
