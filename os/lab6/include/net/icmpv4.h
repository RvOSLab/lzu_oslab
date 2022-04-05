#ifndef ICMPV4_H
#define ICMPV4_H

#include <stddef.h>

#define ICMP_V4_REPLY				0x00  /* 回显应答 */
#define ICMP_V4_DST_UNREACHABLE		0x03
#define ICMP_V4_SRC_QUENCH			0x04
#define ICMP_V4_REDIRECT			0x05
#define ICMP_V4_ECHO				0x08
#define ICMP_V4_ROUTER_ADV			0x09
#define ICMP_V4_ROUTER_SOL			0x0a
#define ICMP_V4_TIMEOUT				0x0b
#define ICMP_V4_MALFORMED			0x0c
#define ICMP_V4_TSTAMP				0x0d	/* 时间戳请求 */
#define ICMP_V4_TSTAMP_REPLY		0x0e	/* 时间戳应答 */

/* Codes for UNREACH. */
#define ICMP_NET_UNREACH	0	/* Network Unreachable		*/
#define ICMP_HOST_UNREACH	1	/* Host Unreachable		*/
#define ICMP_PROT_UNREACH	2	/* Protocol Unreachable		*/
#define ICMP_PORT_UNREACH	3	/* Port Unreachable		*/
#define ICMP_FRAG_NEEDED	4	/* Fragmentation Needed/DF set	*/
#define ICMP_SR_FAILED		5	/* Source Route failed		*/
#define ICMP_NET_UNKNOWN	6
#define ICMP_HOST_UNKNOWN	7
#define ICMP_HOST_ISOLATED	8
#define ICMP_NET_ANO		9
#define ICMP_HOST_ANO		10
#define ICMP_NET_UNR_TOS	11
#define ICMP_HOST_UNR_TOS	12
#define ICMP_PKT_FILTERED	13	/* Packet filtered */
#define ICMP_PREC_VIOLATION	14	/* Precedence violation */
#define ICMP_PREC_CUTOFF	15	/* Precedence cut off */
#define NR_ICMP_UNREACH		15	/* instead of hardcoding immediate value */

// icmp报文通用格式
struct icmp_v4 {
	uint8_t type;		// 8位类型
	uint8_t code;		// 8位代码
	uint16_t csum;		// 16位校验和
	uint8_t data[];
} __attribute__((packed));

struct icmp_v4_echo {
	uint16_t id;
	uint16_t seq;
	uint8_t data[];
} __attribute__((packed));

struct icmp_v4_timestamp {
	uint8_t type;
	uint8_t code;
	uint16_t csum;
	uint32_t otime;		/* 发起时间 */
	uint32_t rtime;		/* 接收时间 */
	uint32_t ttime;		/* 传送时间 */
} __attribute__((packed));

struct icmp_v4_dst_unreachable {
	uint32_t unused;
	uint8_t data[];
} __attribute__((packed));

void icmpv4_incoming(struct sk_buff *skb);
void icmpv4_reply(struct sk_buff *skb);
void icmpv4_echo_request(uint32_t daddr, uint32_t seq, char* txt); 		// 回显请求
void icmpv4_port_unreachable(uint32_t daddr, struct sk_buff *recv_skb); // 端口不可达
// void icmpv4_timestamp(uint8_t *buffer);
#endif // !ICMPV4_H
