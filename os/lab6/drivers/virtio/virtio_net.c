#include <device/virtio/virtio_net.h>
#include <device/irq.h>

#include <kdebug.h>
#include <assert.h>
#include <sched.h>
#include <mm.h>


struct virtio_net_header rx_packet_header;
uint8_t rx_buffer[1500];

void virtio_net_recv(struct device *dev) {
    struct virtio_net_data *data = device_get_data(dev);
    struct virtio_device *device = data->virtio_device;
    struct virtq *virtio_net_queue = &data->virtio_net_rx_queue;

    uint16_t idx, head;
    head = idx = virtq_get_desc(virtio_net_queue);
    assert(idx != 0xff);
    virtio_net_queue->desc[idx].addr = PHYSICAL(((uint64_t)&rx_packet_header));
    virtio_net_queue->desc[idx].len = sizeof(struct virtio_net_header);
    virtio_net_queue->desc[idx].flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
    idx = virtq_get_desc(virtio_net_queue);
    assert(idx != 0xff);
    virtio_net_queue->desc[idx].addr = PHYSICAL(((uint64_t)rx_buffer));
    virtio_net_queue->desc[idx].len = 1500;
    virtio_net_queue->desc[idx].flags = VIRTQ_DESC_F_WRITE;
    virtio_net_queue->desc[idx].next = 0;
    
    virtq_put_avail(virtio_net_queue, head);
    device->queue_notify = 0;
};

void virtio_net_irq_handler(struct device *dev) {
    struct virtio_net_data *data = device_get_data(dev);
    uint32_t interrupt_status = data->virtio_device->interrupt_status;
    struct virtq *virtio_net_queue = &data->virtio_net_rx_queue;
    struct virtq_used_elem *used_elem = virtq_get_used_elem(virtio_net_queue);
    
    while (used_elem) {
        virtio_net_recv(dev);
        uint64_t used_len = used_elem->len - sizeof(struct virtio_net_header);
        kprintf("virtio-net: recv %u bits\n    ", used_len);
        for (uint64_t i = 0; i < used_len; i += 1) {
            if(rx_buffer[i] < 0x10) kprintf("0");
            kprintf("%x ", rx_buffer[i]);
            if(i%8 == 7) kprintf(" ");
            if(i%16 == 15) kprintf("\n    ");
        }
        kprintf("\n");
        virtq_free_desc_chain(virtio_net_queue, used_elem->id);
        used_elem = virtq_get_used_elem(virtio_net_queue);
    }

    data->virtio_device->interrupt_ack = interrupt_status;
}

struct irq_descriptor virtio_net_irq = {
    .name = "virtio_net rx irq handler",
    .handler = virtio_net_irq_handler
};

void virtio_net_config(struct virtio_net_data *data, uint64_t is_legacy) {
    struct virtio_device *device = data->virtio_device;
    struct virtq *virtio_net_tx_queue = &data->virtio_net_tx_queue;
    struct virtq *virtio_net_rx_queue = &data->virtio_net_rx_queue;

    struct virtio_net_config *net_config;
    // 1. reset device
    device->status = VIRTIO_STATUS_RESET;
    // 2. set acknowledge
    device->status |= VIRTIO_STATUS_ACKNOWLEDGE;
    // 3. set driver
    device->status |= VIRTIO_STATUS_DRIVER;
    // 4. config features
    uint32_t features = device->device_features;
    kprintf("virtio_net: supported features: %x\n", features);
    if(!(features & VIRTIO_NET_F_MAC)) {
        panic("virtio_net device can not provide mac address");
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
            panic("virtio_net device does not support features");
        }
    }
    // 7. perform device-specific setup
    virtio_queue_init(virtio_net_rx_queue, is_legacy);
    virtio_queue_init(virtio_net_tx_queue, is_legacy);
    virtio_net_tx_queue->used->flags |= VIRTQ_USED_F_NO_NOTIFY;
    virtio_set_queue(device, is_legacy, 0, virtio_net_rx_queue->physical_addr);
    virtio_set_queue(device, is_legacy, 1, virtio_net_tx_queue->physical_addr);
    
    net_config = (struct virtio_net_config *)((uint64_t)device + VIRTIO_NET_CONFIG_OFFSET);
    kprintf(
        "virtio_net: mac: %x:%x:%x:%x:%x:%x\n",
        net_config->mac[0],
        net_config->mac[1],
        net_config->mac[2],
        net_config->mac[3],
        net_config->mac[4],
        net_config->mac[5]
    );
    // 8. set driver ok
    device->status |= VIRTIO_STATUS_DRIVER_OK;
}

void virtio_net_send(struct device *dev, void *buffer, uint64_t length) {
    struct virtio_net_data *data = device_get_data(dev);
    struct virtio_device *device = data->virtio_device;
    struct virtq *virtio_net_queue = &data->virtio_net_tx_queue;

    struct virtio_net_header packet_header = {
        .flags = 0,
        .gso_type = VIRTIO_NET_HDR_GSO_NONE
    };

    uint16_t idx, head;
    head = idx = virtq_get_desc(virtio_net_queue);
    assert(idx != 0xff);
    virtio_net_queue->desc[idx].addr = PHYSICAL(((uint64_t)&packet_header));
    virtio_net_queue->desc[idx].len = sizeof(struct virtio_net_header);
    virtio_net_queue->desc[idx].flags = VIRTQ_DESC_F_NEXT;
    idx = virtq_get_desc(virtio_net_queue);
    assert(idx != 0xff);
    virtio_net_queue->desc[idx].addr = PHYSICAL(((uint64_t)buffer));
    virtio_net_queue->desc[idx].len = length;
    virtio_net_queue->desc[idx].flags = 0;
    virtio_net_queue->desc[idx].next = 0;
    
    virtq_put_avail(virtio_net_queue, head);
    device->queue_notify = 1;
    struct virtq_used_elem *used_elem = NULL;
    while(!used_elem) {
        used_elem = virtq_get_used_elem(virtio_net_queue);
    }
    virtq_free_desc_chain(virtio_net_queue, used_elem->id);
}

struct net_device virtio_net_device = {
    .send = virtio_net_send
};

void *virtio_net_get_interface(struct device *dev, uint64_t flag) {
    if(flag & NET_INTERFACE_BIT) return &virtio_net_device;
    return NULL;
}

void virtio_net_init(struct device *dev, struct virtio_device *device, uint64_t is_legacy) {
    struct virtio_net_data *data = kmalloc(sizeof(struct virtio_net_data));
    memset(data, 0, sizeof(struct virtio_net_data));
    data->virtio_device = device;
    device_set_data(dev, data);
    virtio_net_config(data, is_legacy);
    
    struct fdt_header *fdt = device_get_fdt(dev);
    struct fdt_node_header * node = device_get_fdt_node(dev);
    struct fdt_property *prop = fdt_get_prop(fdt, node, "interrupts");
    uint32_t irq_id = fdt_get_prop_num_value(prop, 0);
    virtio_net_irq.dev = dev;
    irq_add(0, irq_id, &virtio_net_irq);

    virtio_net_recv(dev);
}

uint64_t virtio_net_device_probe(struct device *dev, struct virtio_device *device, uint64_t is_legacy) {
    virtio_net_init(dev, device, is_legacy);
    virtio_net_device.dev = dev;
    device_set_interface(dev, NET_INTERFACE_BIT, virtio_net_get_interface);
    device_register(dev, "virtio-net", VIRTIO_MAJOR, NULL);
    return 0;
}
