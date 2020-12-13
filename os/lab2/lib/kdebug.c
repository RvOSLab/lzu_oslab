#include <kdebug.h>
#include <string.h>

void
kputchar(int ch)
{
    sbi_console_putchar(ch);
}

int
kputs(const char *msg)
{
    const char *ret = msg;
    for ( ; *msg != '\0'; ++msg )
    {
        kputchar(*msg);
    }
    kputchar('\n');
    return ret == msg;
}

void
do_panic(const char* file, int line, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    /// kprintf("--------------------------------------------------------------------------\n");
    /// kprintf("Panic at %s: %d\n", file, line);
    if (strlen(fmt))
    {
        /// kprintf("Assert message: ");
        /// kprintf(fmt, ap);
        kputs(fmt);
    }
    kputchar('\n');
    va_end(ap);
    sbi_shutdown();
    /// kprintf("Shutdown machine\n");
    /// kprintf("--------------------------------------------------------------------------\n");
}
