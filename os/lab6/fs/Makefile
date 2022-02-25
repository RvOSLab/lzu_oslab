include ../tools/toolchain.mk
objects := $(patsubst %.c, %.o, $(wildcard *.c) $(wildcard **/*.c))
CFLAGS := -mcmodel=medany -Wall -g3 -fno-pie -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I../include

vpath %.h ../include

.PHONY : clean build
build : libfs.a

libfs.a : $(objects) ../mm/libmm.a ../lib/libstd.a
	$(AR) vq $@ $^

../lib/libstd.a:
	make -C ../lib build

../lib/libmm.a:
	make -C ../mm build

clean:
	-find . -name '*.o' -exec rm {} \;
	-find . -name '*.a' -exec rm {} \;
