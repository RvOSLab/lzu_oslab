#include <mm.h>
#include <device/irq/plic.h>

struct plic_match_data *match_info = NULL;
struct plic_match_data qemu_match_info = {
    .mmio_start_addr = 0x0C000000,
    .mmio_end_addr = 0x10000000,
    .num_sources = 127,
    .num_priorities = 7
};

struct driver_resource plic_mmio_res = {
    .resource_start = 0, /* set in init */
    .resource_end = 0, /* set in init */
    .resource_type = DRIVER_RESOURCE_MEM
};

void plic_enable_irq(struct device *dev, uint32_t hart_id, uint32_t irq_id) {
    uint64_t reg_addr = plic_mmio_res.map_address +
        PLIC_ENABLE_BASE + (hart_id * 2 + 1) * PLIC_ENABLE_STRIDE + 
        (irq_id / 32) * 4;
    *((volatile uint32_t *)reg_addr) |= (1 << (irq_id % 32));
}

void plic_disable_irq(struct device *dev, uint32_t hart_id, uint32_t irq_id) {
    uint64_t reg_addr = plic_mmio_res.map_address +
        PLIC_ENABLE_BASE + (hart_id * 2 + 1) * PLIC_ENABLE_STRIDE + 
        (irq_id / 32) * 4;
    *((volatile uint32_t *)reg_addr) &= ~(1 << (irq_id % 32));
}

void plic_set_priority(struct device *dev, uint32_t irq_id, uint32_t priority) {
    uint64_t reg_addr = plic_mmio_res.map_address +
        PLIC_PRIORITY_BASE + 4 * irq_id;
    *((volatile uint32_t *)reg_addr) =
        (priority > match_info->num_priorities ? match_info->num_priorities - 1 : priority);
}

void plic_set_threshold(struct device *dev, uint32_t hart_id, uint32_t threshold) {
    uint64_t reg_addr = plic_mmio_res.map_address +
        PLIC_CONTEXT_BASE + (hart_id * 2 + 1) * PLIC_CONTEXT_STRIDE + PLIC_THRESHOLD_OFFSET;
    *((volatile uint32_t *)reg_addr) =
        (threshold > match_info->num_priorities ? match_info->num_priorities - 1 : threshold);
}

uint32_t plic_get_claim(uint32_t hart_id) {
    uint64_t reg_addr = plic_mmio_res.map_address +
        PLIC_CONTEXT_BASE + (hart_id * 2 + 1) * PLIC_CONTEXT_STRIDE + PLIC_CLAIM_OFFSET;
    return *((volatile uint32_t *)reg_addr);
}

void plic_complete(uint32_t hart_id, uint32_t irq_id) {
    uint64_t reg_addr = plic_mmio_res.map_address +
        PLIC_CONTEXT_BASE + (hart_id * 2 + 1) * PLIC_CONTEXT_STRIDE + PLIC_COMPLETE_OFFSET;
    *((volatile uint32_t *)reg_addr) = irq_id;
}

uint64_t plic_handler_get_hash(struct hash_table_node *node) {
    struct plic_handler *handler = container_of(node, struct plic_handler, hash_node);
    return (handler->irq_id * 0x10001) + 19;
}

uint64_t plic_handler_is_equal(struct hash_table_node *nodeA, struct hash_table_node *nodeB) {
    struct plic_handler *handlerA = container_of(nodeA, struct plic_handler, hash_node);
    struct plic_handler *handlerB = container_of(nodeB, struct plic_handler, hash_node);
    return handlerA->irq_id == handlerB->irq_id;
}

struct hash_table_node plic_handler_buffer[PLIC_HANDLER_BUFFER_LENGTH];
struct hash_table plic_handler_table = {
    .buffer = plic_handler_buffer,
    .buffer_length = PLIC_HANDLER_BUFFER_LENGTH,
    .get_hash = plic_handler_get_hash,
    .is_equal = plic_handler_is_equal
};

void plic_set_handler(struct device *dev, uint32_t hart_id, uint32_t irq_id, struct irq_descriptor* descriptor) {
    struct plic_handler *handler = kmalloc(sizeof(struct plic_handler));
    handler->irq_id = irq_id;
    handler->descriptor = descriptor;
    hash_table_set(&plic_handler_table, &handler->hash_node);
    plic_enable_irq(dev, hart_id, irq_id);
}

void plic_interrupt_handle(struct device *dev) {
    uint32_t irq_id = plic_get_claim(0);
    struct plic_handler tmp = {
        .irq_id = irq_id
    };
    struct hash_table_node *hash_node = hash_table_get(&plic_handler_table, &tmp.hash_node);
    struct plic_handler *handler = container_of(hash_node, struct plic_handler, hash_node);
    if(handler) handler->descriptor->handler(handler->descriptor->dev);
    plic_complete(0, irq_id);
}

void plic_device_init(struct device *dev) {
    match_info = device_get_match_data(dev);
    plic_mmio_res.resource_start = match_info->mmio_start_addr;
    plic_mmio_res.resource_end = match_info->mmio_end_addr;
    device_add_resource(dev, &plic_mmio_res);

    hash_table_init(&plic_handler_table);

    plic_set_threshold(dev, 0, 0);
    for (uint32_t i = 0; i < match_info->num_sources; i += 1) {
        plic_disable_irq(dev, 0, i);
        plic_set_priority(dev, i, 1);
    }
}

struct irq_device plic_irq_device = {
    .enable_irq = plic_enable_irq,
    .disable_irq = plic_disable_irq,
    .set_priority = plic_set_priority,
    .set_threshold = plic_set_threshold,
    .set_handler = plic_set_handler,
    .interrupt_handle = plic_interrupt_handle
};

void *plic_get_interface(struct device *dev, uint64_t flag) {
    if(flag & IRQ_INTERFACE_BIT) return &plic_irq_device;
    return NULL;
}

uint64_t plic_device_probe(struct device *dev) {
    device_init(dev);
    plic_device_init(dev);
    device_set_data(dev, NULL);
    plic_irq_device.dev = dev;
    device_set_interface(dev, IRQ_INTERFACE_BIT, plic_get_interface);
    device_register(dev, "plic", PLIC_MAJOR, NULL);
    setup_irq_dev(dev);
    return 0;
}

struct driver_match_table plic_match_table[] = {
    { .compatible = "riscv,plic0", .match_data = &qemu_match_info },
    { NULL }
};

struct device_driver plic_driver = {
    .driver_name = "MaPl PLIC driver",
    .match_table = plic_match_table,
    .device_probe = plic_device_probe
};
