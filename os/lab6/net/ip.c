#include <net/ip.h>
#include <net/arp.h>
#include <net/net_utils.h>
#include <net/netdef.h>
#include <mm.h>
#include <kdebug.h>

struct iphdr *ip_hdr(uint8_t *buffer) {
	// 以太网帧中以太网头部之后跟的就是ip头部
	return (struct iphdr *)(buffer + ETH_HDR_LEN);
}

// ip_init_pkt对于接收到的ip数据报进行一定程度的解码,也就是将网络字节序转换为主机字节序
// 方便后面的操作.
static void
ip_init_pkt(struct iphdr *ih) {
	ih->saddr = ntohl(ih->saddr);	/* 源ip地址 */
	ih->daddr = ntohl(ih->daddr);	/* 目的ip地址 */
	ih->len = ntohs(ih->len);		/* 16位总长度 */
	ih->id = ntohs(ih->id);			/* 唯一的标识 */
}

/**\
 * ip_pkt_for_us 判断数据包是否是传递给我们的.
\**/
static int
ip_pkt_for_us(struct iphdr *ih) {
	extern struct netdev* netdev;
	return ih->daddr == netdev->addr ? 1 : 0;
}
/**\
 * ip_rcv 处理接收到的ip数据报
\**/
int ip_rcv(uint8_t *buffer) {
	struct iphdr *ih = ip_hdr(buffer);
	uint16_t csum = -1;


	if (ih->version != IPV4) { // 0x0800 IPv4
		kprintf("Datagram version was not IPv4\n");
		goto drop_pkt;
	}

	if (ih->ihl < 5) {
		// 5 * 32bit = 20字节
        kprintf("IPv4 header length must be at least 5\n");
		goto drop_pkt;
	}

	if (ih->ttl == 0) {
		kprintf("Time to live of datagram reached 0\n");
		goto drop_pkt;
	}

	csum = checksum(ih, ih->ihl * 4, 0);

	if (csum != 0) {
		/* 无效的数据报 */
		goto drop_pkt;
	}

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  todo: ip数据报重组
	  ip协议并不是一个可靠的协议,它没有tcp协议的重传和确认机制.
	  当上层应用数据报过大,超过了MTU,那么在ip层就要进行拆包,将
	  大数据拆分成小数据发送出去,对方接收到之后也要进行组包.
	 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	ip_init_pkt(ih);

	if (!ip_pkt_for_us(ih)) goto drop_pkt;
    switch (ih->proto) {
    case ICMPV4:
		kprintf("recv a ICMPV4 packet.");
        // todo
        return 0;
    case IP_TCP:
		kprintf("recv a TCP packet.");
        // todo
        return 0;
	case IP_UDP:
		kprintf("recv a UDP packet.");
		// todo;
		return 0;
    default:
        kprintf("Unknown IP header proto\n");
        goto drop_pkt;
    }

drop_pkt:
	kfree(buffer);
	return 0;
}

int ip_output(uint8_t *buffer, uint32_t daddr, uint8_t proto, 
	struct netdev *netdev, uint32_t len) {
	
	struct iphdr *ihdr = ip_hdr(buffer);

	ihdr->version = IPV4;			/* ip的版本是IPv4 */
	ihdr->ihl = 0x05;				/* ip头部20字节,也就是说不附带任何选项 */
	ihdr->tos = 0;					/* tos选项不被大多数TCP/IP实现所支持  */
	ihdr->len = len;				/* 整个ip数据报的大小 */
	ihdr->id = ihdr->id;			/* id不变 */
	ihdr->flags = 0;
	ihdr->frag_offset = 0;
	ihdr->ttl = 64;
	ihdr->proto = proto;
	ihdr->saddr = netdev->addr;
	ihdr->daddr = daddr;
	ihdr->csum = 0;

	ihdr->len = htons(ihdr->len);
	ihdr->id = htons(ihdr->id);
	ihdr->daddr = htonl(ihdr->daddr);
	ihdr->saddr = htonl(ihdr->saddr);
	ihdr->csum = htons(ihdr->csum);
	ihdr->csum = checksum(ihdr, ihdr->ihl * 4, 0);

	int count = 0;
	uint8_t *dmac;

	// 发送到网关
	uint32_t nip[] = {10, 0, 0, 2};
	uint32_t ndaddr = iptoi(nip);

	uint32_t saddr = ntohl(ihdr->saddr);

	// static uint8_t broadcast_hw[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; /* 广播地址 */

try_agin:
	dmac = arp_get_hwaddr(ndaddr); /* 根据ip地址获得mac地址 */
	// dmac = broadcast_hw;

	if (dmac) {
		return netdev_transmit(buffer, dmac, ETH_P_IP, len + ETH_HDR_LEN, netdev);
	}
	else {
		count += 1;
		arp_request(saddr, ndaddr, netdev);
        /* Inform upper layer that traffic was not sent, retry later */
		if (count < 3) {
			// sleep
			for(int i = 0; i < 1000000; ++i);
			goto try_agin;
		}
		return -1;
	}
}





