#include <net/net_utils.h>

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
