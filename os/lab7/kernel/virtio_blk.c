#include <virtio_blk.h>

#include <kdebug.h>
#include <assert.h>

struct virtq virtio_queue;

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
    virtio_queue_init(&virtio_queue);
    virtio_set_queue(device, is_legacy, virtio_queue.physical_addr);
    blk_config = (volatile struct virtio_blk_config *)((uint64_t)device + VIRTIO_BLK_CONFIG_OFFSET);
    kprintf("virtio blk capacity: 0x%x\n", blk_config->capacity);
    kprintf("virtio blk size: 0x%x\n", blk_config->blk_size);
    // 8. set driver ok
    device->status |= VIRTIO_STATUS_DRIVER_OK;
}

void virtio_blk_test(volatile struct virtio_device *device) {
    struct virtio_blk_req req = {
        .type = VIRTIO_BLK_T_OUT,
        .reserved = 0,
        .sector = 0,
        .status = 111
    };
    kprintf("req @ 0x%x - %x\n", PHYSICAL(((uint64_t)&req)), &req);
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < 16; i++) {
        buffer[i] = i;
    }
    uint16_t idx, head;
    head = idx = virtq_get_desc(&virtio_queue);
    assert(idx != 0xff);
    kprintf("idx = %x\n", idx);
    virtio_queue.desc[idx].addr = PHYSICAL(((uint64_t)&req));
    virtio_queue.desc[idx].len = 16;
    virtio_queue.desc[idx].flags = VIRTQ_DESC_F_NEXT;
    idx = virtq_get_desc(&virtio_queue);
    assert(idx != 0xff);
    kprintf("idx = %x\n", idx);
    virtio_queue.desc[idx].addr = PHYSICAL(((uint64_t)buffer));
    virtio_queue.desc[idx].len = 512;
    virtio_queue.desc[idx].flags = VIRTQ_DESC_F_NEXT;
    idx = virtq_get_desc(&virtio_queue);
    assert(idx != 0xff);
    kprintf("idx = %x\n", idx);
    virtio_queue.desc[idx].addr = PHYSICAL(((uint64_t)&(req.status)));
    virtio_queue.desc[idx].len = sizeof(req.status);
    virtio_queue.desc[idx].flags = VIRTQ_DESC_F_WRITE;
    virtio_queue.desc[idx].next = 0;
    
    virtq_put_avail(&virtio_queue, head);
    device->queue_notify = head;
    while(req.status == 111);
    if(req.status != 0) {
        kprintf("virtio_test failed $ 0x%x\n", req.status);
    } else {
        kprintf("virtio_test ok $ 0x%x\n", req.status);
    }
    struct virtq_used_elem *used_elem;
    while (used_elem = virtq_get_used(&virtio_queue)) {
        kprintf("used_elem @ 0x%x - %x\n", PHYSICAL((uint64_t)used_elem), used_elem);
        kprintf("used_elem->id = 0x%x\n", used_elem->id);
        kprintf("used_elem->len = 0x%x\n", used_elem->len);
    }
    
    for (int i = 0; i < 16; i++) {
        if(!(buffer[i] & 0xF0)) kprintf("0");
        kprintf("%x ", buffer[i]);
    }
    kprintf("\n");    
}