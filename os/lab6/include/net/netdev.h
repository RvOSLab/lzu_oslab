#ifndef NETDEV_H
#define NETDEV_H

#include <stddef.h>
#include <device/virtio/virtio_net.h>
#include <net/netdef.h>
#include <net/ethernet.h>
#include <kdebug.h>
#include <mm.h>
#include <string.h>


int netdev_send(void *buffer, uint64_t length);
int netdev_recv(uint8_t *rx_buffer, uint64_t used_len);

uint64_t net_test();

#endif // !NETDEV_H
