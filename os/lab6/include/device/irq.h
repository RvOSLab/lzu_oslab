#ifndef DEVICE_IRQ_H
#define DEVICE_IRQ_H

#include <stddef.h>
#include <device.h>

#define IRQ_INTERFACE_BIT (1 << 11)

struct irq_descriptor {
    const char* name;
    struct device *dev;
    void (*handler)(struct device *dev);
};

/* 初始状态: 不使能任何IRQ, 阈值为最低 */
struct irq_device {
    struct device *dev;
    void (*enable_irq)(struct device *dev, uint32_t hart_id, uint32_t irq_id);
    void (*disable_irq)(struct device *dev, uint32_t hart_id, uint32_t irq_id);
    void (*set_threshold)(struct device *dev, uint32_t hart_id, uint32_t threshold);
    void (*set_priority)(struct device *dev, uint32_t irq_id, uint32_t priority);
    void (*set_handler)(struct device *dev, uint32_t hart_id, uint32_t irq_id, struct irq_descriptor* descriptor);
    void (*interrupt_handle)(struct device *dev);
};

extern struct irq_device *g_irq_dev;
static inline void setup_irq_dev(struct device *dev) {
    if (dev) {
        g_irq_dev = dev->get_interface(dev, IRQ_INTERFACE_BIT);
    }
}
static inline struct irq_device *get_irq_device() {
    return g_irq_dev;
}
static inline void irq_handle() {
    if(g_irq_dev) {
        g_irq_dev->interrupt_handle(g_irq_dev->dev);
    }
}
static inline void irq_add(uint32_t hart_id, uint32_t irq_id, struct irq_descriptor* descriptor) {
    if(g_irq_dev) {
        g_irq_dev->set_handler(g_irq_dev->dev, hart_id, irq_id, descriptor);
    }
}

#endif
