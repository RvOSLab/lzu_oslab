include ../tools/toolchain.mk
objects := $(patsubst %.c, %.o, $(wildcard *.c)) trapentry.o
CFLAGS := -mcmodel=medany -Wall -g -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I ../include

vpath %.h ../include

.PHONY : clean build
build : $(objects)

$(objects) : %.o : %.c
	$(CC) $(CFLAGS) -c $<

../lib/libstd.a :
	make -C ../lib build
	 
clean:
	-find . -name '*.o' -exec rm {} \;
	-find . -name '*.a' -exec rm {} \;
