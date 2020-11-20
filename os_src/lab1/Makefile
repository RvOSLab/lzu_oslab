CC := riscv64-linux-gnu-gcc
LD := riscv64-linux-gnu-ld
OBJCOPY := riscv64-linux-gnu-objcopy
OBDDUMP := riscv64-linux-gnu-objdump
AS := riscv64-linux-gnu-as
GDB := riscv64-unknown-elf-gdb
TMUX := tmux
QEMU := qemu-system-riscv64

KERN_IMG := kernel.img
KERN_BIN := kernel.bin
KERN_MAP := kernel.map
KERN_SYM := kernel.sym
KERN_ASM := kernel.asm

CFLAGS := -mcmodel=medany -Wall -g -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I . 

objects := $(patsubst %.c, %.o, $(wildcard *.c)) entry.o

.PHONY : all build run run-gui symbol debug clean disassembly

all: build

build : $(KERN_BIN)

$(KERN_BIN) : $(objects)
	$(LD) -T linker.ld -M=$(KERN_MAP) -o $(KERN_BIN) $^
	$(OBJCOPY) $(KERN_BIN) --strip-all -O binary $(KERN_IMG)

run : build
	@$(QEMU) \
    		-machine virt \
    		-nographic \
    		-bios default \
    		-device loader,file=$(KERN_IMG),addr=0x80200000 

run-gui : build
	@$(QEMU) \
    		-machine virt \
    		-bios default \
    		-device loader,file=$(KERN_IMG),addr=0x80200000 

debug :
	$(TMUX) new -s debug -d "$(QEMU) \
    			-machine virt \
				-s -S \
				-serial mon:stdio \
				-nographic \
				-bios default \
				-device loader,file=$(KERN_IMG),addr=0x80200000" && $(TMUX) split-window -h "$(GDB) -q -x gdbinit"
	$(TMUX) attach-session -t debug


symbol : $(KERN_BIN)
	riscv64-linux-gnu-nm -v $(KERN_BIN) > $(KERN_SYM)

disassembly: $(KERN_BIN)
	riscv64-linux-gnu-objdump -d $(KERN_BIN) > $(KERN_ASM)


clean:
	-rm -f *.o $(KERN_BIN) $(KERN_IMG) $(KERN_SYM) $(KERN_ASM)
