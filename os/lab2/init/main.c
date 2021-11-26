#include <riscv.h>
#include <clock.h>
#include <sbi.h>
#include <kdebug.h>
#include <trap.h>
#include <plic.h>
#include <uart.h>
#include <rtc.h>

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

	kprintf("timestamp now: %u\n", goldfish_rtc_read_time());
    goldfish_rtc_set_time(0);
	kprintf("timestamp now: %u\n", goldfish_rtc_read_time());
    uint64_t alarm_time = goldfish_rtc_read_time() + 1000000000;
    kprintf("alarm time: %u\n", alarm_time);
    goldfish_rtc_set_alarm(alarm_time); // alarm为毫秒时间戳
    kprintf("alarm time: %u\n", goldfish_rtc_read_alarm());

	enable_interrupt(); // 启用 interrupt，sstatus的SSTATUS_SIE位置1

	__asm__ __volatile__("ebreak \n\t");

	kputs("SYSTEM END");

	while (1)
		; /* infinite loop */
	return 0;
}
