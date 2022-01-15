#ifndef DEVICE_TEST_H
#define DEVICE_TEST_H

#define TEST_INTERFACE_BIT (1 << 12)

struct test_device {
    void (*shutdown)(struct device *dev);
};

#endif
