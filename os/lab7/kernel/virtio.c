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

    for(ptr = VIRTIO_START_ADDRESS; ptr < VIRTIO_END_ADDRESS; ptr += VIRTIO_STRIDE) {
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
    virtio_queue_init(device, is_legacy);
    blk_config = (volatile struct virtio_blk_config *)((uint64_t)device + VIRTIO_BLK_CONFIG_OFFSET);
    kprintf("virtio blk capacity: 0x%x\n", blk_config->capacity);
    kprintf("virtio blk size: 0x%x\n", blk_config->blk_size);
    // 8. set driver ok
    device->status |= VIRTIO_STATUS_DRIVER_OK;
}

void virtio_queue_init(volatile struct virtio_device *device, uint64_t is_legacy) {
    uint64_t page_ptr;
    uint64_t virtq_phy_addr;
    // 1. select the queue 0
    device->queue_sel = 0;
    if(!is_legacy) {
    // 2. check if the queue is not already in use
        assert(device->queue_ready == 0);
    }
    // 3. check the queue num max
    assert(device->queue_num_max >= VIRTIO_QUEUE_NUM);
    // 4. allocate and zero the queue memory
    page_ptr = VIRTIO_QUEUE_START_ADDRESS;
    virtq_phy_addr = get_empty_page(page_ptr , KERN_RW) - PAGE_SIZE;
    kprintf("virtq_phy_addr @ 0x%x - 0x%x\n", virtq_phy_addr, page_ptr);
    for(page_ptr + PAGE_SIZE; page_ptr < VIRTIO_QUEUE_END_ADDRESS; page_ptr += PAGE_SIZE) {
        get_empty_page(page_ptr, KERN_RW);
    }
    memset((uint8_t *)VIRTIO_QUEUE_START_ADDRESS, 0, VIRTIO_QUEUE_LENGTH);
    // 5. set queue num
    virtio_queue.num = VIRTIO_QUEUE_NUM;
    device->queue_num = VIRTIO_QUEUE_NUM;
    // 6. set the queue address
    virtio_queue.desc  = (struct virtq_desc *)  VIRTIO_QUEUE_START_ADDRESS;
    virtio_queue.avail = (struct virtq_avail *) (virtio_queue.desc  + VIRTIO_QUEUE_NUM);
    virtio_queue.used  = (struct virtq_used *) (VIRTIO_QUEUE_START_ADDRESS + 16 * VIRTIO_QUEUE_NUM + 6 + 2 * VIRTIO_QUEUE_NUM);
    if(is_legacy) {
        device->guest_page_size = PAGE_SIZE;
        device->queue_pfn = virtq_phy_addr / PAGE_SIZE;
    } else {
        device->queue_desc = virtq_phy_addr;
        device->queue_driver = virtq_phy_addr + 16 * VIRTIO_QUEUE_NUM;
        device->queue_device = virtq_phy_addr + 16 * VIRTIO_QUEUE_NUM + 6 + 2 * VIRTIO_QUEUE_NUM;
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
    uint8_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    kprintf("virtio_queue.desc @0x%x\n", virtio_queue.desc);
    kprintf("virtio_queue.avail@0x%x\n", virtio_queue.avail);
    kprintf("virtio_queue.used @0x%x\n", virtio_queue.used);
    kprintf("buffer @0x%x\n", buffer);
    virtio_queue.desc[0].addr = PHYSICAL(((uint64_t)&req));
    virtio_queue.desc[0].len = 16;
    virtio_queue.desc[0].flags = VIRTQ_DESC_F_NEXT;
    virtio_queue.desc[0].next = 1;

    virtio_queue.desc[1].addr = PHYSICAL(((uint64_t)buffer));
    virtio_queue.desc[1].len = 512;
    virtio_queue.desc[1].flags = VIRTQ_DESC_F_NEXT;
    virtio_queue.desc[1].next = 2;

    virtio_queue.desc[2].addr = PHYSICAL(((uint64_t)&(req.status)));
    virtio_queue.desc[2].len = sizeof(req.status);
    virtio_queue.desc[2].flags = VIRTQ_DESC_F_WRITE;
    virtio_queue.desc[2].next = 0;
    
    virtio_queue.avail->ring[virtio_queue.avail->idx % VIRTIO_QUEUE_NUM] = 0;
    __sync_synchronize();
    virtio_queue.avail->idx += 1;
    __sync_synchronize();
    device->queue_notify = 0;
    while(req.status == 111);
    if(req.status != 0) {
        kprintf("virtio_test failed $ 0x%x * 0x%x\n", req.status, virtio_queue.used->ring[virtio_queue.used->idx-1].len);
    } else {
        kprintf("virtio_test ok $ 0x%x * 0x%x\n", req.status, virtio_queue.used->ring[virtio_queue.used->idx-1].len);
    }
    for (int i = 0; i < 16; i++)
    {
        if(!(buffer[i] & 0xF0)) kprintf("0");
        kprintf("%x ", buffer[i]);
    }
    kprintf("\n");    
}