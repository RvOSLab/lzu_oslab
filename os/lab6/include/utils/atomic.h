#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include <stddef.h>
#include <riscv.h>
#include <assert.h>

/* 用法：
    #include <utils/atomic.h>
    struct spinlock lock_a;
    init_lock(&lock_a,"lock_a");
    acquire_lock(&lock_a);
    release_lock(&lock_a);
*/

// Mutual exclusion lock.
struct spinlock
{
    uint64_t locked; // Is the lock held?

    // For debugging:
    char *name; // Name of lock.
};

static inline void init_lock(struct spinlock *lk, char *name)
{
    lk->name = name;
    lk->locked = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
static inline void acquire_lock(struct spinlock *lk)
{
    // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
    //   a5 = 1
    //   s1 = &lk->locked
    //   amoswap.w.aq a5, a5, (s1)
    while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
        ;

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen strictly after the lock is acquired.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();
}

// Release the lock.
static inline void release_lock(struct spinlock *lk)
{
    // Tell the C compiler and the CPU to not move loads or stores
    // past this point, to ensure that all the stores in the critical
    // section are visible to other CPUs before the lock is released,
    // and that loads in the critical section occur strictly before
    // the lock is released.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();

    // Release the lock, equivalent to lk->locked = 0.
    // This code doesn't use a C assignment, since the C standard
    // implies that an assignment might be implemented with
    // multiple store instructions.
    // On RISC-V, sync_lock_release turns into an atomic swap:
    //   s1 = &lk->locked
    //   amoswap.w zero, zero, (s1)
    __sync_lock_release(&lk->locked);
}

#endif
