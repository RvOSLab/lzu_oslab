#ifndef DEVICE_RESET_H
#define DEVICE_RESET_H

#define RESET_INTERFACE_BIT (1 << 12)

struct reset_device {
    void (*shutdown)(struct device *dev);
    void (*reboot)(struct device *dev);
};

#endif
