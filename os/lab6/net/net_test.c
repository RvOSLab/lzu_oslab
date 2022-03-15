#include <net/netdev.h>
#include <net/arp.h>

int32_t iptoi(uint32_t *ip) {
    return (ip[0] << 24) + (ip[1] << 16) + (ip[2] << 8) + ip[3];
}

static void arp_test() {
    uint32_t dip[] = {10, 0, 0, 2};
    uint32_t sip[] = {10, 0, 0, 15};
    struct netdev *netdev = netdev_get(iptoi(sip));
    arp_request(iptoi(sip), iptoi(dip), netdev);
}


uint64_t net_test() {
    uint8_t arp_packet[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56,
        0x08, 0x06,
        0x00, 0x01,
        0x08, 0x00,
        0x06, 0x04,
        0x00, 0x01,
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56,
        10, 0, 0, 15, // src ip
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        10, 0, 0, 2   // dst ip
    };
    // netdev_send(arp_packet, sizeof(arp_packet));
    printbuf(arp_packet, 42);
    arp_test();
    return 0;
}

