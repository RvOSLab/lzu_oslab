#ifndef NETDEV_H
#define NETDEV_H

#include <stddef.h>

struct sk_buff;

struct netdev {
	uint32_t addr;			/* ip地址,主机字节序 */
	uint8_t addr_len;		
	uint8_t hwaddr[6];		/* mac地址,6个字节 */
	uint32_t mtu;			/* mtu,最大传输单元,一般默认为1500字节 */
};


void netdev_init();
void netdev_free();
uint32_t netdev_transmit(struct sk_buff *skb, uint8_t *dst, uint16_t ethertype);
uint32_t netdev_recv(uint8_t *rx_buffer, uint64_t used_len);
struct netdev *netdev_get(uint32_t sip);
int local_ipaddress(uint32_t addr);



uint64_t net_test();

#endif // !NETDEV_H
