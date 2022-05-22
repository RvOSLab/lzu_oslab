#include <device.h>
#include <device/reset/sifive_test.h>
#include <device/serial/uart8250.h>
#include <device/block/block_cache.h>
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

    char buffer[512];

    struct block_cache_request req = {
        .request_flag = BLOCK_READ,
        .length = 512,
        .offset = 0,
        .target = buffer
    };

    /* 清空块设备缓冲区 */
    block_cache_length = 0;
    block_cache_clip();

    /* 读测试 */
    block_cache_length = 3;
    int64_t read_seq[] = {1, 2, 1, 3, 2, 4};
    kprintf("read block: ");
    for (int64_t i = 0; i < ARRAY_SIZE(read_seq); i += 1) {
        kprintf("%u ", read_seq[i]);
        req.request_flag = BLOCK_READ;
        req.offset = read_seq[i] * 512;
        block_cache_request(dev, &req);
    }
    kputs("");

    /* 打印块设备缓冲区LRU链表 */
    struct linked_list_node *node;
    kprintf("cached block: ");
    for_each_linked_list_node(node, &block_cache_list) {
        struct block_cache *cache = container_of(node, struct block_cache, list_node);
        kprintf("%u ", cache->block_idx);
    }
    kputs("");

    // /* 清空块设备缓冲区 */
    // block_cache_length = 0;
    // block_cache_clip();

    // /* 写测试 */
    // memset(buffer, 'A', 512);
    // block_cache_length = 3;
    // kprintf("write block('A' * 512): ");
    // for (int64_t i = 0; i < ARRAY_SIZE(read_seq); i += 1) {
    //     kprintf("%u ", read_seq[i]);
    //     req.request_flag = BLOCK_WRITE;
    //     req.offset = read_seq[i] * 512;
    //     block_cache_request(dev, &req);
    // }

    // /* 打印块设备缓冲区LRU链表 */
    // kprintf("buffered block: ");
    // for_each_linked_list_node(node, &block_cache_list) {
    //     struct block_cache *cache = container_of(node, struct block_cache, list_node);
    //     kprintf("%u ", cache->block_idx);
    // }
    // kputs("\n");

    // // kprintf("read: %u bytes\n", ret);
    // for (uint64_t i = 0; i < 64; i += 1) {
    //     if(buffer[i] < 0x10) kprintf("0");
    //     kprintf("%x ", buffer[i]);
    //     if(i%8 == 7) kprintf(" ");
    //     if(i%16 == 15) kprintf("\n");
    // }
    return 0;
}
