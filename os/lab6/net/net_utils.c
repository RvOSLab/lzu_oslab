#include <net/net_utils.h>
#include <kdebug.h>

int32_t iptoi(uint32_t *ip) {
    return (ip[0] << 24) + (ip[1] << 16) + (ip[2] << 8) + ip[3];
}

void itoip(uint32_t *ip, uint32_t i) {
	ip[0] = i >> 24;
	ip[1] = i >> 16 & 0xff0000;
	ip[2] = i >> 8 & 0xff00;
	ip[3] = i & 0xff;
}

void printbuf(uint8_t *buffer, uint32_t length) {
	kprintf("\t");
	for (uint64_t i = 0; i < length; i += 1) {
		if(buffer[i] < 0x10) kprintf("0");
		kprintf("%x ", buffer[i]);
		if(i%8 == 7) kprintf(" ");
		if(i%16 == 15) kprintf("\n\t");
	}
	kprintf("\n\n");
}


uint32_t
sum_every_16bits(void *addr, int count)
{
	register uint32_t sum = 0;
	uint16_t *ptr = addr;
	uint16_t answer = 0;

	while (count > 1) {
		/*  This is the inner loop */
		sum += *ptr++;
		count -= 2;
	}

	if (count == 1) {
		/*
		这里有一个细节需要注意一下. unsigned char 8bit
		answer 16bit			将answer强制转换为8bit,会使得最后剩下的8bit被放入到x中
		+-----+-----+			+-----+-----+
		|  8  |  8  |			|xxxxx|     |
		+-----+-----+			+-----+-----+
		*/
		*(uint8_t *)(&answer) = *(uint8_t *)ptr;
		sum += answer;
	}

	return sum;
}

/* checksum 用于计算校验值 */
uint16_t
checksum(void *addr, int count, int start_sum)
{
	uint32_t sum = start_sum;
	sum += sum_every_16bits(addr, count);

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);
	return ~sum;
}

int
tcp_udp_checksum(uint32_t saddr, uint32_t daddr, uint8_t proto,
	uint8_t *data, uint16_t len)
{
	/* we need to ensure that saddr and daadr are in netowrk byte order */
	uint32_t sum = 0;
	struct pseudo_head head;
	memset(&head, 0, sizeof(struct pseudo_head));
	/* 需要保证传入的daddr以及saddr是网络字节序 */
	head.daddr = daddr;
	head.saddr = saddr;
	/* 对于TCP来说,proto = 0x06,而对于UDP来说proto = 0x17 */
	head.proto = proto; /* sizeof(proto) == 1,
						对于只有1个字节的数据,不需要转换字节序 */
	head.len = htons(len);
	sum = sum_every_16bits(&head, sizeof(struct pseudo_head));
	return checksum(data, len, sum);
}
