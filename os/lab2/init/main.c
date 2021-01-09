#include <stddef.h>
#include <sbi.h>
#include <riscv.h>
#include <trap.h>
#include <clock.h>

int 
main()
{
    struct sbiret ret;
    ret = sbi_probe_extension(TIMER_EXTENTION);
    if (ret.value != 0)
        kputs("TIMER_EXTENTION: available");
    else 
        kputs("TIMER_EXTENTION: unavailable");

    ret = sbi_probe_extension(SHUT_DOWN_EXTENTION);
    if (ret.value != 0)
        kputs("SHUT_DOWN_EXTENTION: available");
    else 
        kputs("SHUT_DOWN_EXTENTION: unavailable");

    ret = sbi_probe_extension(HART_STATE_EXTENTION);
    if (ret.value != 0)
        kputs("HART_STATE_EXTENTION: available");
    else 
        kputs("HART_STATE_EXTENTION: unavailable");

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

    idt_init();
    clock_init();

    __asm__ __volatile__(
        "ebreak \n\t"
        );

    kputs("SYSTEM END");



    while (1)
        ; /* infinite loop */
    return 0;
}
