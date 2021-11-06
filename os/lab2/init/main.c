#include <sbi.h>
#include <trap.h>
#include <clock.h>
#include <kdebug.h>

int main()
{
    kputs("\nLZU OS STARTING....................");
    print_system_infomation();
    kputs("Hello LZU OS");

    trap_init();
    clock_init();

    __asm__ __volatile__("ebreak \n\t");

    kputs("SYSTEM END");

    while (1)
        ; /* infinite loop */
    return 0;
}
