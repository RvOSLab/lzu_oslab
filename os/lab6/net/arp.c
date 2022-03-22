#include <net/arp.h>
#include <net/net_utils.h>
#include <net/netdef.h>
#include <net/ethernet.h>
#include <mm.h>
#include <kdebug.h>
#include <string.h>

static uint8_t broadcast_hw[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; /* 广播地址 */

static LIST_HEAD(arp_cache);

static struct sk_buff *
arp_alloc_skb()
{
	struct sk_buff *skb = alloc_skb(ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN);
	skb_reserve(skb, ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN);
	skb->protocol = htons(ETH_P_ARP);
	return skb;
}

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

void arp_rcv(struct sk_buff *skb) {
	struct arp_hdr *arphdr;
	struct arp_ipv4 *arpdata;
	struct netdev *netdev;
	int merge = 0;
	arphdr = arp_hdr(skb);
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
		arp_reply(skb, netdev);
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
	free_skb(skb);
	return;
}

int arp_request(uint32_t sip, uint32_t dip, struct netdev *netdev) {
	struct sk_buff *skb;
	struct arp_hdr *arp;
	struct arp_ipv4 *payload;
	int rc = 0;

	skb = arp_alloc_skb(); // 分配arp回复的数据
	
	if (!skb) return -1;

	skb->dev = netdev;
	payload = (struct arp_ipv4 *)skb_push(skb, ARP_DATA_LEN);
	
	/* 发送端填入的是本机的地址信息,但是ip是主机字节序 */
	memcpy(payload->smac, netdev->hwaddr, netdev->addr_len); // 拷贝硬件地址
	payload->sip = sip;

	/* 接收端填入的是广播地址以及想要查询的ip的地址,但是ip是主机字节序 */
	memcpy(payload->dmac, broadcast_hw, netdev->addr_len);  // 广播
	payload->dip = dip;


	arp = (struct arp_hdr *)skb_push(skb, ARP_HDR_LEN);

	arp->opcode = htons(ARP_REQUEST);
	arp->hwtype = htons(ARP_ETHERNET);
	arp->protype = htons(ETH_P_IP);
	arp->hwsize = netdev->addr_len;
	arp->prosize = 4;

	payload->sip = htonl(payload->sip);
	payload->dip = htonl(payload->dip);

	rc = netdev_transmit(skb, broadcast_hw, ETH_P_ARP);

	free_skb(skb);
	return rc;
}

void arp_reply(struct sk_buff *skb, struct netdev *netdev)
{
	/* netdev中包含了本机的地址信息,包括ip地址和mac地址 */
	struct arp_hdr *arphdr;
	struct arp_ipv4 *arpdata;

	arphdr = arp_hdr(skb);

	// skb_reserve函数会将skb的data指针向后移动ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN个字节.
	skb_reserve(skb, ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN);	
	// skb_push函数将skb的data指针向前移动ARP_HDR_LEN个字节
	skb_push(skb, ARP_HDR_LEN + ARP_DATA_LEN);

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

	skb->dev = netdev;
	// arp协议完成自己的部分,然后接下来让链路层去操心
	netdev_transmit(skb, arpdata->dmac, ETH_P_ARP);
	free_skb(skb);
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

