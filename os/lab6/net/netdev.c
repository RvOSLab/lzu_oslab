#include <net/netdev.h>
#include <net/arp.h>
#include <net/ip.h>
#include <device/virtio/virtio_net.h>
#include <net/netdef.h>
#include <net/ethernet.h>
#include <kdebug.h>
#include <mm.h>
#include <string.h>
#include <net/net_utils.h>
#include <net/skbuff.h>
#include <net/socket.h>
#include <net/timer.h>
#include <net/udp.h>
#include <net/tcp.h>

struct netdev *loop;
struct netdev *netdev; /* 用于记录本机地址,包括ip和mac地址 */

//
// addr表示ip地址, hwadddr表示mac地址, mtu表示最大传输单元的大小
static struct netdev *netdev_alloc(uint32_t *addr, uint8_t* hwaddr, uint32_t mtu) {
	/* hwaddr表示硬件地址 */
	struct netdev *dev = kmalloc(sizeof(struct netdev));
	dev->addr = (addr[0] << 24) + (addr[1] << 16) + (addr[2] << 8) + addr[3];		/* 记录下ip地址 */

	for(int i = 0; i < 6; ++i) {      /* 记录下mac地址 */
        dev->hwaddr[i] = hwaddr[i];
    }				

	dev->addr_len = 6;					/* 地址长度 */
	dev->mtu = mtu;						/* 最大传输单元 */
	return dev;
}


void netdev_init() {
	/* 本地环回地址 */
    uint32_t loop_ip[] = {127, 0, 0, 1};
    uint8_t loop_mac[] = {0, 0, 0, 0, 0, 0};
	loop = netdev_alloc(loop_ip, loop_mac, 1500);
	
    uint32_t netdev_ip[] = {10, 0, 0, 15};
    uint8_t netdev_mac[] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
	netdev = netdev_alloc(netdev_ip, netdev_mac, 1500);

    kprintf("netdev_init():\n");
    kprintf("\tIP  : %u.%u.%u.%u\n", netdev_ip[0], netdev_ip[1], netdev_ip[2],netdev_ip[3]);
    kprintf("\tMAC : ");
    for(int i = 0; i < 5; ++i) {
        if(netdev_mac[i] < 0x10) kprintf("0");
        kprintf("%x:", netdev_mac[i]);
    }
    if(netdev_mac[5] < 0x10) kprintf("0");
    kprintf("%x\n", netdev_mac[5]);

	// 初始化协议栈中用到的锁
	init_socket_lock();
	init_tcp_sock_lock();
	init_udp_sock_lock();
    init_timer_lock();
	
}


uint32_t netdev_transmit(struct sk_buff *skb, uint8_t *dst_hw, uint16_t ethertype) {

	struct netdev *netdev;
	struct eth_hdr *hdr;
	int ret = 0;

	netdev = skb->dev;
	skb_push(skb, ETH_HDR_LEN);
	hdr = (struct eth_hdr *)skb->data;

	/* 拷贝硬件地址 */
	memcpy(hdr->dmac, dst_hw, netdev->addr_len); /* 对端的mac地址 */
	memcpy(hdr->smac, netdev->hwaddr, netdev->addr_len); /* 本端的mac地址 */
	
	hdr->ethertype = htons(ethertype);	/* 帧类型 */
	
	struct device *dev = get_dev_by_major_minor(VIRTIO_MAJOR, 1);
	struct net_device *net_device = dev->get_interface(dev, NET_INTERFACE_BIT);

	net_device->send(dev, skb->data, skb->len);

	return ret;
}


uint32_t netdev_recv(uint8_t *rx_buffer, uint64_t used_len) {

	struct sk_buff *skb = alloc_skb(used_len);
    memcpy(skb->data, rx_buffer, used_len);

    struct eth_hdr *hdr = eth_hdr(skb);  /* 获得以太网头部信息,以太网头部包括
										 目的mac地址,源mac地址,以及类型信息 */

	/* 以太网头部的Type(类型)字段 0x86dd表示IPv6 0x0800表示IPv4
	0x0806表示ARP */
	switch (hdr->ethertype) {
	case ETH_P_ARP:	/* ARP  0x0806 */
		arp_rcv(skb);
		break;
	case ETH_P_IP:  /* IPv4 0x0800 */
		ip_rcv(skb);
		break;
	case ETH_P_IPV6: /* IPv6 0x86dd -- not supported! */
	default:
		kprintf("Unsupported ethertype %x\n", hdr->ethertype);
		free_skb(skb);
		break;
	}
	return 0;
}


void netdev_free() {
    kfree(loop);
    kfree(netdev);
}


struct netdev *netdev_get(uint32_t sip) {
    if (netdev->addr == sip) {
		return netdev; /* 将static local variable的地址传递出去, netdev包含mac地址信息 */
	}
	else
	{
		return NULL;
	}
}

int local_ipaddress(uint32_t addr) {
    /* 传入的addr是本机字节序表示的ip地址 */
	if (!addr) /* INADDR_ANY */
		return 1;
	/* netdev的addr域记录的是本机字节序的ip地址 */
	if (addr == netdev->addr) return 1;
	if (addr == loop->addr) return 1;
	return 0;
}


