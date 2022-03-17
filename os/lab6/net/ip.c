#include <net/ip.h>

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
uint32_t
ip_rcv(uint8_t *buffer)
{
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



struct iphdr *ip_hdr(uint8_t *buffer)
{
	// 以太网帧中以太网头部之后跟的就是ip头部
	return (struct iphdr *)(buffer + ETH_HDR_LEN);
}

