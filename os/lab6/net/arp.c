#include <net/arp.h>
#include <net/net_utils.h>
#include <net/netdef.h>
#include <net/ethernet.h>
#include <mm.h>
#include <kdebug.h>
#include <string.h>

static uint8_t broadcast_hw[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; /* 广播地址 */

static LIST_HEAD(arp_cache);

static struct arp_cache_entry *
arp_entry_alloc(struct arp_hdr *hdr, struct arp_ipv4 *data)
{
	struct arp_cache_entry *entry = kmalloc(sizeof(struct arp_cache_entry));
	list_init(&entry->list);
	entry->state = ARP_RESOLVED;	// arp应答
	entry->hwtype = hdr->hwtype;
	entry->sip = data->sip;
	memcpy(entry->smac, data->smac, sizeof(entry->smac));
	return entry;
}

static int  
insert_arp_translation_table(struct arp_hdr *hdr, struct arp_ipv4 *data) {
	struct arp_cache_entry *entry = arp_entry_alloc(hdr, data);
	list_add_tail(&entry->list, &arp_cache); /* 添加到arp_cache的尾部 */
	return 0;
}

static int 
update_arp_translation_table(struct arp_hdr *hdr, struct arp_ipv4 *data) {
	struct list_head *item;
	struct arp_cache_entry *entry;
	
	list_for_each(item, &arp_cache) {
		entry = list_entry(item, struct arp_cache_entry, list);
		if (entry->hwtype == hdr->hwtype && entry->sip == data->sip) {
			memcpy(entry->smac, data->smac, 6);
			return 1;
		}
	}
	return 0;
}

void arp_init() {
	
}

void arp_rcv(uint8_t *buffer) {
    struct arp_hdr *arphdr;
    struct arp_ipv4 *arpdata;
    struct netdev *netdev;
    int32_t merge = 0;
    arphdr = arp_hdr(buffer);
    arphdr->hwtype = ntohs(arphdr->hwtype);
    arphdr->protype = ntohs(arphdr->protype);
    arphdr->opcode = ntohs(arphdr->opcode);

   if (arphdr->hwtype != ARP_ETHERNET) {		// 1代表以太网地址
		kprintf("ARP: Unsupported HW type\n");
		goto drop_pkt;
	}

	if (arphdr->protype != ARP_IPV4) {			// 0x0800 -- 2048表示arp协议
		kprintf("ARP: Unsupported protocol\n");
		goto drop_pkt;
	}

    arpdata = (struct arp_ipv4 *)arphdr->data;

	arpdata->sip = ntohl(arpdata->sip);		// 发送方ip地址
	arpdata->dip = ntohl(arpdata->dip);		// 接收方ip地址

    merge = update_arp_translation_table(arphdr, arpdata); // 更新arp缓存

	if (!(netdev = netdev_get(arpdata->dip))) {
		kprintf("ARP was not for us\n");
		goto drop_pkt;
	}

	if (!merge && insert_arp_translation_table(arphdr, arpdata) != 0) {
		kprintf("ERR: No free space in ARP translation table\n");
		goto drop_pkt;
	}

    switch (arphdr->opcode) {
	case ARP_REQUEST:		// 0x0001 -- arp请求
		kprintf("recv a arp request!");
		kprintf("from %u.%u.%u.%u\n", arpdata->sip >> 24, arpdata->sip >> 16 & 0xff, 
			arpdata->sip >> 8 & 0xff, arpdata->sip & 0xff);
		arp_reply(buffer, netdev);
		return;
	case ARP_REPLY:			// 0x0002 -- arp回复,这里实际上在上面已经处理了,更新了arp缓存
		kprintf("recv a arp reply!");
		kprintf("from %u.%u.%u.%u\n", arpdata->sip >> 24, arpdata->sip >> 16 & 0xff, 
			arpdata->sip >> 8 & 0xff, arpdata->sip & 0xff);
		return;
	default:
		kprintf("ARP: Opcode not supported\n");
		goto drop_pkt;
	}

drop_pkt:
	kfree(buffer);
	return;
}

int arp_request(uint32_t sip, uint32_t dip, struct netdev *netdev) {
	uint8_t *buffer;
	struct arp_hdr *arp;
	struct arp_ipv4 *payload;
	int rc = 0;
	int size = ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN;

	buffer = kmalloc(size); // 分配arp回复的数据

	payload = (struct arp_ipv4 *)(buffer + (size - ARP_DATA_LEN));
	
	/* 发送端填入的是本机的地址信息,但是ip是主机字节序 */
	memcpy(payload->smac, netdev->hwaddr, netdev->addr_len); // 拷贝硬件地址
	payload->sip = sip;

	/* 接收端填入的是广播地址以及想要查询的ip的地址,但是ip是主机字节序 */
	memcpy(payload->dmac, broadcast_hw, netdev->addr_len);  // 广播
	payload->dip = dip;


	arp = arp_hdr(buffer);

	arp->opcode = htons(ARP_REQUEST);
	arp->hwtype = htons(ARP_ETHERNET);
	arp->protype = htons(ETH_P_IP);
	arp->hwsize = netdev->addr_len;
	arp->prosize = 4;

	payload->sip = htonl(payload->sip);
	payload->dip = htonl(payload->dip);

	rc = netdev_transmit(buffer, broadcast_hw, ETH_P_ARP, size, netdev);

	kfree(buffer);
	return rc;
}

void arp_reply(uint8_t *buffer, struct netdev* netdev){
	/* netdev中包含了本机的地址信息,包括ip地址和mac地址 */
	struct arp_hdr *arphdr;
	struct arp_ipv4 *arpdata;

	int size = ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN;

	arphdr = arp_hdr(buffer);

	arpdata = (struct arp_ipv4 *)arphdr->data;

	/* 将源地址和目的地址进行交换,需要注意的是下面的ip地址都是主机字节序 */
	memcpy(arpdata->dmac, arpdata->smac, 6);
	arpdata->dip = arpdata->sip;

	memcpy(arpdata->smac, netdev->hwaddr, 6);
	arpdata->sip = netdev->addr;

	arphdr->opcode = ARP_REPLY; // 0x0002


	arphdr->opcode = htons(arphdr->opcode);
	arphdr->hwtype = htons(arphdr->hwtype);
	arphdr->protype = htons(arphdr->protype);

	/* 最终转换为本机字节序 */
	arpdata->sip = htonl(arpdata->sip);
	arpdata->dip = htonl(arpdata->dip);

	// arp协议完成自己的部分,然后接下来让链路层去操心
	netdev_transmit(buffer, arpdata->dmac, ETH_P_ARP, size, netdev);
	kfree(buffer);
}

// arp_get_hwaddr 根据ip得到mac地址,找不到则返回NULL
uint8_t* arp_get_hwaddr(uint32_t sip) {
	struct list_head *item;
	struct arp_cache_entry *entry;
	
	list_for_each(item, &arp_cache) {
		entry = list_entry(item, struct arp_cache_entry, list);

		if (entry->state == ARP_RESOLVED &&
			entry->sip == sip) {
			return entry->smac;
		}
	}
	return NULL;
}

void free_arp(){
	struct list_head *item, *tmp;
	struct arp_cache_entry *entry;
	
	list_for_each_safe(item, tmp, &arp_cache) {
		entry = list_entry(item, struct arp_cache_entry, list);
		list_del(item);
		kfree(entry);
	}
}

struct arp_hdr * arp_hdr(uint8_t *buffer)
{
	return (struct arp_hdr *)(buffer + ETH_HDR_LEN);
}
