#include <device/virtio/virtio_mmio.h>
#include <device/virtio/virtio_blk.h>
#include <device/virtio/virtio_net.h>

#include <kdebug.h>
#include <assert.h>
#include <mm.h>

void virtio_mmio_probe(struct device *dev) {
    struct driver_resource *mem = (struct driver_resource *)device_get_data(dev);
    struct virtio_device *device = (struct virtio_device *)mem->map_address;
    if(device->magic_value == VIRTIO_MAGIC) {
        if(device->version == VIRTIO_VERSION || device->version == VIRTIO_VERSION_LEGACY) {
            uint64_t is_legacy = device->version == VIRTIO_VERSION_LEGACY;
            if (is_legacy) kprintf("virtio: device is legacy\n");
            if(device->device_id == VIRTIO_DEVICE_ID_BLOCK) {
                virtio_block_device_probe(dev, device, is_legacy);
            }
            if(device->device_id == VIRTIO_DEVICE_ID_NETWORK) {
                virtio_net_init(device, is_legacy);
                virtio_net_test(device);
            }
        }
    } 
}

void virtio_set_queue(struct virtio_device *device, uint64_t is_legacy, uint32_t queue_idx, uint64_t virtq_phy_addr) {
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
        device->queue_align = PAGE_SIZE;
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
