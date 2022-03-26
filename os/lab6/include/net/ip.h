#ifndef IP_H
#define IP_H

#include <net/netdev.h>
#include <net/ethernet.h>
#include <net/skbuff.h>
#include <net/sock.h>

#define IPV4	 0x04
#define IP_TCP	 0x06
#define IP_UDP	 0x11
#define ICMPV4	 0x01

#define IP_HDR_LEN sizeof(struct iphdr)
// ip_len表示ip数据报的大小,不包含首部
#define ip_len(ip) (ip->len - (ip->ihl * 4))

// iphdr 表示ip头部
// ihl -- 首部长度指的是首部占32bit字的数目包括任何选项,由于它是一个4比特字段,因此,首部长度最长为
//        15*4=60字节.
// tos -- 该字段不被大多数tcp/ip实现所支持.
// len -- 总长度字段指的是整个IP数据报的长度,以字节为单位,利用首部长度字段和总长度字段,就可以知
//        道IP数据报中数据内容的 起始位置和长度,由于该字段长16bit,所以IP数据报最长可达65535字节.
//  id -- 标识字段,唯一的标识主机发送的每一份数据报.
// ttl -- 生存时间,设置数据报最多可以经过的路由器数.
struct iphdr {
	uint8_t ihl : 4;					// 4位首部长度
	uint8_t version : 4;				// 4位版本号
	uint8_t tos;						// 8位服务类型
	uint16_t len;						// 16位总长度
	uint16_t id;						// 16位标识
	uint16_t flags : 3;					// 3位标志
	uint16_t frag_offset : 13;			// 13位偏移
	uint8_t ttl;						// 8位生存时间
	uint8_t proto;						// 8位协议
	uint16_t csum;						// 16位首部校验和
	uint32_t saddr;						// 源地址
	uint32_t daddr;						// 目的地址
	uint8_t data[];
} __attribute__((packed));

static inline struct iphdr *
ip_hdr(const struct sk_buff *skb)
{
	// 以太网帧中以太网头部之后跟的就是ip头部
	return (struct iphdr *)(skb->head + ETH_HDR_LEN);
}


int ip_rcv(struct sk_buff *skb);
int ip_output(struct sock *sk, struct sk_buff *skb);
int dst_neigh_output();

#endif // IP_H
