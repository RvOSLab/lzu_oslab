#include <sbi.h>
#include <kdebug.h>
#include <mm.h>
#include <trap.h>
#include <sched.h>
#include <clock.h>
#include <syscall.h>

/* 创建系统调用 testsyscall() */
_syscall0(int, testsyscall)

int main()
{
    print_system_infomation();
    mem_init();
    mem_test();
    trap_init();
    sched_init();
    clock_init();
    int i = 10000;
    while (i--);
    kputs("Hello LZU OS");
    /* 使用 ecall 实现系统调用，此时系统处于内核态，
     * 系统认为 ecall 是 SBI 调用，因此无现象*/
    testsyscall();
    while (1)
        ; /* infinite loop */
    return 0;
}
