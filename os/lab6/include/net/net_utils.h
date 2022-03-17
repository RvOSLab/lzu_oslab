#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stddef.h>
#include <kdebug.h>

inline static uint32_t htonl(uint32_t hostlong) {		//把uint32_t类型从主机序转换到网络序
	return (hostlong << 24) + (hostlong << 8 & 0xff0000) + (hostlong >> 8 & 0xff00) + (hostlong >> 24);
}
inline static uint16_t htons(uint16_t hostshort) {		//把uint16_t类型从主机序转换到网络序
	return (hostshort << 8) + (hostshort >> 8);
}
inline static uint32_t ntohl(uint32_t netlong) {		//把uint32_t类型从网络序转换到主机序
	return (netlong << 24) + (netlong << 8 & 0xff0000) + (netlong >> 8 & 0xff00) + (netlong >> 24);
}
inline static uint16_t ntohs(uint16_t netshort) {		//把uint16_t类型从网络序转换到主机序
	return (netshort << 8) + (netshort >> 8);
}

void printbuf(uint8_t *buffer, uint32_t length);

uint32_t sum_every_16bits(void *addr, int count);
uint16_t checksum(void *addr, int count, int start_sum);


#endif
