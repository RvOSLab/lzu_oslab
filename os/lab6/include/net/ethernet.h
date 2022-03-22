#ifndef ETHERNET_H_
#define ETHERNET_H_
#include <stddef.h>
#include <net/netdev.h>
#include <net/net_utils.h>
#include <net/skbuff.h>

#define ETH_HDR_LEN sizeof(struct eth_hdr)


// eth_hdr 以太网头部
struct eth_hdr
{
	uint8_t dmac[6];	// 目的mac地址
	uint8_t smac[6];	// 源mac地址
	uint16_t ethertype; // 帧的类型,可选有0x0800(IPv4), 0x86dd(IPv6)
	uint8_t payload[];
} __attribute__((packed));

static inline struct eth_hdr *
eth_hdr(struct sk_buff *skb)
{
	struct eth_hdr *hdr = (struct eth_hdr *)skb_head(skb);
	hdr->ethertype = ntohs(hdr->ethertype);			// 将网络字节序转换为本地字节序
	return hdr;
}

#endif
