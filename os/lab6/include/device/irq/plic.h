#ifndef PLIC_H
#define PLIC_H

#include <device/irq.h>
#include <utils/hash_table.h>

#define PLIC_MAJOR 0x5678

#define PLIC_PRIORITY_BASE 0x0
#define PLIC_PENDING_BASE 0x1000
#define PLIC_ENABLE_BASE 0x2000
#define PLIC_ENABLE_STRIDE 0x80
#define PLIC_CONTEXT_BASE 0x200000
#define PLIC_CONTEXT_STRIDE 0x1000

#define PLIC_THRESHOLD_OFFSET 0
#define PLIC_CLAIM_OFFSET 4
#define PLIC_COMPLETE_OFFSET 4

struct plic_match_data {
    uint64_t mmio_start_addr;
    uint64_t mmio_end_addr;
    uint64_t num_sources;
    uint64_t num_priorities;
};

#define PLIC_HANDLER_BUFFER_LENGTH 11
struct plic_handler {
    uint32_t irq_id;
    struct irq_descriptor* descriptor;

    struct hash_table_node hash_node;
};


extern struct device_driver plic_driver;

#endif