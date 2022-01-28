#ifndef DEVICE_LOADER
#define DEVICE_LOADER

#include <device/fdt.h>
void fdt_loader(const struct fdt_header *fdt, struct device_driver *driver_list[]);

#endif
