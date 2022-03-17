#include <net/arp.h>
#include <net/netdev.h>
#include <net/net_utils.h>


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

