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
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56,
        0x08, 0x06,
        0x00, 0x01,
        0x08, 0x00,
        0x06, 0x04,
        0x00, 0x01,
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56,
        10, 0, 0, 15, // src ip
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        10, 0, 0, 2   // dst ip
    };
    net_test->send(dev, arp_packet, 42);
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
    struct device *dev = get_dev_by_major_minor(VIRTIO_MAJOR, 2);
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
