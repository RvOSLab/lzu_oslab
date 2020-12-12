    .globl boot_stack, boot_stack_top, _start
    .section .text.entry
_start:
    la sp, boot_stack_top
    call main

    .section .bss
boot_stack:
    .space 4096 * 16
boot_stack_top:


