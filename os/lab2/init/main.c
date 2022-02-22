#include <riscv.h>
#include <clock.h>
#include <sbi.h>
#include <kdebug.h>
#include <trap.h>
#include <plic.h>
#include <uart.h>
#include <rtc.h>
#include <dtb.h>

int main(void *nothing, const void *dtb_start)
{
    kputs("\nLZU OS STARTING....................");
    print_system_infomation();
    kputs("Hello LZU OS");

    set_stvec();
    // clock_init();
    kprintf("complete timer init\n");
    //dtb_start = (const void *)0x82200000;
    unflatten_device_tree(dtb_start);

    plic_init();
    kprintf("complete plic init\n");
    uart_init();
    kprintf("complete uart init\n");

    rtc_init();
    kprintf("timestamp now: %u\n", read_time());
    set_alarm(read_time() + 1000000000);
    kprintf("alarm time: %u\n", read_alarm());

    enable_interrupt(); // 启用 interrupt，sstatus的SSTATUS_SIE位置1

    game_start();
    while (1)
        ; /* infinite loop */

    return 0;
}
