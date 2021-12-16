#include <virtio.h>
#include <virtio_blk.h>
#include <virtio_net.h>

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
                    virtio_blk_test(device);
                }
                if(device->device_id == VIRTIO_DEVICE_ID_NETWORK) {
                    virtio_net_init(device, is_legacy);
                    virtio_net_test(device);
                    break;
                }
            }
        }
    }
    if(ptr == VIRTIO_END_ADDRESS) {
        kprintf("Can not find any virtio-blk device, virtio init failed.\n");
        return;
    }   
}

void virtio_set_queue(volatile struct virtio_device *device, uint64_t is_legacy, uint32_t queue_idx, uint64_t virtq_phy_addr) {
    // 1. select the queue 0
    device->queue_sel = queue_idx;
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
