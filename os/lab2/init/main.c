#include <stddef.h>
#include <sbi.h>
#include <riscv.h>
#include <trap.h>
#include <clock.h>
#include <kdebug.h>

int main()
{
	print_system_infomation();
	kputs("Hello LZU OS");

	idt_init();
	clock_init();

	__asm__ __volatile__("ebreak \n\t");

	kputs("SYSTEM END");

	while (1)
		; /* infinite loop */
	return 0;
}
