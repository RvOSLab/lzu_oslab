#include <device.h>
#include <device/reset/sifive_test.h>
#include <device/serial/uart8250.h>
#include <device/virtio/virtio_blk.h>
#include <device/virtio/virtio_net.h>
#include <kdebug.h>

uint64_t char_dev_test(uint64_t c) {
    struct device *dev = get_dev_by_major_minor(UART8250_MAJOR, 1);
    struct serial_device *char_test = dev->get_interface(dev, SERIAL_INTERFACE_BIT);
    char_test->request(dev, &c, 1, !c);
    return c;
}

uint64_t net_dev_test() {
    struct device *dev = get_dev_by_major_minor(VIRTIO_MAJOR, 1);
    struct net_device *net_test = dev->get_interface(dev, NET_INTERFACE_BIT);
    uint8_t arp_packet[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Eth dst ff:ff:ff:ff:ff:ff
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56, // Eth src 52:54:00:12:34:56
        0x08, 0x00,             // IPv4 packet
        0x45, 0x00, 0x00, 0x20, // IP version = 4 Header length = 5 Length = 32 bytes
        0x00, 0x01, 0x00, 0x00, // IPv4 type: UDP
        0x40, 0x11, 0x9c, 0x3c, // IPv4 TTL = 64
        0xac, 0x13, 0x30, 0x7b, // IPv4 src 172.19.48.123
        0x01, 0x01, 0x01, 0x01, // IPv4 dst 1.1.1.1
        0x16, 0x2e, 0x04, 0xd2, // UDP dst port 5678(12 6e) UDP src port 1234(04 d2)
        0x00, 0x0c, 0x1e, 0x6c, // UDP length 12(8 + 4)
        0x74, 0x65, 0x73, 0x74  // payload "test"
    };
    net_test->send(dev, arp_packet, 46);
    return 0;
}

uint64_t reset_dev_test(uint64_t function) {
    struct device *dev = get_dev_by_major_minor(SIFIVE_TEST_MAJOR, 1);
    struct reset_device *reset_test = dev->get_interface(dev, RESET_INTERFACE_BIT);
#define SHUTDOWN_FUNCTION 0
#define REBOOT_FUNCTION 1
    switch (function)
    {
    case SHUTDOWN_FUNCTION:
        reset_test->shutdown(dev);
        break;
    case REBOOT_FUNCTION:
        reset_test->reboot(dev);
        break;
    default:
        break;
    }
    return 0;
}

uint64_t block_dev_test() {
    struct device *dev = get_dev_by_major_minor(VIRTIO_MAJOR, 1);
    struct block_device *block_test = dev->get_interface(dev, BLOCK_INTERFACE_BIT);
    char buffer[512];
    struct block_request req = {
        .is_read = 1,
        .sector = 0,
        .buffer = buffer,
        .wait_queue = NULL
    };
    block_test->request(dev, &req);
    for (uint64_t i = 0; i < 16; i += 1) {
        if(buffer[i] < 0x10) kprintf("0");
        kprintf("%x ", buffer[i]);
        if(i%8 == 7) kprintf(" ");
        if(i%16 == 15) kprintf("\n");
    }
    return 0;
}
