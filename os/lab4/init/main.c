#include <stddef.h>
#include <assert.h>
#include <sbi.h>
#include <kdebug.h>
#include <mm.h>

int main()
{
	print_system_infomation();
	kputs("Hello LZU OS");
    mem_init();
	while (1)
		; /* infinite loop */
	return 0;
}
