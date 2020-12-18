#include <stddef.h>
#include <sbi.h>
#include <kdebug.h>
#include <mm.h>

int 
main()
{
    print_system_infomation();
    kputs("Hello LZU OS");
    kputs("running mem_init()");
    mem_init();
    kputs("running mem_test()");
    mem_test();
    while (1)
        ; /* infinite loop */
    return 0;
}



