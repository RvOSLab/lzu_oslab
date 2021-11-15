#include <virtio_blk.h>
#include <virtio_queue.h>

#include <kdebug.h>
#include <assert.h>

volatile struct virtio_device *blk_device;
struct virtq virtio_blk_queue;

void virtio_blk_init(volatile struct virtio_device *device, uint64_t is_legacy) {
    volatile struct virtio_blk_config *blk_config;
    // 1. reset device
    device->status = VIRTIO_STATUS_RESET;
    // 2. set acknowledge
    device->status |= VIRTIO_STATUS_ACKNOWLEDGE;
    // 3. set driver
    device->status |= VIRTIO_STATUS_DRIVER;
    // 4. config features
    uint32_t features = device->device_features;
    kprintf("supported features: %x\n", features);
    if(features & VIRTIO_BLK_F_RO) {
        panic("virtio blk device is read-only");
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
            panic("virtio blk device does not support features");
        }
    }
    // 7. perform device-specific setup
    virtio_queue_init(&virtio_blk_queue);
    virtio_set_queue(device, is_legacy, virtio_blk_queue.physical_addr);
    blk_config = (volatile struct virtio_blk_config *)((uint64_t)device + VIRTIO_BLK_CONFIG_OFFSET);
    kprintf("virtio blk capacity: 0x%x\n", blk_config->capacity);
    kprintf("virtio blk size: 0x%x\n", blk_config->blk_size);
    // 8. set driver ok
    device->status |= VIRTIO_STATUS_DRIVER_OK;

    blk_device = device;
}

uint8_t virtio_blk_rw(uint8_t* buffer, uint64_t sector, uint64_t is_write) {
    struct virtio_blk_req req = {
        .type = (is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN),
        .reserved = 0,
        .sector = sector,
        .status = 0
    };
    kprintf("virtio_blk_queue.desc @ 0x%x\n", virtio_blk_queue.desc);
    uint16_t idx, head;
    head = idx = virtq_get_desc(&virtio_blk_queue);
    assert(idx != 0xff);
    virtio_blk_queue.desc[idx].addr = PHYSICAL(((uint64_t)&req));
    virtio_blk_queue.desc[idx].len = 16;
    virtio_blk_queue.desc[idx].flags = VIRTQ_DESC_F_NEXT;
    idx = virtq_get_desc(&virtio_blk_queue);
    assert(idx != 0xff);
    virtio_blk_queue.desc[idx].addr = PHYSICAL(((uint64_t)buffer));
    virtio_blk_queue.desc[idx].len = 512;
    virtio_blk_queue.desc[idx].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
    idx = virtq_get_desc(&virtio_blk_queue);
    assert(idx != 0xff);
    virtio_blk_queue.desc[idx].addr = PHYSICAL(((uint64_t)&(req.status)));
    virtio_blk_queue.desc[idx].len = sizeof(req.status);
    virtio_blk_queue.desc[idx].flags = VIRTQ_DESC_F_WRITE;
    virtio_blk_queue.desc[idx].next = 0;
    
    virtq_put_avail(&virtio_blk_queue, head);
    blk_device->queue_notify = 0;
    struct virtq_used_elem *used_elem = NULL;
    while (!used_elem) {
        used_elem = virtq_get_used_elem(&virtio_blk_queue);
    }
    while (used_elem) {
        virtq_free_desc_chain(&virtio_blk_queue, used_elem->id);
        used_elem = virtq_get_used_elem(&virtio_blk_queue);
    }

    return req.status;
}

void virtio_blk_test(volatile struct virtio_device *device) {
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < 16; i++) {
        buffer[i] = i;
    }
    
    if(virtio_blk_rw(buffer, 0, 1) != 0) {
        kprintf("virtio_blk_test write failed\n");
    } else {
        kprintf("virtio_blk_test write ok\n");
    }
    if(virtio_blk_rw(buffer, 0, 0) != 0) {
        kprintf("virtio_blk_test read failed\n");
    } else {
        kprintf("virtio_blk_test read ok\n");
    }
    
    for (int i = 0; i < 16; i++) {
        if(!(buffer[i] & 0xF0)) kprintf("0");
        kprintf("%x ", buffer[i]);
    }
    kprintf("\n");    
}
