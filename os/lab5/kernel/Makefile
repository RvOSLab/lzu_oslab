include ../tools/toolchain.mk
objects := $(patsubst %.c, %.o, $(wildcard *.c))
CFLAGS := -mcmodel=medany -Wall -g3 -fno-pie -fno-pic -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I../include
ASFLAGS := -g

vpath %.h ../include

.PHONY : clean build
build : libkernel.a

libkernel.a : $(objects) trapentry.o ../lib/libstd.a
	$(AR) vq $@ $^

$(objects) : %.o : %.c
	$(CC) $(CFLAGS) -c $<

../lib/libstd.a:
	make -C ../lib

clean:
	-find . -name '*.o' -exec rm {} \;
	-find . -name '*.a' -exec rm {} \;
