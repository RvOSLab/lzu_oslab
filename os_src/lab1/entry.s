    .globl boot_stack
    .globl _start
    .section .text.entry
_start:
    la sp, boot_stack_top
    call main

    .section .bss
boot_stack:
    .space 4096 * 16
    .global boot_stack_top
boot_stack_top:


