#include <net/arp.h>
#include <net/ip.h>
#include <net/icmpv4.h>
#include <net/netdev.h>
#include <net/net_utils.h>
#include <mm.h>
#include <net/netdef.h>
#include <string.h>
#include <kdebug.h>

static void arp_test() {
    uint32_t dip[] = {10, 0, 0, 2};
    uint32_t sip[] = {10, 0, 0, 15};
    struct netdev *netdev = netdev_get(iptoi(sip));
    arp_request(iptoi(sip), iptoi(dip), netdev);
}

static void ip_test() {
    uint8_t udp_packet[] = {
        0x16, 0x2e, 0x04, 0xd2, // UDP dst port 5678(12 6e) UDP src port 1234(04 d2)
        0x00, 0x0c, 0x1e, 0x6c, // UDP length 12(8 + 4)
        0x74, 0x65, 0x73, 0x74  // payload "test"
    };
    uint32_t len = IP_HDR_LEN + sizeof(udp_packet); // 20 + 
    uint8_t *buffer = kmalloc(len);
    struct iphdr *ihdr = ip_hdr(buffer);
    memcpy(ihdr->data, udp_packet, sizeof(udp_packet));
    printbuf(buffer, ETH_HDR_LEN + len);
    // uint32_t dip[] = {172, 25, 16, 1}; 
    // uint32_t dip[] = {220, 181, 38, 148}; 
    uint32_t dip[] = {1, 1, 1, 1};
    extern struct netdev* netdev;
    ip_output(buffer, iptoi(dip), IP_UDP, netdev, len);
}

void ping(uint32_t daddr, uint32_t count) {
    for(int i = 1; i <= count; ++i)
        icmpv4_echo_request(daddr, i, "hello");
}

static void icmp_test() {
    kprintf("ping -c4 10.0.0.2\n");
    uint32_t dip[] = {10,0,0,2};
    ping(iptoi(dip), 4);
}

uint64_t net_test() {
    // arp_test();
    // ip_test();
    icmp_test();
    return 0;
}

