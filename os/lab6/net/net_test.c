#include <net/arp.h>
#include <net/ip.h>
#include <net/icmpv4.h>
#include <net/netdev.h>
#include <net/net_utils.h>
#include <mm.h>
#include <net/netdef.h>
#include <string.h>
#include <kdebug.h>
#include <net/skbuff.h>
#include <net/socket.h>
#include <net/timer.h>
#include <lib/sleep.h>

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
    uint32_t len = ETH_HDR_LEN + IP_HDR_LEN + sizeof(udp_packet); // 20 + 
    struct sk_buff *skb = alloc_skb(len);
    skb_reserve(skb, len);
    skb->protocol = IP_UDP;
    struct iphdr *ihdr = ip_hdr(skb);
    skb_push(skb, sizeof(udp_packet));
    memcpy(ihdr->data, udp_packet, sizeof(udp_packet));
    printbuf(skb_head(skb), len);
    // uint32_t dip[] = {172, 25, 16, 1}; 
    // uint32_t dip[] = {220, 181, 38, 148}; 
    uint32_t dip[] = {1, 1, 1, 1};
    extern struct netdev* netdev;
    ip_output(skb, iptoi(dip));
}

void ping(uint32_t daddr, uint32_t count) {
    for(int i = 1; i <= count; ++i)
        icmpv4_echo_request(daddr, i, "hello");
}

static void icmp_test() {
    kprintf("ping -c4 220.181.38.148\n");
    uint32_t dip[] = {220, 181, 38, 148};
    // kprintf("ping -c4 10.0.0.2\n");
    // uint32_t dip[] = {10, 0, 0, 2};
    // kprintf("ping -c4 172.26.0.1\n");
    // uint32_t dip[] = {172, 26, 0, 1};
    ping(iptoi(dip), 4);
}


static void udp_recv_test() {
    pid_t pid = 111;
    int sktfd = _socket(pid, AF_INET, SOCK_DGRAM, IP_UDP);
    struct sockaddr_in *addr = kmalloc(sizeof(struct sockaddr_in));
    int32_t dip[] = {10, 0, 0, 15};
    addr->sin_addr = htonl(iptoi(dip));
    addr->sin_port = htons(1234);
    addr->sin_family = AF_INET;
    _bind(pid, sktfd, addr);
    char buf[1024];

    kprintf("Waiting to receive data on port 1234...\n");
    _read(pid, sktfd, buf, 1024);
    kprintf("recv data : %s", buf);
    
}

static void udp_send_test() {
    pid_t pid = 111;
    int sktfd = _socket(pid, AF_INET, SOCK_DGRAM, IP_UDP);
    char *txt = "hello, world!";
    struct sockaddr_in *addr = kmalloc(sizeof(struct sockaddr_in));
    int32_t dip[] = {10, 0, 0, 2};
    addr->sin_addr = htonl(iptoi(dip));
    addr->sin_port = htons(1234);
    addr->sin_family = AF_INET;
    _sendto(pid, sktfd, txt, strlen(txt), addr);
    kprintf("Send \"hello, world!\" to 10.0.0.2:1234 by UDP\n");
}


static void time_test(uint32_t x, void *arg) {
    static int i = 0;
    kprintf("timer%u\n", i++);
    timer_add(300, &time_test, 0);
}

static void tcp_test() {
    int pid = 1;
    int sktfd = _socket(pid, AF_INET, SOCK_STREAM, IP_TCP);
    if(sktfd < 0) kprintf("socket failed");

    struct sockaddr_in *addr = kmalloc(sizeof(struct sockaddr_in));
    int32_t dip[] = {10, 0, 0, 15};
    addr->sin_addr = htonl(iptoi(dip));
    addr->sin_port = htons(1234);
    addr->sin_family = AF_INET;
    _bind(pid, sktfd, addr);

    kprintf("Waiting to connect on port 1234...\n");
    _listen(pid, sktfd, 5);

    struct sockaddr_in *addr_in = kmalloc(sizeof(struct sockaddr_in));
    memset(addr_in, 0, sizeof(struct sockaddr_in));
    int acfd = _accept(pid, sktfd, addr_in);

    char buf[1024];
    while(1) {
        memset(buf, 0, sizeof(buf));
        _read(pid, acfd, buf, sizeof(buf));
        kprintf("%s", buf);
        _write(pid, acfd, buf, strlen(buf));
    }
    _close(pid, acfd);
    _close(pid, sktfd);

}

uint64_t net_test() {
    // kprintf("----------arp_test----------\n");
    // arp_test();
    // kprintf("-----------ip_test----------\n");
    // ip_test();
    // kprintf("---------icmp_test----------\n");
    // icmp_test();
    // kprintf("--------udp_recv_test-------\n");
    // udp_recv_test();
    // kprintf("--------udp_send_test-------\n");
    // udp_send_test();
    // kprintf("---------timer_test---------\n");
    // timer_add(300, &time_test, 0);

    kprintf("----------tcp_test---------\n");
    tcp_test();
    return 0;
}

