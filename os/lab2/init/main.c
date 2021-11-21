#include <riscv.h>
#include <clock.h>
#include <sbi.h>
#include <kdebug.h>
#include <trap.h>
#include <plic.h>
#include <uart.h>


int main()
{
    kputs("\nLZU OS STARTING....................");
    print_system_infomation();
    kputs("Hello LZU OS");

    set_stvec();
    clock_init();
    kprintf("complete timer init\n");
    plic_init();
    kprintf("complete plic init\n");
    uart_init();
    kprintf("complete uart init\n");
    enable_interrupt();     // 启用 interrupt，sstatus的SSTATUS_SIE位置1    

    __asm__ __volatile__("ebreak \n\t");

    kputs("SYSTEM END");

    while (1)
        ; /* infinite loop */
    return 0;
}
