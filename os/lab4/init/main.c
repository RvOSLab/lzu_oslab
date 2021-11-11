#include <sbi.h>
#include <kdebug.h>
#include <mm.h>
int main()
{
    kputs("\nLZU OS STARTING....................");
    print_system_infomation();
    mem_init();
    mem_test();
    uint64_t * new = (uint64_t *)malloc(100);
    kprintf("new: %p\n", new);
    free_s(new,100);
    kputs("Hello LZU OS");

    while (1)
        ; /* infinite loop */
    return 0;
}
