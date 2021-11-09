# 以下是工具链的定义，其中部分是 Makefile 的默认变量，使用 := 符号定义可以覆盖原变量的定义
# CC - 编译器；LD - 链接器；OBJCOPY - 二进制拷贝程序；OBJDUMP - 反汇编程序
# NM - 符号提取程序；AS - 编译器；GDB - 调试程序；TMUX - 终端复用器；QEMU - RISC-V模拟器
ifneq ($(wildcard $(foreach p,$(subst :, ,$(PATH)),$(p)/riscv64-linux-gnu-*)),)
	PREFIX := riscv64-linux-gnu
else
	PREFIX := riscv64-unknown-elf
endif

CC := $(PREFIX)-gcc
LD := $(PREFIX)-ld
AR := $(PREFIX)-ar
OBJCOPY := $(PREFIX)-objcopy
OBJDUMP := $(PREFIX)-objdump
READELF := $(PREFIX)-readelf
NM := $(PREFIX)-nm
AS := $(PREFIX)-as
TMUX := tmux
QEMU := qemu-system-riscv64
GDB := riscv64-unknown-elf-gdb
