include ../tools/toolchain.mk
objects := $(patsubst %.c, %.o, $(wildcard *.c))
CFLAGS := -mcmodel=medany -Wall -g3 -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I../include

vpath %.h ../include

.PHONY : clean build
build : libmm.a

libmm.a : $(objects) ../lib/libstd.a
	make -C ../lib build
	$(AR) vq $@ $^

$(objects) : %.o : %.c
	$(CC) $(CFLAGS) -c $<

../lib/libstd.a :
	make -C ../lib build
	 
clean:
	-find . -name '*.o' -exec rm {} \;
	-find . -name '*.a' -exec rm {} \;


