#include <net/ip.h>
#include <net/icmpv4.h>
#include <kdebug.h>
#include <mm.h>

void icmpv4_incoming(uint8_t *buffer) {
	struct iphdr *iphdr = ip_hdr(buffer);					   // 获得ip头部
	struct icmp_v4 *icmp = (struct icmp_v4 *)iphdr->data;  // ip头部后紧跟icmp

	// todo: Check csum

	switch (icmp->type)
	{
	case ICMP_V4_ECHO:		// 0x08 ping request
		icmpv4_reply(buffer);
		return;
	case ICMP_V4_DST_UNREACHABLE:
        kprintf("ICMPv4 received 'dst unreachable' code %d, "
                  "check your routes and firewall rules\n", icmp->code);
        goto drop_pkt;
	default:
		kprintf("ICMPv4 did not match supported types\n");
		goto drop_pkt;
	}
drop_pkt:
	kfree(buffer);
	return;
}

void icmpv4_reply(uint8_t *buffer) {
	struct iphdr *iphdr = ip_hdr(buffer);		// 获得ip头部
	struct icmp_v4 *icmp = (struct icmp_v4 *)iphdr->data;


	// iphdr->ihl * 4指的是ip头部的大小
	uint16_t icmp_len = iphdr->len - (iphdr->ihl * 4);		// ip数据报的总长度减去ip头部大小,得到icmp数据报的大小
	
	icmp->type = ICMP_V4_REPLY;				   // ICMP回显应答 
	icmp->csum = 0;
	icmp->csum = checksum(icmp, icmp_len, 0);  /* 计算校验和 */

	
    extern struct netdev* netdev;
	ip_output(buffer, iphdr->saddr, ICMPV4, netdev, iphdr->len);
	kfree(buffer);
}
