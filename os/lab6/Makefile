# 二进制文件，可直接启动系统
KERN_IMG := kernel.img
# 链接得到的 ELF 格式的可执行文件
override KERN_BIN := kernel.bin
# 导出的内存布局文件
KERN_MAP := kernel.map
# 导出的符号表（存放了变量等符号信息）
KERN_SYM := kernel.sym
# 反汇编得到的汇编程序
KERN_ASM := kernel.S

RISCV_PREFIX := riscv64-linux-gnu
CC := $(RISCV_PREFIX)-gcc
LD := $(RISCV_PREFIX)-ld
AR := $(RISCV_PREFIX)-ar
OBJCOPY := $(RISCV_PREFIX)-objcopy
OBJDUMP := $(RISCV_PREFIX)-objdump
READELF := $(RISCV_PREFIX)-readelf
NM := $(RISCV_PREFIX)-nm
AS := $(RISCV_PREFIX)-as
TMUX := tmux
QEMU := qemu-system-riscv64
GDB := gdb-multiarch
BEAR := bear

override BUILD_DIR := ./build
# SRC_DIRS := ./init ./lib ./kernel ./drivers ./fs ./mm
SRC_DIRS := ./init ./lib ./mm
INC_DIRS := ./include

SRCS := $(shell find $(SRC_DIRS) -name '*.c' -or -name '*.S')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP
CFLAGS := -mcmodel=medany -fno-pie -Wall -g3 -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I include
LDFLAGS := -T tools/linker.ld -Map=$(BUILD_DIR)/$(KERN_MAP)

.PHONY : all run run-gui build symbol debug clean disassembly format compiledb env

all: build symbol disassembly

build: $(BUILD_DIR)/$(KERN_IMG)

symbol: $(BUILD_DIR)/$(KERN_SYM)

disassembly: $(BUILD_DIR)/$(KERN_ASM)

# run，启动 TUI 的 QEMU 运行操作系统，先决条件是 build （见上）
# bios 使用 tools/fw_jump.bin (OpenSBI v0.9) 作为 BIOS。
run : build
	@$(QEMU) \
    		-machine virt \
    		-nographic \
    		-bios tools/fw_jump.bin \
    		-device loader,file=$(BUILD_DIR)/$(KERN_IMG),addr=0x80200000

# run-gui，启动图形形式的 QEMU 运行操作系统，需要 X 服务器，先决条件是 build （见上）
run-gui : build
	@$(QEMU) \
    		-machine virt \
    		-bios tools/fw_jump.bin \
    		-device loader,file=$(BUILD_DIR)/$(KERN_IMG),addr=0x80200000 \
    		-monitor stdio \

# debug，启动 QEMU 调试支持，使用我们编译的 GDB 连接上 QEMU 的调试端口，使用 tmux 分割终端，使 QEMU 执行和 GDB 调试界面一起显示
debug : build
	$(TMUX) new -s debug -d "$(QEMU) \
				-machine virt \
				-s -S \
				-nographic \
				-bios tools/fw_jump.bin \
				-device loader,file=$(BUILD_DIR)/$(KERN_IMG),addr=0x80200000" && $(TMUX) split-window -h "$(GDB) -q -x tools/gdbinit"
	$(TMUX) attach-session -t debug
# 格式化代码
format:
	@find . -regex '.*\.\(c\|h\)' -exec clang-format -i {} \;

compiledb:
	@$(MAKE) clean
	@$(BEAR) -- $(MAKE)

env:
	@echo KERN_IMG: $(KERN_IMG)
	@echo KERN_BIN: $(KERN_BIN)
	@echo KERN_MAP: $(KERN_MAP)
	@echo KERN_SYM: $(KERN_SYM)
	@echo KERN_ASM: $(KERN_ASM)
	@echo RISCV_PREFIX: $(RISCV_PREFIX)
	@echo CC: $(CC)
	@echo LD: $(LD)
	@echo AR: $(AR)
	@echo OBJCOPY: $(OBJCOPY)
	@echo OBJDUMP: $(OBJDUMP)
	@echo READELF: $(READELF)
	@echo NM: $(NM)
	@echo AS: $(AS)
	@echo TMUX: $(TMUX)
	@echo QEMU: $(QEMU)
	@echo GDB: $(GDB)
	@echo BEAR: $(BEAR)
	@echo BUILD_DIR: $(BUILD_DIR)
	@echo SRC_DIRS: $(SRC_DIRS)
	@echo INC_DIRS: $(INC_DIRS)
	@echo SRCS: $(SRCS)
	@echo OBJS: $(OBJS)
	@echo DEPS: $(DEPS)
	@echo INC_FLAGS: $(INC_FLAGS)
	@echo CPPFLAGS: $(CPPFLAGS)
	@echo CFLAGS: $(CFLAGS)
	@echo LDFLAGS: $(LDFLAGS)

clean:
	@rm -r $(BUILD_DIR)


$(BUILD_DIR)/$(KERN_IMG): $(BUILD_DIR)/$(KERN_BIN)
	$(OBJCOPY) $^ --strip-all -O binary $@

# The final build step.
$(BUILD_DIR)/$(KERN_BIN): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build step for .S source
$(BUILD_DIR)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(KERN_SYM): $(BUILD_DIR)/$(KERN_BIN)
	$(READELF) -s $^ > $@

$(BUILD_DIR)/$(KERN_ASM): $(BUILD_DIR)/$(KERN_BIN)
	$(OBJDUMP) -dS $^ > $@

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
