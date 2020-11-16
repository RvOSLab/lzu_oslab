#include <stddef.h>
#include <sbi.h>
static void kputs(const char *msg);

int 
main()
{
    kputs("Hello World");
    while (1)
        ; /* infinite loop */
    return 0;
}


static void
kputs(const char *msg)
{
    for ( ; *msg != '\0'; ++msg )
    {
        sbi_console_putchar(*msg);
    }
    sbi_console_putchar('\n');
}

