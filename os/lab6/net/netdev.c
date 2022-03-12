#include <net/netdev.h>

int netdev_send(void *buffer, uint64_t length) {
    int ret = 0;
    
    struct device *dev = get_dev_by_major_minor(VIRTIO_MAJOR, 1);
    virtio_net_send(dev, buffer, length);

    return ret;
}


int netdev_recv(uint8_t *buffer, uint64_t used_len) {
    struct eth_hdr *hdr = eth_hdr(buffer);  /* 获得以太网头部信息,以太网头部包括
										 目的mac地址,源mac地址,以及类型信息 */

	/* 以太网头部的Type(类型)字段 0x86dd表示IPv6 0x0800表示IPv4
	0x0806表示ARP */
	switch (hdr->ethertype) {
	case ETH_P_ARP:	/* ARP  0x0806 */
        kprintf("recv a arp packet!\n");
        kprintf("virtio-net: recv %u bits\n    ", used_len);
        for (uint64_t i = 0; i < used_len; i += 1) {
            if(buffer[i] < 0x10) kprintf("0");
            kprintf("%x ", buffer[i]);
            if(i%8 == 7) kprintf(" ");
            if(i%16 == 15) kprintf("\n    ");
        }
        kprintf("\n");
		// todo
		break;
	case ETH_P_IP:  /* IPv4 0x0800 */
		// todo
		break;
	case ETH_P_IPV6: /* IPv6 0x86dd -- not supported! */
	default:
		kprintf("Unsupported ethertype %x\n", hdr->ethertype);
		// free_s_i(buffer, sizeof(buffer));  
		break;
	}
	return 0;
}
