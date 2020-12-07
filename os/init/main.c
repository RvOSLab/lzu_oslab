#include <stddef.h>
#include <sbi.h>
#include <mm.h>

int 
main()
{
    print_system_infomation();
    kputs("Hello LZU OS");
    mem_init(MEM_START, MEM_END);
    mem_test(); 
    while (1)
        ; /* infinite loop */
    return 0;
}



