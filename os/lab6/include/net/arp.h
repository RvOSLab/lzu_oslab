#ifndef ARP_H
#define ARP_H
#include <net/ethernet.h>
#include <net/netdev.h>
#include <net/list.h>
#include <net/skbuff.h>

#define ARP_ETHERNET	0x0001
#define ARP_IPV4		0x0800
#define ARP_REQUEST		0x0001
#define ARP_REPLY		0x0002

#define ARP_HDR_LEN	sizeof(struct arp_hdr)
#define ARP_DATA_LEN sizeof(struct arp_ipv4)

#define ARP_CACHE_LEN	32
//
// ARP请求的操作字段一共有4种操作类型.ARP请求(1), ARP应答(2)
// RARP请求(3)和RARP应答(4),RARP协议基本上被废弃了.
// 
#define ARP_FREE		0			
#define ARP_WAITING		1
#define ARP_RESOLVED	2



// ARP 头部
struct arp_hdr
{
	uint16_t hwtype;		// 硬件类型
	uint16_t protype;		// 协议类型
	uint8_t hwsize;			// 硬件地址长度
	uint8_t prosize;		// 协议地址长度
	uint16_t opcode;		// 操作类型
	uint8_t data[];
} __attribute__((packed));

// ARP请求和应答分组的数据部分
struct arp_ipv4
{
	uint8_t smac[6];  // 发送端以太网地址
	uint32_t sip;			// 发送端ip地址
	uint8_t dmac[6];  // 目的以太网地址
	uint32_t dip;			// 目的ip地址
} __attribute__((packed));

// arp_cache_entry 用于表示arp缓存
struct arp_cache_entry
{
	struct list_head list;    
	uint16_t hwtype;
	uint32_t sip;
	uint8_t smac[6];
	uint32_t state;
};

uint8_t* arp_get_hwaddr(uint32_t sip);
void arp_init();
void free_arp();
void arp_rcv(struct sk_buff *skb);
void arp_reply(struct sk_buff *skb, struct netdev *netdev);
int arp_request(uint32_t sip, uint32_t dip, struct netdev *netdev);


// arp_hdr用于获取从以太网帧中获取arp头部,以太网头部之后立马就是arp协议的头部
static inline struct arp_hdr *
arp_hdr(struct sk_buff *skb)
{
	return (struct arp_hdr *)(skb->head + ETH_HDR_LEN);
}
#endif // !ARP_H_
