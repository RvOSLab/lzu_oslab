#include <sbi.h>
void kputchar(char ch);
int kputs(const char *msg);

int main()
{
    kputs("\nLZU OS STARTING....................");
    struct sbiret ret;
    ret = sbi_probe_extension(TIMER_EXTENTION);
    if (ret.value != 0)
        kputs("TIMER_EXTENTION: available");
    else
        kputs("TIMER_EXTENTION: unavailable");

    ret = sbi_probe_extension(HART_STATE_EXTENTION);
    if (ret.value != 0)
        kputs("HART_STATE_EXTENTION: available");
    else
        kputs("HART_STATE_EXTENTION: unavailable");

    ret = sbi_probe_extension(RESET_EXTENTION);
    if (ret.value != 0)
        kputs("RESET_EXTENTION: available");
    else
        kputs("RESET_EXTENTION: unavailable");

    ret = sbi_get_impl_id();
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
    kputs("Hello LZU OS");
    while (1)
        ; /* infinite loop */
    return 0;
}

void kputchar(char ch)
{
    sbi_console_putchar(ch);
}

int kputs(const char *msg)
{
    const char *ret = msg;
    for (; *msg != '\0'; ++msg)
    {
        kputchar(*msg);
    }
    kputchar('\n');
    return ret == msg;
}
