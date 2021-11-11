#include <virtio.h>
#include <virtio_blk.h>

#include <mm.h>
#include <stddef.h>
#include <kdebug.h>
#include <assert.h>

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
    virtio_blk_test(device);
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
