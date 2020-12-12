#include <clock.h>
#include <sbi.h>
#include <riscv.h>

volatile size_t ticks;

static void kputs(const char *msg);

static void
kputs(const char *msg)
{
    for ( ; *msg != '\0'; ++msg )
    {
        sbi_console_putchar(*msg);
    }
    sbi_console_putchar('\n');
}



static inline uint64_t get_cycles(void) {
    uint64_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
}

static uint64_t timebase;

/* *
 * clock_init - initialize 8253 clock to interrupt 100 times per second,
 * and then enable IRQ_TIMER.
 * */
void clock_init(void) {
    // 设置100Hz的时钟，qemu运行频率为3686400Hz
    // divided by 100 when using QEMU(10MHz)
    timebase = 100000;
    // initialize time counter 'ticks' to zero
    ticks = 0;
    // 开启时钟
    //clear_csr(CSR_MIE, MIP_STIP);
    //set_csr(CSR_MIE, MIP_MTIP);
    set_csr(0x104, 1<<5);
    //kputs("set stip!");
    clock_set_next_event();
    kputs("Setup Timer!");
}

void clock_set_next_event(void) { sbi_set_timer(get_cycles() + timebase); }
