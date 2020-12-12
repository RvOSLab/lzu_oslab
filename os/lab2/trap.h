#ifndef __TRAP_H__
#define __TRAP_H__

#include <stddef.h>

// 保存通用寄存器的结构体抽象，用于trapframe结构体
struct pushregs {
    uintptr_t zero;  // Hard-wired zero
    uintptr_t ra;    // Return address
    uintptr_t sp;    // Stack pointer
    uintptr_t gp;    // Global pointer
    uintptr_t tp;    // Thread pointer
    uintptr_t t0;    // Temporary
    uintptr_t t1;    // Temporary
    uintptr_t t2;    // Temporary
    uintptr_t s0;    // Saved register/frame pointer
    uintptr_t s1;    // Saved register
    uintptr_t a0;    // Function argument/return value
    uintptr_t a1;    // Function argument/return value
    uintptr_t a2;    // Function argument
    uintptr_t a3;    // Function argument
    uintptr_t a4;    // Function argument
    uintptr_t a5;    // Function argument
    uintptr_t a6;    // Function argument
    uintptr_t a7;    // Function argument
    uintptr_t s2;    // Saved register
    uintptr_t s3;    // Saved register
    uintptr_t s4;    // Saved register
    uintptr_t s5;    // Saved register
    uintptr_t s6;    // Saved register
    uintptr_t s7;    // Saved register
    uintptr_t s8;    // Saved register
    uintptr_t s9;    // Saved register
    uintptr_t s10;   // Saved register
    uintptr_t s11;   // Saved register
    uintptr_t t3;    // Temporary
    uintptr_t t4;    // Temporary
    uintptr_t t5;    // Temporary
    uintptr_t t6;    // Temporary
};

// 保存上下文的栈的结构体抽象，和trap.S中分配的36*XLENB相对应
struct trapframe {
    struct pushregs gpr;    // x0-x31
    uintptr_t status;   // sstatus
    uintptr_t epc;      // sepc
    uintptr_t badvaddr; // stval 实际常用于存放需要访问但是不在内存中的地址
    uintptr_t cause;    // scause
};

void trap(struct trapframe *tf);    // 中断处理程序（从汇编的中断处理程序跳转过来处理）
void idt_init(void);                // 初始化中断
void print_trapframe(struct trapframe *tf); // 打印trapframe（上下文）
void print_regs(struct pushregs* gpr);  // 打印所有通用寄存器
int trap_in_kernel(struct trapframe *tf);   // 检测中断是否发生在内核态，返回1=true，0=false

#endif