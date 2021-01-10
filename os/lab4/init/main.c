#include <stddef.h>
#include <assert.h>
#include <sbi.h>
#include <kdebug.h>
#include <mm.h>
int main()
{
	print_system_infomation();
    mem_init();
	kputs("Hello LZU OS");
	while (1)
		; /* infinite loop */
	return 0;
}
