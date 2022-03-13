#ifndef NETDEV_H
#define NETDEV_H

#include <stddef.h>
#include <device/virtio/virtio_net.h>
#include <net/netdef.h>
#include <net/ethernet.h>
#include <kdebug.h>
#include <mm.h>
#include <string.h>

struct netdev {
	uint32_t addr;			/* ip地址,主机字节序 */
	uint8_t addr_len;		
	uint8_t hwaddr[6];		/* mac地址,6个字节 */
	uint32_t mtu;			/* mtu,最大传输单元,一般默认为1500字节 */
};


void netdev_init();
void netdev_free();
int netdev_send(void *buffer, uint64_t length);
int netdev_recv(uint8_t *rx_buffer, uint64_t used_len);
struct netdev *netdev_get(uint32_t sip);
int local_ipaddress(uint32_t addr);

uint64_t net_test();


#endif // !NETDEV_H
