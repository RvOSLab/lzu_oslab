#include <net/net_utils.h>
#include <kdebug.h>

int32_t iptoi(uint32_t *ip) {
    return (ip[0] << 24) + (ip[1] << 16) + (ip[2] << 8) + ip[3];
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
