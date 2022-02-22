#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

#include <device/virtio/virtio_mmio.h>

#define VIRTIO_NET_F_MAC (1 << 5)

#define VIRTIO_NET_CONFIG_OFFSET 0x100
struct virtio_net_config {
    uint8_t mac[6];
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
};

#define VIRTIO_NET_HDR_F_NEEDS_CSUM 1
#define VIRTIO_NET_HDR_F_DATA_VALID 2
#define VIRTIO_NET_HDR_F_RSC_INFO 4

#define VIRTIO_NET_HDR_GSO_NONE 0
#define VIRTIO_NET_HDR_GSO_TCPV4 1
#define VIRTIO_NET_HDR_GSO_UDP 3
#define VIRTIO_NET_HDR_GSO_TCPV6 4
#define VIRTIO_NET_HDR_GSO_ECN 0x80

struct virtio_net_header {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
};

void virtio_net_init(struct virtio_device *device, uint64_t is_legacy);
void virtio_net_test(struct virtio_device *device);

#endif /* VIRTIO_NET_H */
