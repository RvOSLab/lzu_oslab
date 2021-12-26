/**
 * @file trap.h
 * @author Hanabichan (93yutf@gmail.com)
 * @brief 本文件声明了中断处理相关的数据结构和函数
 */
#ifndef __TRAP_H__
#define __TRAP_H__

#include <stddef.h>

/**
 * @brief 保存通用寄存器的结构体抽象，用于trapframe结构体
 */
struct pushregs {
    uint64_t zero; // Hard-wired zero
    uint64_t ra;   // Return address
    uint64_t sp;   // Stack pointer
    uint64_t gp;   // Global pointer
    uint64_t tp;   // Thread pointer
    uint64_t t0;   // Temporary
    uint64_t t1;   // Temporary
    uint64_t t2;   // Temporary
    uint64_t s0;   // Saved register/frame pointer
    uint64_t s1;   // Saved register
    uint64_t a0;   // Function argument/return value
    uint64_t a1;   // Function argument/return value
    uint64_t a2;   // Function argument
    uint64_t a3;   // Function argument
    uint64_t a4;   // Function argument
    uint64_t a5;   // Function argument
    uint64_t a6;   // Function argument
    uint64_t a7;   // Function argument
    uint64_t s2;   // Saved register
    uint64_t s3;   // Saved register
    uint64_t s4;   // Saved register
    uint64_t s5;   // Saved register
    uint64_t s6;   // Saved register
    uint64_t s7;   // Saved register
    uint64_t s8;   // Saved register
    uint64_t s9;   // Saved register
    uint64_t s10;  // Saved register
    uint64_t s11;  // Saved register
    uint64_t t3;   // Temporary
    uint64_t t4;   // Temporary
    uint64_t t5;   // Temporary
    uint64_t t6;   // Temporary
};

/**
 * @brief 保存上下文的栈的结构体抽象，和trap.S中分配的36*XLENB相对应
 */
struct trapframe {
    struct pushregs gpr; // x0-x31 (general purpose register)
    uint64_t status;     // sstatus
    uint64_t epc;        // sepc
    uint64_t badvaddr;   // stval
    uint64_t cause;      // scause
};

extern void set_stack(char *stack);
extern void __trapret();
extern void __alltraps();

struct trapframe * trap(struct trapframe *tf);
void set_stvec();
void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *gpr);
int trap_in_kernel(struct trapframe *tf);
#endif
