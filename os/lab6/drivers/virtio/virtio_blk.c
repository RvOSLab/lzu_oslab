#include <device/virtio/virtio_blk.h>
#include <device/irq.h>

#include <kdebug.h>
#include <assert.h>
#include <sched.h>
#include <mm.h>

uint64_t virtio_blk_get_hash(struct hash_table_node *node) {
    struct virtio_blk_qmap *qmap = container_of(node, struct virtio_blk_qmap, hash_node);
    return (qmap->desp_idx * 0x10001) + 19;
}

uint64_t virtio_blk_is_equal(struct hash_table_node *nodeA, struct hash_table_node *nodeB) {
    struct virtio_blk_qmap *qmapA = container_of(nodeA, struct virtio_blk_qmap, hash_node);
    struct virtio_blk_qmap *qmapB = container_of(nodeB, struct virtio_blk_qmap, hash_node);
    return  qmapA->desp_idx == qmapB->desp_idx;
}

#define VIRTIO_BLK_BUFFER_LENGTH 13
struct hash_table_node virtio_blk_buffer[VIRTIO_BLK_BUFFER_LENGTH];
struct hash_table virtio_blk_table = {
    .buffer = virtio_blk_buffer,
    .buffer_length = VIRTIO_BLK_BUFFER_LENGTH,
    .get_hash = virtio_blk_get_hash,
    .is_equal = virtio_blk_is_equal
};

void virtio_block_irq_handler(struct device *dev) {
    struct virtio_blk_data *data = device_get_data(dev);
    struct virtq *virtio_blk_queue = &data->virtio_blk_queue;
    uint32_t interrupt_status = data->virtio_device->interrupt_status;

    struct virtq_used_elem *used_elem = virtq_get_used_elem(virtio_blk_queue);
    while (used_elem) {
        struct virtio_blk_qmap qmap_search = {
            .desp_idx = used_elem->id
        };
        virtq_free_desc_chain(virtio_blk_queue, used_elem->id);
        struct hash_table_node *node = hash_table_get(&virtio_blk_table, &qmap_search.hash_node);
        struct virtio_blk_qmap * qmap = container_of(node, struct virtio_blk_qmap, hash_node);
        wake_up(&qmap->request->wait_queue);
        hash_table_del(&virtio_blk_table, &qmap->hash_node);
        used_elem = virtq_get_used_elem(virtio_blk_queue);
    }
    data->virtio_device->interrupt_ack = interrupt_status;
}

struct irq_descriptor virtio_block_irq = {
    .name = "virtio_block irq handler",
    .handler = virtio_block_irq_handler
};

void virtio_blk_config(struct virtio_blk_data *data, uint64_t is_legacy) {
    struct virtio_device *device = data->virtio_device;
    struct virtq *virtio_blk_queue = &data->virtio_blk_queue;

    struct virtio_blk_config *blk_config;
    // 1. reset device
    device->status = VIRTIO_STATUS_RESET;
    // 2. set acknowledge
    device->status |= VIRTIO_STATUS_ACKNOWLEDGE;
    // 3. set driver
    device->status |= VIRTIO_STATUS_DRIVER;
    // 4. config features
    uint32_t features = device->device_features;
    kprintf("virtio_blk: supported features: %x\n", features);
    if(features & VIRTIO_BLK_F_RO) {
        panic("virtio_blk device is read-only");
    }
    if(!is_legacy) {
        features = (
            0
        );
        device->driver_features = features;
    // 5. set features ok
        device->status |= VIRTIO_STATUS_FEATURES_OK;
    // 6. check features ok
        if(!(device->status & VIRTIO_STATUS_FEATURES_OK)) {
            device->status |= VIRTIO_STATUS_FAILED;
            panic("virtio_blk device does not support features");
        }
    }
    // 7. perform device-specific setup
    virtio_queue_init(virtio_blk_queue, is_legacy);
    virtio_set_queue(device, is_legacy, 0, virtio_blk_queue->physical_addr);
    blk_config = (struct virtio_blk_config *)((uint64_t)device + VIRTIO_BLK_CONFIG_OFFSET);
    kprintf("virtio_blk: capacity: 0x%x\n", blk_config->capacity);
    kprintf("virtio_blk: size: 0x%x\n", blk_config->blk_size);
    // 8. set driver ok
    device->status |= VIRTIO_STATUS_DRIVER_OK;
}

