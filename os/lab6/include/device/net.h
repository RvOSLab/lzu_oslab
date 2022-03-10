#ifndef DEVICE_NET_H
#define DEVICE_NET_H

#include <device.h>

#define NET_INTERFACE_BIT (1 << 13)

struct net_device {
    struct device *dev;
    void (*send)(struct device *dev, void *buffer, uint64_t length);
};

#endif
