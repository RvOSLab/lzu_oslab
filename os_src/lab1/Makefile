# 以下是部分变量的定义，其中部分是 Makefile 的默认变量，使用 := 符号定义可以覆盖原变量的定义
# CC - 编译器；LD - 链接器；OBJCOPY - 二进制拷贝程序；OBJDUMP - 反汇编程序
# NM - 符号提取程序；AS - 编译器；GDB - 调试程序；TMUX - 终端复用器；QEMU - RISC-V模拟器
CC := riscv64-linux-gnu-gcc
LD := riscv64-linux-gnu-ld
OBJCOPY := riscv64-linux-gnu-objcopy
OBDDUMP := riscv64-linux-gnu-objdump
NM := riscv64-linux-gnu-nm
AS := riscv64-linux-gnu-as
GDB := riscv64-unknown-elf-gdb
TMUX := tmux
QEMU := qemu-system-riscv64

# 二进制文件，可直接启动系统
KERN_IMG := kernel.img
# 链接得到的 ELF 格式的可执行文件
KERN_BIN := kernel.bin
# 导出的内存布局文件
KERN_MAP := kernel.map
# 导出的符号表（存放了变量等符号信息）
KERN_SYM := kernel.sym
# 反汇编得到的汇编程序
KERN_ASM := kernel.asm

# -mcmodel=medany 适用于64位RISC-V的CPU（RV64I指令集），使一些C语言代码中的寻址操作可以被编译为auipc指令，用于相对寻址，可使程序的运行与程序被加载的位置无关
# -Wall 将所有的警告信息全开
# -g 添加适用于gdb的调试信息
# -fno-builtin 不使用 GCC 内建函数
# -fno-stack-protector 不使用栈保护
# -fno-strict-aliasing 允许未定义的类型间强制转换
# -nostdinc 不使用系统标准库
# -I . 优先在当前目录搜索头文件
CFLAGS := -mcmodel=medany -Wall -g -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I . 

# 创建一个文件名列表，列表内容是所有.c文件对应的.o文件（还未创建，将在下面被创建），最后加上entry.o（对应 entry.s）
# wildcard 函数返回通配符匹配到的文件名，文件间以空格分隔。
# patsubst 函数进行模式替换，3个参数：需要匹配的模式，用什么来替换它，需要被处理的由空格分隔的字列。
objects := $(patsubst %.c, %.o, $(wildcard *.c)) entry.o

# .PHONY表示后面这些都是伪造的target，无论同名文件是否存在都会运行
.PHONY : all build run run-gui symbol debug clean disassembly

# make 不加参数默认选取第一个 target 运行，此处 all 就是第一个 target，要想生成 target 需要先决条件 build（见下）
all : build

# build，先决条件是 $(KERN_BIN)，也即 kernel.bin（见下）
build : $(KERN_BIN)

# $(KERN_BIN)，先决条件是 $(objects) 里所包含的文件
# 我们没有手动写对 $(objects) 的生成规则， make 命令会使用自身的隐含规则，读取所有对应的源代码文件（本例中是.c文件和.s文件）来自动编译成.o文件
# 后面的第一条命令是按照 linker.ld 文件描述的内存布局来链接所有的.o目标文件（即$^指代的先决条件），链接后生成的文件名为 $(KERN_BIN)，然后导出内存布局至 $(KERN_MAP)
# 第二条命令，使用 objcopy 将生成的文件里的裸二进制内容拷贝出来到 $(KERN_IMG) 文件中。
# 拷贝二进制的原因：ld 生成的文件默认为 ELF 可执行文件格式，需要借助 Linux 的 ELF 加载器来执行，我们的目的是写一个全新的操作系统，在裸机上不会有 ELF 加载器帮助我们加载，因此需要去掉所有 ELF 文件相关信息，转换为一种平铺模式的二进制文件，使 cpu 直接转到一个指令后，就能顺序执行，不受 ELF 文件信息的干扰
$(KERN_BIN) : $(objects)
	$(LD) -T linker.ld -M=$(KERN_MAP) -o $(KERN_BIN) $^
	$(OBJCOPY) $(KERN_BIN) --strip-all -O binary $(KERN_IMG)

# run，启动 TUI 的 QEMU 运行操作系统，先决条件是 build （见上）
# bios 使用 QEMU 默认的 BIOS，即使用 QEMU 自带的 OpenSBI v0.7。
run : build
	@$(QEMU) \
    		-machine virt \
    		-nographic \
    		-bios default \
    		-device loader,file=$(KERN_IMG),addr=0x80200000 

# run-gui，启动图形形式的 QEMU 运行操作系统，需要 vncviewer 连接，先决条件是 build （见上）
run-gui : build
	@$(QEMU) \
    		-machine virt \
    		-bios default \
    		-device loader,file=$(KERN_IMG),addr=0x80200000 

# debug，启动 QEMU 调试支持，使用我们编译的 GDB 连接上 QEMU 的调试端口，使用 tmux 分割终端，使 QEMU 执行和 GDB 调试界面一起显示
debug :
	$(TMUX) new -s debug -d "$(QEMU) \
    			-machine virt \
				-s -S \
				-serial mon:stdio \
				-nographic \
				-bios default \
				-device loader,file=$(KERN_IMG),addr=0x80200000" && $(TMUX) split-window -h "$(GDB) -q -x gdbinit"
	$(TMUX) attach-session -t debug

# 使用 riscv64-linux-gnu-nm 命令输出 kernel.bin 中符号的信息到 kernel.sym，先决条件是 $(KERN_BIN)
# -v：按照地址由低到高排序。
symbol : $(KERN_BIN)
	$(NM) -v $(KERN_BIN) > $(KERN_SYM)

# 使用 objdump 命令输出 kernel.bin 的反汇编后的汇编代码，先决条件是 $(KERN_BIN)
# -d：仅反汇编代码段内容
disassembly : $(KERN_BIN)
	$(OBJDUMP) -d $(KERN_BIN) > $(KERN_ASM)

# 清理编译产生的文件。
# 命令前面加上“-”，即使这条命令出错，make 也会继续执行后续命令
clean :
	-rm -f *.o $(KERN_BIN) $(KERN_IMG) $(KERN_SYM) $(KERN_ASM) $(KERN_MAP)