void virtio_block_request(struct device *dev, struct block_request *request) {
    struct virtio_blk_data *data = device_get_data(dev);
    struct virtio_device *device = data->virtio_device;
    struct virtq *virtio_blk_queue = &data->virtio_blk_queue;

    struct virtio_blk_req req = {
        .type = request->is_read ? VIRTIO_BLK_T_IN : VIRTIO_BLK_T_OUT,
        .reserved = 0,
        .sector = request->sector,
        .status = 0
    };

    uint16_t idx, head;
    head = idx = virtq_get_desc(virtio_blk_queue);
    assert(idx != 0xff);
    virtio_blk_queue->desc[idx].addr = PHYSICAL(((uint64_t)&req));
    virtio_blk_queue->desc[idx].len = 16;
    virtio_blk_queue->desc[idx].flags = VIRTQ_DESC_F_NEXT;
    idx = virtq_get_desc(virtio_blk_queue);
    assert(idx != 0xff);
    virtio_blk_queue->desc[idx].addr = PHYSICAL(((uint64_t)request->buffer));
    virtio_blk_queue->desc[idx].len = 512;
    virtio_blk_queue->desc[idx].flags = VIRTQ_DESC_F_NEXT | (request->is_read ? VIRTQ_DESC_F_WRITE : 0);
    idx = virtq_get_desc(virtio_blk_queue);
    assert(idx != 0xff);
    virtio_blk_queue->desc[idx].addr = PHYSICAL(((uint64_t)&(req.status)));
    virtio_blk_queue->desc[idx].len = sizeof(req.status);
    virtio_blk_queue->desc[idx].flags = VIRTQ_DESC_F_WRITE;
    virtio_blk_queue->desc[idx].next = 0;
    
    struct virtio_blk_qmap qmap = {
        .desp_idx = head,
        .request = request
    };
    hash_table_set(&virtio_blk_table, &qmap.hash_node);
    
    virtq_put_avail(virtio_blk_queue, head);
    device->queue_notify = 0;

    sleep_on(&request->wait_queue);
}

struct block_device virtio_block_device = {
    .request = virtio_block_request
};

void *virtio_block_get_interface(struct device *dev, uint64_t flag) {
    if(flag & BLOCK_INTERFACE_BIT) return &virtio_block_device;
    return NULL;
}

void virtio_block_init(struct device *dev, struct virtio_device *device, uint64_t is_legacy) {
    struct virtio_blk_data *data = kmalloc(sizeof(struct virtio_blk_data));
    memset(data, 0, sizeof(struct virtio_blk_data));
    data->virtio_device = device;
    device_set_data(dev, data);
    virtio_blk_config(data, is_legacy);
    hash_table_init(&virtio_blk_table);
    
    struct fdt_header *fdt = device_get_fdt(dev);
    struct fdt_node_header * node = device_get_fdt_node(dev);
    struct fdt_property *prop = fdt_get_prop(fdt, node, "interrupts");
    uint32_t irq_id = fdt_get_prop_num_value(prop, 0);
    virtio_block_irq.dev = dev;
    irq_add(0, irq_id, &virtio_block_irq);
}

uint64_t virtio_block_device_probe(struct device *dev, struct virtio_device *device, uint64_t is_legacy) {
    virtio_block_init(dev, device, is_legacy);
    virtio_block_device.dev = dev;
    device_set_interface(dev, BLOCK_INTERFACE_BIT, virtio_block_get_interface);
    device_register(dev, "virtio-block", VIRTIO_MAJOR, NULL);
    return 0;
}
