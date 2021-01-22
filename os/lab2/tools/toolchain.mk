# 以下是工具链的定义，其中部分是 Makefile 的默认变量，使用 := 符号定义可以覆盖原变量的定义
# CC - 编译器；LD - 链接器；OBJCOPY - 二进制拷贝程序；OBJDUMP - 反汇编程序
# NM - 符号提取程序；AS - 编译器；GDB - 调试程序；TMUX - 终端复用器；QEMU - RISC-V模拟器
CC := riscv64-linux-gnu-gcc
LD := riscv64-linux-gnu-ld
AR := riscv64-linux-gnu-ar
OBJCOPY := riscv64-linux-gnu-objcopy
OBJDUMP := riscv64-linux-gnu-objdump
READELF := riscv64-linux-gnu-readelf
NM := riscv64-linux-gnu-nm
AS := riscv64-linux-gnu-as
GDB := riscv64-unknown-elf-gdb
TMUX := tmux
QEMU := qemu-system-riscv64
