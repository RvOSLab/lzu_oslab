#include <uart.h>
#include <dtb.h>
#include <string.h>

struct uart_class_device uart_device;
extern struct device_node node[100];
extern struct property prop[100];
extern int64_t node_used;
extern int64_t prop_used;

static uint64_t uart_probe()
{
    for (size_t i = 0; i < node_used + 20; i++) {
        if (is_begin_with(node[i].name, "uart")) {
            for (struct property *prop_ptr = node[i].properties; prop_ptr; prop_ptr = prop_ptr->next) {
                if (strcmp(prop_ptr->name, "compatible") == 0) {
                    if (strcmp(prop_ptr->value, "ns16550a") == 0) {
                        return UART_16550A;
                    } else if (strcmp(prop_ptr->value, "allwinner,sun20i-uart") == 0) {
                        return UART_SUNXI;
                    }
                }
            }
        }
    }
    return -1;
}

void uart_init()
{
    uart_device.id = uart_probe();
    switch (uart_device.id) {
    case UART_16550A:
        uart_16550a_init();
        break;
    case UART_SUNXI:
        //uart_sunxi_init();
        break;
    }
}

int8_t uart_read()
{
    return (*uart_device.ops.uart_read)();
}

void uart_write(int8_t c)
{
    (*uart_device.ops.uart_write)(c);
}

void uart_interrupt_handler()
{
    (*uart_device.ops.uart_interrupt_handler)();
}