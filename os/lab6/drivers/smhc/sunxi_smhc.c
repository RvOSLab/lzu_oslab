#include <device/smhc/sunxi_smhc.h>
#include <device/irq.h>
#include <kdebug.h>
#include <riscv.h>
#include <sched.h>
void sunxi_smhc_read_irq_handler(struct device *dev);
uint64_t sunxi_smhc_request(struct device *dev, void *buffer, uint64_t size, uint64_t opration);
void sunxi_smhc_init(struct device *dev);

struct driver_resource sunxi_smhc_mmio_res = {
    .resource_start = 0x04020000, // The SMHC0 controls the devices that comply with the Secure Digital Memory (SD mem-version 3.0)
    .resource_end = 0x04020200,
    .resource_type = DRIVER_RESOURCE_MEM};
/*
#define sunxi_smhc_BUFF_LEN 1024
uint8_t sunxi_smhc_rx_buffer[sunxi_smhc_BUFF_LEN];
uint64_t sunxi_smhc_rx_buffer_start = 0;
uint64_t sunxi_smhc_rx_buffer_end = 0;
uint64_t sunxi_smhc_rx_buffer_empty = 1;
struct task_struct *sunxi_smhc_rx_buffer_wait = NULL;
*/
struct smhc_device sunxi_smhc_device = {
    .init = sunxi_smhc_init,
    .request = sunxi_smhc_request,
};

struct irq_descriptor sunxi_smhc_read_irq = {
    .name = "sunxi smhc read irq handler",
    .handler = sunxi_smhc_read_irq_handler,
};

static struct sunxi_smhc_regs *get_regs(struct device *dev)
{
    struct linked_list_node *mem_res_node;
    for_each_linked_list_node(mem_res_node, &(dev->resource_list))
    {
        if (container_of(mem_res_node, struct driver_resource, list_node)->resource_type == DRIVER_RESOURCE_MEM)
            break;
    }

    return (struct sunxi_smhc_regs *)mem_res_node;
}

void sunxi_smhc_read_irq_handler(struct device *dev)
{
    struct sunxi_smhc_regs *regs = get_regs(dev);
}

void sunxi_smhc_init(struct device *dev)
{
    struct sunxi_smhc_regs *regs = get_regs(dev);
    // Configure the corresponding GPIO register as an SMHC by Port Controller module
    *(uint64_t *)(0x02000000 + 0x00F0) = 0x0F222222; // set PF0-PF5 to SDC function, disable PF6
    // reset clock by writing 1 to SMHC_BGR_REG[SMHC0_RST]
    uint64_t *SMHC_BGR_REG = (uint64_t *)(0x02001000 + 0x084C);
    *SMHC_BGR_REG |= (1 << SMHC0_RST);
    // open clock gating by writing 1 to SMHC_BGR_REG[SMHCx_GATING]
    *SMHC_BGR_REG |= (1 << SMHC0_GATING);
    // select clock sources and set the division factor by configuring the SMHCx_CLK_REG (x = 0, 1) register.
    uint64_t *SMHC0_CLK_REG = (uint64_t *)(0x02001000 + 0x0830);
    *SMHC0_CLK_REG = 0x80000000;
    // Configure SMHC_CTRL to reset the FIFO and controller, and enable the global interrupt
    regs->SMHC_CTRL |= 0x37;
    // configure SMHC_INTMASK to 0xFFCE to enable normal interrupts and error abnormal interrupts, and then register the interrupt function.
    regs->SMHC_INTMASK = 0xFFCE; // reg
    // Configure SMHC_CLKDIV to open clock for devices
    regs->SMHC_CLKDIV |= (1 << CCLK_ENB);
    // configure SMHC_CMD as the change clock command (for example 0x80202000); send the update clock command to deliver clocks to devices.
    regs->SMHC_CMD = 0x80202000;
    // Configure SMHC_CMD as a normal command. 
    regs->SMHC_CMD|= 0x3000000;
    // Configure SMHC_CMDARG to set command parameters. 
    // Configure SMHC_CMD to set parameters like whether to send the response, the response type, and the response length and then send the commands. According to the initialization process in the protocol, you can finish SMHC initialization by sending the corresponding command one by one.
}

uint64_t sunxi_smhc_request(struct device *dev, void *buffer, uint64_t size, uint64_t opration)
{
    struct sunxi_smhc_regs *regs = get_regs(dev);
}

