/* 有关 Linker Script 可以参考：https://sourceware.org/binutils/docs/ld/Scripts.html */

/* 目标架构 */
OUTPUT_ARCH(riscv)

/*
 * 起始地址 asm/page.h: __START_KERNEL
 *
 */

BASE_ADDRESS = 0xffffffff80200000;

SECTIONS
{
    /* . 表示当前地址（location counter） */
    . = ALIGN(4K);
    . = BASE_ADDRESS;

    /* .text 段 */
    __text = .;
    .text : {
        /* 把 entry 函数放在最前面 */
        *(.text.entry)
        /* 要链接的文件的 .text 字段集中放在这里 */
        *(.text .text.*)
    }

    /* .rodata 段 */
    . = ALIGN(4K);
    __text_end = .;
    __rodata = .;
    .rodata : {
        /* 要链接的文件的 .rodata 字段集中放在这里 */
        *(.rodata .rodata.*)
    }

    /* .data 段 */
    . = ALIGN(4K);
    __rodata_end = .;
    __data = .;
    .data : {
        /* 要链接的文件的 .data 字段集中放在这里 */
        *(.data .data.*)
    }

    /* .bss 字段 */
    . = ALIGN(4K);
    __data_end = .;
     __bss = .;
    .bss : {
        /* 要链接的文件的 .bss 字段集中放在这里 */
        *(.sbss .bss .bss.*)
    }

    /* 结束地址 */
    . = ALIGN(4K);
    __bss_end = .;
    __end = .;
}
