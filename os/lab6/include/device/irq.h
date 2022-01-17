#ifndef DEVICE_IRQ_H
#define DEVICE_IRQ_H

#include <stddef.h>
#include <device.h>

#define IRQ_INTERFACE_BIT (1 << 11)

struct irq_descriptor {
    const char* name;
    void (*handler)();
};

/* 初始状态: 不使能任何IRQ, 阈值为最低 */
struct irq_device {
    void (*enable_irq)(struct device *dev, uint32_t hart_id, uint32_t irq_id);
    void (*disable_irq)(struct device *dev, uint32_t hart_id, uint32_t irq_id);
    void (*set_threshold)(struct device *dev, uint32_t hart_id, uint32_t threshold);
    void (*set_priority)(struct device *dev, uint32_t irq_id, uint32_t priority);
    void (*set_handler)(struct device *dev, uint32_t hart_id, uint32_t irq_id, struct irq_descriptor* descriptor);
    void (*interrupt_handle)(struct device *dev);
};

extern struct device *irq_dev;
static inline void set_irq_dev(struct device *dev) {
    irq_dev = dev;
}

static inline struct device *get_irq_dev() {
    return irq_dev;
}

#endif