/*
void sunxi_smhc_rx_irq_handler(struct device *dev) {
    struct uart_qemu_regs *regs = (struct uart_qemu_regs *)sunxi_smhc_mmio_res.map_address;
    wake_up(&sunxi_smhc_rx_buffer_wait);
    while (regs->LSR & (1 << LSR_DR)) {
        sunxi_smhc_rx_buffer_empty = 0;
        sunxi_smhc_rx_buffer[sunxi_smhc_rx_buffer_end] = regs->RBR_THR_DLL;
        sunxi_smhc_rx_buffer_end = (sunxi_smhc_rx_buffer_end + 1) % sunxi_smhc_BUFF_LEN;
        if (sunxi_smhc_rx_buffer_start == sunxi_smhc_rx_buffer_end) { // full
            kprintf("buffer: ");
            for(uint64_t i=0;i<sunxi_smhc_BUFF_LEN;i++) {
                kputchar(sunxi_smhc_rx_buffer[(i + sunxi_smhc_rx_buffer_end) % sunxi_smhc_BUFF_LEN]);
            }
            kputchar('\n');
            sunxi_smhc_rx_buffer_start = (sunxi_smhc_rx_buffer_start + 1) % sunxi_smhc_BUFF_LEN;
        }
    }
}

struct irq_descriptor sunxi_smhc_rx_irq = {
    .name = "sunxi_smhc rx irq handler",
    .handler = sunxi_smhc_rx_irq_handler
};

void sunxi_smhc_init(struct device *dev) {
    device_add_resource(dev, &sunxi_smhc_mmio_res);
    struct uart_qemu_regs *regs = (struct uart_qemu_regs *)sunxi_smhc_mmio_res.map_address;

    regs->IER_DLM = 0; // 关闭 16550a 的所有中断，避免初始化未完成就发生中断
    while(!(regs->LSR & (1 << LSR_THRE))); // 等待发送缓冲区为空

    // 设置波特率
    uint8_t divisor_least = 592 & 0xFF;
    uint8_t divisor_most = 592 >> 8;
    regs->LCR |= 1 << LCR_DLAB; // 设置 DLAB=1，进入波特率设置模式，访问 DLL 和 DLM 寄存器设置波特率
    regs->RBR_THR_DLL = divisor_least; // 设置 DLL
    regs->IER_DLM = divisor_most; // 设置 DLM

    regs->LCR = 0b00000011; // 一次传输 8bit（1字节），无校验，1 位停止位，禁用 break 信号，设置 DLAB=0，进入数据传输/中断设置模式
    regs->IIR_FCR |= 0b00000001; // 设置 FCR[TL]=00，设置中断阈值为 1 字节，设置 FCR[FIFOE]=1，启动 FIFO
    regs->IER_DLM |= 1 << IER_ERBFI; // 设置 IER，启用接收数据时发生的中断

    irq_add(0, 0x0a, &sunxi_smhc_rx_irq);
}

uint64_t sunxi_smhc_request(struct device *dev, void *buffer, uint64_t size, uint64_t is_read) {
    char *char_buffer = (char *)buffer;
    if (is_read) {
        for (uint64_t i = 0; i < size; i += 1) {
            if (sunxi_smhc_rx_buffer_empty) sleep_on(&sunxi_smhc_rx_buffer_wait);
            char_buffer[i] = sunxi_smhc_rx_buffer[sunxi_smhc_rx_buffer_start];
            sunxi_smhc_rx_buffer_start = (sunxi_smhc_rx_buffer_start + 1) % sunxi_smhc_BUFF_LEN;
            if (sunxi_smhc_rx_buffer_start == sunxi_smhc_rx_buffer_end) { // empty
                sunxi_smhc_rx_buffer_empty = 1;
            }
        }
        return size;
    } else {// !is_read
        struct uart_qemu_regs *regs = (struct uart_qemu_regs *)sunxi_smhc_mmio_res.map_address;
        for (uint64_t i = 0; i < size; i += 1) {
            while (!(regs->LSR & (1 << LSR_THRE)));
            regs->RBR_THR_DLL = char_buffer[i];
        }
        return size;
    }
}
*/

void *sunxi_smhc_get_interface(struct device *dev, uint64_t flag)
{
    if (flag & SMHC_INTERFACE_BIT)
        return &sunxi_smhc_device;
    return NULL;
}

uint64_t sunxi_smhc_device_probe(struct device *dev)
{
    device_init(dev);
    sunxi_smhc_init(dev);
    device_set_data(dev, NULL);
    sunxi_smhc_device.dev = dev;
    device_set_interface(dev, SMHC_INTERFACE_BIT, sunxi_smhc_get_interface);
    device_register(dev, "sunxi_smhc", SUNXI_SMHC_MAJOR, NULL);
    device_add_resource(dev, &sunxi_smhc_mmio_res);
    return 0;
}

struct driver_match_table sunxi_smhc_match_table[] = {
    {.compatible = "allwinner,sunxi-mmc-v5p3x"},
    {NULL},
};

struct device_driver sunxi_smhc_driver = {
    .driver_name = "sunxi_smhc driver",
    .match_table = sunxi_smhc_match_table,
    .device_probe = sunxi_smhc_device_probe,
};
