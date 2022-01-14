#include <sbi.h>
#include <kdebug.h>

void sbi_set_timer(uint64_t stime_value)
{
    register uint64_t a0 asm("a0") = stime_value;
    register uint64_t a6 asm("a6") = 0;
    register uint64_t a7 asm("a7") = TIMER_EXTENTION;
    __asm__ __volatile__("ecall \n\t"
                 : /* empty output list */
                 : "r"(a0), "r"(a6), "r"(a7)
                 : "memory");
}

void sbi_console_putchar(char ch)
{
    register uint64_t a0 asm("a0") = ch;
    register uint64_t a6 asm("a6") = 0;
    register uint64_t a7 asm("a7") = 1;
    __asm__ __volatile__("ecall \n\t"
                 : /* empty output list */
                 : "r"(a0), "r"(a6), "r"(a7)
                 : "memory");
}

char sbi_console_getchar()
{
    register uint64_t a6 asm("a6") = 0;
    register uint64_t a7 asm("a7") = 2;
    register uint64_t ret asm("a0");
    __asm__ __volatile__("ecall \n\t"
                 : "+r"(ret)
                 : "r"(a6), "r"(a7)
                 : "memory");
    return ret;
}

struct sbiret sbi_get_spec_version()
{
    register uint64_t a7 asm("a7") = BASE_EXTENSTION;
    register uint32_t a6 asm("a6") = 0;
    register uint64_t error asm("a0");
    register uint64_t value asm("a1");
    __asm__ __volatile__("ecall \n\t"
                 : "=r"(error), "=r"(value)
                 : "r"(a6), "r"(a7)
                 : "memory");
    return (struct sbiret){ error, value };
}

struct sbiret sbi_get_impl_id()
{
    register uint64_t a7 asm("a7") = BASE_EXTENSTION;
    register uint32_t a6 asm("a6") = 1;
    register uint64_t error asm("a0");
    register uint64_t value asm("a1");
    __asm__ __volatile__("ecall \n\t"
                 : "=r"(error), "=r"(value)
                 : "r"(a6), "r"(a7)
                 : "memory");
    return (struct sbiret){ error, value };
}

struct sbiret sbi_get_impl_version()
{
    register uint64_t a7 asm("a7") = BASE_EXTENSTION;
    register uint32_t a6 asm("a6") = 2;
    register uint64_t error asm("a0");
    register uint64_t value asm("a1");
    __asm__ __volatile__("ecall \n\t"
                 : "=r"(error), "=r"(value)
                 : "r"(a6), "r"(a7)
                 : "memory");
    return (struct sbiret){ error, value };
}

struct sbiret sbi_probe_extension(long extension_id)
{
    register uint64_t a7 asm("a7") = BASE_EXTENSTION;
    register uint64_t a6 asm("a6") = 3;
    register uint64_t error asm("a0") = (uint64_t)extension_id;
    register uint64_t value asm("a1");
    __asm__ __volatile__("ecall \n\t"
                 : "+r"(error), "=r"(value)
                 : "r"(a6), "r"(a7)
                 : "memory");
    return (struct sbiret){ error, value };
}

void sbi_shutdown()
{
    register uint64_t a0 asm("a0") = 0;
    register uint64_t a1 asm("a1") = 0;
    register uint64_t a7 asm("a7") = RESET_EXTENTION;
    register uint64_t a6 asm("a6") = 0;
    __asm__ __volatile__("ecall \n\t"
                 : /* empty output list */
                 : "r"(a0), "r"(a1), "r"(a6), "r"(a7)
                 : "memory");
}

void print_system_infomation()
{
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
    switch (ret.value) {
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
