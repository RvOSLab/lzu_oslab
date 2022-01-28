#ifndef DEVICE_LOADER
#define DEVICE_LOADER

#include <device.h>
#include <device/fdt.h>

extern struct device_driver *driver_list[];

void fdt_loader(const struct fdt_header *fdt, struct device_driver *driver_list[]);

#endif
