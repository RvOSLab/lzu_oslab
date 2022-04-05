#include <net/ip.h>
#include <net/icmpv4.h>
#include <kdebug.h>
#include <mm.h>
#include <string.h>
#include <net/sock.h>

static struct sk_buff *
icmpv4_alloc_skb(uint64_t icmp_len)
{
	struct sk_buff *skb = alloc_skb(icmp_len + ETH_HDR_LEN + IP_HDR_LEN);
	skb_reserve(skb, icmp_len + ETH_HDR_LEN + IP_HDR_LEN);
	skb->protocol = htons(ICMPV4);
	return skb;
}


static void icmp_print_reply(struct sk_buff *skb) {
    struct iphdr *iphdr = ip_hdr(skb);					  
	struct icmp_v4 *icmp = (struct icmp_v4 *)iphdr->data;  
    struct icmp_v4_echo *echo = (struct icmp_v4_echo *)icmp->data;

    int32_t sip[4];
    itoip(sip, iphdr->saddr);
    kprintf("%u bytes from %u.%u.%u.%u:", iphdr->len, sip[0], sip[1], sip[2], sip[3]);
    kprintf("icmp_seq=%u ttl=%u\n", ntohs(echo->seq), iphdr->ttl);
}

void icmpv4_incoming(struct sk_buff *skb) {
	struct iphdr *iphdr = ip_hdr(skb);					   // 获得ip头部
	struct icmp_v4 *icmp = (struct icmp_v4 *)iphdr->data;  // ip头部后紧跟icmp
	// todo: Check csum

	switch (icmp->type)
	{
	case ICMP_V4_ECHO:		// 0x08 ping request
		icmpv4_reply(skb);
		return;
    case ICMP_V4_REPLY:
        icmp_print_reply(skb);
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
	free_skb(skb);
	return;
}

void icmpv4_reply(struct sk_buff *skb) {
	struct iphdr *iphdr = ip_hdr(skb);		// 获得ip头部
	struct icmp_v4 *icmp = (struct icmp_v4 *)iphdr->data;

	struct sock sk;
	memset(&sk, 0, sizeof(struct sock));


	// iphdr->ihl * 4指的是ip头部的大小
	uint16_t icmp_len = iphdr->len - (iphdr->ihl * 4);		// ip数据报的总长度减去ip头部大小,得到icmp数据报的大小
    skb_reserve(skb, ETH_HDR_LEN + IP_HDR_LEN + icmp_len);
	skb_push(skb, icmp_len); // icmp回复的大小
	
	icmp->type = ICMP_V4_REPLY;				   // ICMP回显应答 
	icmp->csum = 0;
	icmp->csum = checksum(icmp, icmp_len, 0);  /* 计算校验和 */

	skb->protocol = ICMPV4;
	sk.daddr = iphdr->saddr;

	ip_output(&sk, skb);
	free_skb(skb);
}

void icmpv4_echo_request(uint32_t daddr, uint32_t seq, char* txt) {
    uint32_t icmp_len = sizeof(struct icmp_v4) +  sizeof(struct icmp_v4_echo) + strlen(txt);
    struct sk_buff *skb = icmpv4_alloc_skb(icmp_len);
    // 填入ICMP数据
    struct icmp_v4 *icmp = (struct icmp_v4 *)skb_push(skb, icmp_len);
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

	struct sock sk;
	memset(&sk, 0, sizeof(struct sock));
	sk.daddr = daddr;

    skb->protocol = ICMPV4;
    ip_output(&sk, skb);
    free_skb(skb);

}

void icmpv4_port_unreachable(uint32_t daddr, struct sk_buff *recv_skb) {
	struct iphdr *iphdr = ip_hdr(recv_skb);
	uint32_t icmp_len = sizeof(struct icmp_v4) + sizeof(struct icmp_v4_dst_unreachable) + iphdr->ihl + 8;
	struct sk_buff *skb = icmpv4_alloc_skb(icmp_len);
	struct icmp_v4 *icmp = (struct icmp_v4 *)skb_push(skb, icmp_len);
	struct icmp_v4_dst_unreachable *icmp_unreachable = (struct icmp_v4_dst_unreachable*)(icmp->data);

	memcpy(icmp_unreachable->data, iphdr, iphdr->ihl * 4 + 8 * 4);
	icmp->type = ICMP_V4_DST_UNREACHABLE;
	icmp->code = ICMP_PORT_UNREACH;

	icmp->csum = 0;
    icmp->csum = checksum(icmp, icmp_len, 0);
	skb->protocol = ICMPV4;

	struct sock sk;
	memset(&sk, 0, sizeof(struct sock));
	sk.daddr = daddr;

    ip_output(&sk, skb);
    free_skb(skb);
}
