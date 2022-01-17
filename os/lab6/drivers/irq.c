#include <device/irq.h>

struct device *irq_device = NULL;
void (*interrupt_handle)(struct device *dev) = NULL;
