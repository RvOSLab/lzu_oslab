#include <sbi.h>
#include <kdebug.h>
#include <mm.h>
int main()
{
    kputs("\nLZU OS STARTING....................");
    print_system_infomation();
    mem_init();
    mem_test();
    uint64_t *new1 = (uint64_t *)kmalloc(1);
    kprintf("new: %p\n", new1);
    *new1=0x1234567812345678;
    kprintf("new: %x\n", *new1);
    uint64_t *new2 = (uint64_t *)kmalloc(1);
    kprintf("new: %x\n", *new1);
    uint64_t *new3 = (uint64_t *)kmalloc(1);

    kprintf("new: %p\n", new1);
    kprintf("new: %p\n", new2);
    kprintf("new: %p\n", new3);
    kfree_s(new2, 0);
    kputs("Hello LZU OS");

    while (1)
        ; /* infinite loop */
    return 0;
}
