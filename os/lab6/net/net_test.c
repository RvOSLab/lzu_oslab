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
    arp_test();
    return 0;
}

