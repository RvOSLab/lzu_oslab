include tools/toolchain.mk
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
# -g3 添加适用于gdb的调试信息
# -fno-builtin 不使用 GCC 内建函数
# -fno-stack-protector 不使用栈保护
# -fno-strict-aliasing 允许未定义的类型间强制转换
# -nostdinc 不使用系统标准库
# -I include 优先在include目录搜索头文件
CFLAGS := -mcmodel=medany -fno-pie -Wall -g3 -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I include

# .PHONY表示后面这些都是伪造的target，无论同名文件是否存在都会运行
.PHONY : all build run run-gui symbol debug clean disassembly format

# make 不加参数默认选取第一个 target 运行，此处 all 就是第一个 target，要想生成 target 需要先决条件 build（见下）
all : build

build :
	make -C init build
	make -C lib build
	make -C mm build
	make -C kernel build
	make -C drivers build
	make -C fs build
	make -C net build
	$(LD) -T tools/linker.ld -Map=$(KERN_MAP) -o $(KERN_BIN) init/entry.o init/main.o  kernel/libkernel.a fs/libfs.a net/libnet.a drivers/libdrivers.a mm/libmm.a lib/libstd.a
	$(OBJCOPY) $(KERN_BIN) --strip-all -O binary $(KERN_IMG)
# run，启动 TUI 的 QEMU 运行操作系统，先决条件是 build （见上）
# bios 使用 tools/fw_jump.bin (OpenSBI v0.9) 作为 BIOS。
run : build
	@$(QEMU) \
    		-machine virt \
    		-nographic \
    		-bios tools/fw_jump.bin \
    		-device loader,file=$(KERN_IMG),addr=0x80200000 \
			-global virtio-mmio.force-legacy=off \
			-drive if=none,format=raw,file=kernel.img,id=hd0 \
			-device virtio-blk-device,drive=hd0,bus=virtio-mmio-bus.1 \
			-netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
			-device virtio-net-device,netdev=net0,bus=virtio-mmio-bus.2

# run-gui，启动图形形式的 QEMU 运行操作系统，需要 X 服务器，先决条件是 build （见上）
run-gui : build
	@$(QEMU) \
    		-machine virt \
    		-bios tools/fw_jump.bin \
    		-device loader,file=$(KERN_IMG),addr=0x80200000 \
    		-monitor stdio \

# debug，启动 QEMU 调试支持，使用我们编译的 GDB 连接上 QEMU 的调试端口，使用 tmux 分割终端，使 QEMU 执行和 GDB 调试界面一起显示
debug : build
	$(TMUX) new -s debug -d "$(QEMU) \
				-machine virt \
				-s -S \
				-nographic \
				-bios tools/fw_jump.bin \
				-device loader,file=$(KERN_IMG),addr=0x80200000" && $(TMUX) split-window -h "$(GDB) -q -x tools/gdbinit"
	$(TMUX) attach-session -t debug

# 使用 riscv64-linux-gnu-nm 命令输出 kernel.bin 中符号的信息到 kernel.sym，先决条件是 $(KERN_BIN)
# -v：按照地址由低到高排序。
symbol : $(KERN_BIN)
	$(READELF) -s $(KERN_BIN) > $(KERN_SYM)

# 使用 objdump 命令输出 kernel.bin 的反汇编后的汇编代码，先决条件是 $(KERN_BIN)
# -d：仅反汇编代码段内容
disassembly : $(KERN_BIN)
	$(OBJDUMP) -d $(KERN_BIN) > $(KERN_ASM)

# 格式化代码
format:
	@find . -regex '.*\.\(c\|h\)' -exec clang-format -i {} \;
# 清理编译产生的文件。
# 命令前面加上“-”，即使这条命令出错，make 也会继续执行后续命令
clean :
	-make -C init/ clean
	-make -C lib/ clean
	-make -C kernel/ clean
	-make -C mm/ clean
	-make -C drivers/ clean
	-make -C fs/ clean
	-make -C net/ clean
	-rm -f $(KERN_BIN) $(KERN_IMG) $(KERN_SYM) $(KERN_ASM) $(KERN_MAP)
