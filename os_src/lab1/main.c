#include <stddef.h>
#include <sbi.h>
static void kputs(const char *msg);

int 
main()
{
    struct sbiret ret;
    ret = sbi_probe_extension(TIMER_EXTENTION);
    if (ret.error == SBI_SUCCESS)
        kputs("TIMER_EXTENTION: available");
    else 
        kputs("TIMER_EXTENTION: unavailable");

    ret = sbi_probe_extension(SHUT_DOWN_EXTENTION);
    if (ret.error == SBI_SUCCESS)
        kputs("SHUT_DOWN_EXTENTION: available");
    else 
        kputs("SHUT_DOWN_EXTENTION: unavailable");

    ret = sbi_probe_extension(HART_STATE_EXTENTION);
    if (ret.error == SBI_SUCCESS)
        kputs("HART_STATE_EXTENTION: available");
    else 
        kputs("HART_STATE_EXTENTION: unavailable");

    ret = sbi_get_impl_id();
    if (ret.error != SBI_SUCCESS)
    {
        kputs("sbi_get_impl_id() failed");
    }
    else
    {
        switch (ret.value)
        {
            case BERKELY_BOOT_LOADER:
                kputs("Implemention ID: Berkely boot loader");
                break;
            case OPENSBI:
                kputs("Implemention ID: OpenSBI");
                break;
            case XVISOR:
                kputs("Implemention ID: XVISOR");
                break;
            case KVM:
                kputs("Implemention ID: KVM");
                break;
            case RUSTSBI:
                kputs("Implemention ID: RustSBI");
                break;
            case DIOSIX:
                kputs("Implemention ID: DIOSIX");
                break;
            default:
                kputs("Implemention ID: Unkonwn");
        }
    }
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

