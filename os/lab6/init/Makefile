include ../tools/toolchain.mk
# -fno-pie 禁止生成位置无关代码，可以简单地理解为：禁止 GCC 生成使用 auipc/ld 指令获取符号地址的代码
CFLAGS := -mcmodel=medany -fno-pie -Wno-main -Wall -g3 -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I../include

vpath %.h ../include
vpath $.a ../lib

.PHONY : clean build

build : entry.o main.o

entry.o : entry.s 
	$(CC) $(CFLAGS) -c entry.s
main.o : main.c
	$(CC) $(CFLAGS) -c main.c

clean :
	-find . -regex '.*\.o' -exec rm {} \;


