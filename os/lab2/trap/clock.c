#include <clock.h>
#include <sbi.h>
#include <riscv.h>
#include <kdebug.h>

volatile size_t ticks;
static uint64_t timebase;

static inline uint64_t get_cycles(void) {
    uint64_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
}

/* *
 * clock_init - initialize 8253 clock to interrupt 100 times per second,
 * and then enable IRQ_TIMER.
 * */
void clock_init(void) {
    // QEMU clock frequency: 10MHz
    // set timebase = 100000 means clock interrupt frequency is 100Hz 
    timebase = 100000;
    // initialize time counter 'ticks' to zero
    ticks = 0;
    // 开启时钟
    //set_csr(CSR_MIE, MIP_MTIP);
    set_csr(0x104, 1<<5);
    clock_set_next_event();
    kputs("Setup Timer!");
}

void clock_set_next_event(void) { sbi_set_timer(get_cycles() + timebase); }
