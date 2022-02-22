#include <sbi.h>
#include <kdebug.h>
#include <mm.h>
#include <trap.h>
#include <sched.h>
#include <clock.h>
#include <syscall.h>
#include <device/loader.h>

int main(const char* args, const struct fdt_header *fdt)
{
    kputs("\nLZU OS STARTING....................");
    print_system_infomation();
    mem_init();
    mem_test();
    malloc_test();
    init_device_table();
    fdt_loader(fdt, driver_list);
    drivers_test();
    set_stvec();
    sched_init();
    clock_init();
    kputs("Hello LZU OS");
    enable_interrupt();
    init_task0();
    syscall(NR_fork);    /* task 0 creates task 1 */
    long local = syscall(NR_getpid) + 100;
    if (local == 101) syscall(NR_mapl);
    // syscall(NR_fork);    /* task 0 creates task 2, task 1 creates task 3 */
    // syscall(NR_test_fork, local);
    while (1)
        ; /* infinite loop */
    return 0;
}
