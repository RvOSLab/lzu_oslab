#include <device/virtio/virtio_net.h>
#include <device/virtio/virtio_queue.h>

#include <kdebug.h>
#include <assert.h>
#include <mm.h>

volatile struct virtio_device *net_device;
struct virtq virtio_net_queue_rx;
struct virtq virtio_net_queue_tx;

void virtio_net_init(struct virtio_device *device, uint64_t is_legacy) {
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
    virtio_queue_init(&virtio_net_queue_rx, is_legacy);
    virtio_queue_init(&virtio_net_queue_tx, is_legacy);
    virtio_set_queue(device, is_legacy, 0, virtio_net_queue_rx.physical_addr);
    virtio_set_queue(device, is_legacy, 1, virtio_net_queue_tx.physical_addr);
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

    net_device = device;
}

void virtio_net_test(struct virtio_device *device) {
    struct virtio_net_header packet_header = {
        .flags = 0,
        .gso_type = VIRTIO_NET_HDR_GSO_NONE
    };
    uint8_t arp_packet[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Eth dst ff:ff:ff:ff:ff:ff
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56, // Eth src 52:54:00:12:34:56
        0x08, 0x00,             // IPv4 packet
        0x45, 0x00, 0x00, 0x20, // IP version = 4 Header length = 5 Length = 32 bytes
        0x00, 0x01, 0x00, 0x00, // IPv4 type: UDP
        0x40, 0x11, 0x9c, 0x3c, // IPv4 TTL = 64
        0xac, 0x13, 0x30, 0x7b, // IPv4 src 172.19.48.123
        0x01, 0x01, 0x01, 0x01, // IPv4 dst 1.1.1.1
        0x16, 0x2e, 0x04, 0xd2, // UDP dst port 5678(12 6e) UDP src port 1234(04 d2)
        0x00, 0x0c, 0x1e, 0x6c, // UDP length 12(8 + 4)
        0x74, 0x65, 0x73, 0x74  // payload "test"
    };
    uint8_t bufferA[2000];
    uint8_t bufferB[2000];
    uint8_t *buffer;
    uint64_t buffer_idx = 0;

    uint16_t idx, head;
    struct virtq_used_elem *used_elem = NULL;
    while (1) {
        /* for tx */
        head = idx = virtq_get_desc(&virtio_net_queue_tx);
        assert(idx != 0xff);
        virtio_net_queue_tx.desc[idx].addr = PHYSICAL(((uint64_t)&packet_header));
        virtio_net_queue_tx.desc[idx].len = sizeof(struct virtio_net_header);
        virtio_net_queue_tx.desc[idx].flags = VIRTQ_DESC_F_NEXT;
        idx = virtq_get_desc(&virtio_net_queue_tx);
        assert(idx != 0xff);
        
        virtio_net_queue_tx.desc[idx].addr = PHYSICAL(((uint64_t)arp_packet));
        virtio_net_queue_tx.desc[idx].len = 46;
        virtio_net_queue_tx.desc[idx].flags = 0;
        virtio_net_queue_tx.desc[idx].next = 0;
        virtq_put_avail(&virtio_net_queue_tx, head);
        device->queue_notify = 1;
        kprintf("virtio_net: added tx packet\n");
        while (!used_elem) { // wait for use
            used_elem = virtq_get_used_elem(&virtio_net_queue_tx);
        }
        kprintf("virtio_net: tx packet has gone\n");
        while (used_elem) { // free desp
            kprintf("virtio_net: free used elem\n");
            virtq_free_desc_chain(&virtio_net_queue_tx, used_elem->id);
            used_elem = virtq_get_used_elem(&virtio_net_queue_tx); // next chain
        }
        /* for rx */
        buffer = ((buffer_idx & 1) ? bufferA : bufferB);
        head = idx = virtq_get_desc(&virtio_net_queue_rx);
        assert(idx != 0xff);
        virtio_net_queue_rx.desc[idx].addr = PHYSICAL(((uint64_t)buffer));
        virtio_net_queue_rx.desc[idx].len = 2000;
        virtio_net_queue_rx.desc[idx].flags = VIRTQ_DESC_F_WRITE;
        virtio_net_queue_rx.desc[idx].next = 0;
        virtq_put_avail(&virtio_net_queue_rx, head);
        device->queue_notify = 0;
        kprintf("virtio_net: added rx buffer\n");
        while (!used_elem) { // wait for use
            used_elem = virtq_get_used_elem(&virtio_net_queue_rx);
        }
        kprintf("virtio_net: received a packet\n");
        while (used_elem) { // free desp
            if(buffer[12] == 0x08 || 1) {
                uint64_t used_len = used_elem->len;
                for (uint64_t i = 0; i < used_len; i += 1) {
                    if(buffer[i] < 0x10) kprintf("0");
                    kprintf("%x ", buffer[i]);
                    if(i%8 == 7) kprintf(" ");
                    if(i%16 == 15) kprintf("\n");
                }
                kprintf("\n");
            }
            virtq_free_desc_chain(&virtio_net_queue_rx, used_elem->id);
            used_elem = virtq_get_used_elem(&virtio_net_queue_rx); // next chain
        }
        buffer_idx += 1;
    }
}
