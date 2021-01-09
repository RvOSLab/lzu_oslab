#include <sbi.h>
void
sbi_set_timer(uint64_t stime_value)
{
    register uint64_t a0 asm ("x10") = stime_value;
    register uint64_t a6 asm ("x16") = 0;
    register uint64_t a7 asm ("x17") = TIMER_EXTENTION;
    __asm__ __volatile__(
        "ecall \n\t"
        : /* empty output list */
        : "r" (a0), "r" (a6), "r" (a7)
        : "memory"
        );
}

void 
sbi_console_putchar(int ch)
{
    register int a0 asm ("x10") = ch;
    register uint64_t a6 asm ("x16") = 0;
    register uint64_t a7 asm ("x17") = 1;
    __asm__ __volatile__ (
        "ecall \n\t"
        : /* empty output list */
        : "r" (a0), "r" (a6), "r" (a7)
        : "memory"
        );
}

int
sbi_console_getchar()
{
    register uint64_t a6 asm ("x16") = 0;
    register uint64_t a7 asm ("x17") = 2;
    register uint64_t ret asm ("x10");
    __asm__ __volatile__ (
        "ecall \n\t"
        : "+r" (ret)
        : "r" (a6), "r" (a7)
        : "memory"
        );
    return ret;
}

struct sbiret 
sbi_get_spec_version()
{
    register uint64_t a7 asm ("x17") = BASE_EXTENSTION;
    register uint32_t a6 asm ("x16") = 0;
    register uint64_t error asm ("x10");
    register uint64_t value asm ("x11");
    __asm__ __volatile__ (
        "ecall \n\t"
        : "=r" (error), "=r" (value)
        : "r" (a6), "r" (a7)
        : "memory"
        ); 
    return (struct sbiret) {error, value};
}

struct sbiret 
sbi_get_impl_id()
{
    register uint64_t a7 asm ("x17") = BASE_EXTENSTION;
    register uint32_t a6 asm ("x16") = 1;
    register uint64_t error asm ("x10");
    register uint64_t value asm ("x11");
    __asm__ __volatile__ (
        "ecall \n\t"
        : "=r" (error), "=r" (value)
        : "r" (a6), "r" (a7)
        : "memory"
        ); 
    return (struct sbiret) {error, value};
}

struct sbiret
sbi_get_impl_version()
{
    register uint64_t a7 asm ("x17") = BASE_EXTENSTION;
    register uint32_t a6 asm ("x16") = 2;
    register uint64_t error asm ("x10");
    register uint64_t value asm ("x11");
    __asm__ __volatile__ (
        "ecall \n\t"
        : "=r" (error), "=r" (value)
        : "r" (a6), "r" (a7)
        : "memory"
        ); 
    return (struct sbiret) {error, value};
}

struct sbiret
sbi_probe_extension(long extension_id)
{
    register uint64_t a7 asm ("x17") = BASE_EXTENSTION;
    register uint64_t a6 asm ("x16") = 3;
    register uint64_t error asm ("x10") = (uint64_t)extension_id;
    register uint64_t value asm ("x11");
    __asm__ __volatile__ (
        "ecall \n\t"
        : "+r" (error), "=r" (value)
        :  "r" (a6), "r" (a7)
        : "memory"
        ); 
    return (struct sbiret) {error, value};
}

void
sbi_shutdown()
{
    register uint64_t a7 asm ("x17") = SHUT_DOWN_EXTENTION;
    register uint64_t a6 asm ("x16") = 0;
    __asm__ __volatile__(
        "ecall \n\t"
        : /* empty output list */
        : "r" (a6), "r" (a7)
        : "memory"
        );
}
