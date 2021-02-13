#include <sbi.h>
#include <kdebug.h>
#include <mm.h>
#include <trap.h>
#include <sched.h>
#include <clock.h>
#include <syscall.h>

int main()
{
    print_system_infomation();
    mem_init();
    mem_test();
    trap_init();
    sched_init();
    clock_init();
    kputs("Hello LZU OS");
    init_task0();
    while (1)
        ; /* infinite loop */
    return 0;
}
