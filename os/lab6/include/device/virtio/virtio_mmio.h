#ifndef VIRTIO_MMIO_H
#define VIRTIO_MMIO_H


#include <stddef.h>
#include <device/virtio.h>
#include <device/virtio/virtio_queue.h>

#define VIRTIO_MAGIC 0x74726976
#define VIRTIO_VERSION_LEGACY 0x1
#define VIRTIO_VERSION 0x2

#define VIRTIO_DEVICE_ID_NETWORK 1
#define VIRTIO_DEVICE_ID_BLOCK   2
#define VIRTIO_DEVICE_ID_CONSOLE 3
#define VIRTIO_DEVICE_ID_INPUT   18

#define VIRTIO_ADDR_LOW32(addr)  ((uint32_t)((uint64_t)(addr) & 0xFFFFFFFF))
#define VIRTIO_ADDR_HIGH32(addr) ((uint32_t)((uint64_t)(addr) >> 32))

struct virtio_device {
    volatile uint32_t magic_value;          // 0x00
    volatile uint32_t version;              // 0x04
    volatile uint32_t device_id;            // 0x08
    volatile uint32_t vendor_id;            // 0x0c
    volatile uint32_t device_features;      // 0x10
    volatile uint32_t device_features_sel;  // 0x14
    volatile uint32_t padding_0[2];
    volatile uint32_t driver_features;      // 0x20
    volatile uint32_t driver_features_sel;  // 0x24
    volatile uint32_t guest_page_size;      // 0x28
    volatile uint32_t padding_1;
    volatile uint32_t queue_sel;            // 0x30
    volatile uint32_t queue_num_max;        // 0x34
    volatile uint32_t queue_num;            // 0x38
    volatile uint32_t queue_align;          // 0x3c
    volatile uint32_t queue_pfn;            // 0x40
    volatile uint32_t queue_ready;          // 0x44
    volatile uint32_t padding_2[2];
    volatile uint32_t queue_notify;         // 0x50
    volatile uint32_t padding_3[3];
    volatile uint32_t interrupt_status;     // 0x60
    volatile uint32_t interrupt_ack;        // 0x64
    volatile uint32_t padding_4[2];
    volatile uint32_t status;               // 0x70
    volatile uint32_t padding_5[3];
    volatile uint32_t queue_desc_low;       // 0x80
    volatile uint32_t queue_desc_high;      // 0x84
    volatile uint32_t padding_6[2];
    volatile uint32_t queue_driver_low;     // 0x90
    volatile uint32_t queue_driver_high;    // 0x94
    volatile uint32_t padding_7[2];
    volatile uint32_t queue_device_low;     // 0xa0
    volatile uint32_t queue_device_high;    // 0xa4
};

#define VIRTIO_STATUS_RESET       0
#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER      2
#define VIRTIO_STATUS_FAILED      128
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_DRIVER_OK   4
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET 64

void virtio_mmio_probe(struct device *dev);
void virtio_set_queue(struct virtio_device *device, uint64_t is_legacy, uint32_t queue_idx, uint64_t virtq_phy_addr);

#endif /* VIRTIO_MMIO */
