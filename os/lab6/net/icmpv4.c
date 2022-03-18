#include <net/ip.h>
#include <net/icmpv4.h>
#include <kdebug.h>
#include <mm.h>
#include <string.h>


static void icmp_print_reply(uint8_t *buffer) {
    struct iphdr *iphdr = ip_hdr(buffer);					   // 获得ip头部
	struct icmp_v4 *icmp = (struct icmp_v4 *)iphdr->data;  // ip头部后紧跟icmp
    struct icmp_v4_echo *echo = (struct icmp_v4_echo *)icmp->data;

    int32_t sip[4];
    itoip(sip, iphdr->saddr);
    kprintf("%u bytes from %u.%u.%u.%u:", iphdr->len, sip[0], sip[1], sip[2], sip[3]);
    kprintf("icmp_seq=%u ttl=%u\n", ntohs(echo->seq), iphdr->ttl);
}

void icmpv4_incoming(uint8_t *buffer) {
	struct iphdr *iphdr = ip_hdr(buffer);					   // 获得ip头部
	struct icmp_v4 *icmp = (struct icmp_v4 *)iphdr->data;  // ip头部后紧跟icmp

	// todo: Check csum

	switch (icmp->type)
	{
	case ICMP_V4_ECHO:		// 0x08 ping request
		icmpv4_reply(buffer);
		return;
    case ICMP_V4_REPLY:
        icmp_print_reply(buffer);
        goto drop_pkt;
	case ICMP_V4_DST_UNREACHABLE:
        kprintf("ICMPv4 received 'dst unreachable' code %u "
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

void icmpv4_echo_request(uint32_t daddr, uint32_t seq, char* txt) {
    uint32_t icmp_len = sizeof(struct icmp_v4) +  sizeof(struct icmp_v4_echo) + strlen(txt);
    uint8_t *buffer = kmalloc(ETH_HDR_LEN + IP_HDR_LEN + icmp_len);

    struct icmp_v4 *icmp = (struct icmp_v4 *)(buffer + ETH_HDR_LEN + IP_HDR_LEN);
    struct icmp_v4_echo *echo = (struct icmp_v4_echo *)(icmp->data);

    memcpy(echo->data, txt, strlen(txt));
    icmp->type = ICMP_V4_ECHO;
    icmp->code = 0;

    echo->id = 1;
    echo->seq = seq;

    echo->id = htons(echo->id);
    echo->seq = htons(echo->seq);

    icmp->csum = 0;
    icmp->csum = checksum(icmp, icmp_len, 0);

    extern struct netdev* netdev;
    ip_output(buffer, daddr, ICMPV4, netdev, icmp_len + IP_HDR_LEN);
    kfree(buffer);

}
