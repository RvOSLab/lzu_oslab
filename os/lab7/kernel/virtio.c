#include <virtio.h>
#include <mm.h>
#include <stddef.h>
#include <kdebug.h>
#include <assert.h>

struct virtq virtio_queue;

void virtio_probe() {
    uint64_t ptr;
    uint64_t is_legacy;
    volatile struct virtio_device *device;

    for(ptr = VIRTIO_START_ADDRESS; ptr < VIRTIO_END_ADDRESS; ptr += VIRTIO_MMIO_STRIDE) {
        device = (volatile struct virtio_device *)ptr;
        if(device->magic_value == VIRTIO_MAGIC) {
            if(device->version == VIRTIO_VERSION || device->version == VIRTIO_VERSION_LEGACY) {
                is_legacy = device->version == VIRTIO_VERSION_LEGACY;
                kprintf("Virtio device is legacy 0x%x\n", is_legacy);
                if(device->device_id == VIRTIO_DEVICE_ID_BLOCK) {
                    virtio_blk_init(device, is_legacy);
                    break;
                }
            }
        }
    }
    if(ptr == VIRTIO_END_ADDRESS) {
        kprintf("Can not find any virtio-blk device, virtio init failed.\n");
        return;
    }
    virtio_test(device);
}

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

void virtio_queue_init(struct virtq* virtio_queue) {
    uint64_t virtq_vir_addr;
    uint64_t virtq_phy_addr;
    uint16_t idx;
    virtq_phy_addr = get_free_page();
    virtq_vir_addr = VIRTUAL(virtq_phy_addr);
    memset((uint8_t *)virtq_vir_addr, 0, VIRTQ_LENGTH);
    virtio_queue->num = VIRTQ_RING_NUM;
    virtio_queue->desc  = (struct virtq_desc *)  (virtq_vir_addr);
    virtio_queue->avail = (struct virtq_avail *) (virtq_vir_addr + VIRTQ_DESC_TABLE_LENGTH);
    virtio_queue->used  = (struct virtq_used *)  (virtq_vir_addr + VIRTQ_DESC_TABLE_LENGTH + VIRTQ_AVAIL_RING_LENGTH);
    virtio_queue->last_used_idx = 0;
    virtio_queue->physical_addr = virtq_phy_addr;
    for (idx = 0; idx < VIRTQ_RING_NUM - 1; ++idx) {
        virtio_queue->desc[idx].next = idx + 1;
    }
    virtio_queue->desc[idx].next = 0xff;
}

void virtio_set_queue(volatile struct virtio_device *device, uint64_t is_legacy, uint64_t virtq_phy_addr) {
    // 1. select the queue 0
    device->queue_sel = 0;
    if(!is_legacy) {
    // 2. check if the queue is not already in use
        assert(device->queue_ready == 0);
    }
    // 3. check the queue num max
    assert(device->queue_num_max >= VIRTQ_RING_NUM);
    // 4. allocate and zero the queue memory (see virtio_queue_init)
    // 5. set queue num
    device->queue_num = VIRTQ_RING_NUM;
    // 6. set the queue address
    if(is_legacy) {
        device->guest_page_size = PAGE_SIZE;
        device->queue_pfn = virtq_phy_addr / PAGE_SIZE;
    } else {
        device->queue_desc_low    = VIRTIO_ADDR_LOW32(virtq_phy_addr);
        device->queue_desc_high   = VIRTIO_ADDR_HIGH32(virtq_phy_addr);
        device->queue_driver_low  = VIRTIO_ADDR_LOW32(virtq_phy_addr + VIRTQ_DESC_TABLE_LENGTH);
        device->queue_driver_high = VIRTIO_ADDR_HIGH32(virtq_phy_addr + VIRTQ_DESC_TABLE_LENGTH);
        device->queue_device_low  = VIRTIO_ADDR_LOW32(virtq_phy_addr + VIRTQ_DESC_TABLE_LENGTH + VIRTQ_AVAIL_RING_LENGTH);
        device->queue_device_high = VIRTIO_ADDR_HIGH32(virtq_phy_addr + VIRTQ_DESC_TABLE_LENGTH + VIRTQ_AVAIL_RING_LENGTH);
    // 7. set queue ready
        device->queue_ready = 1;
    }
}

void virtio_test(volatile struct virtio_device *device) {
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