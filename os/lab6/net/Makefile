include ../tools/toolchain.mk
objects := $(patsubst %.c, %.o, $(wildcard *.c))
CFLAGS := -mcmodel=medany -Wall -g3 -fno-pie -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I../include

vpath %.h ../include

.PHONY : clean build
build : libnet.a

libnet.a : $(objects)
	$(AR) vq $@ $^

$(objects) : %.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	-find . -name '*.o' -exec rm {} \;
	-find . -name '*.a' -exec rm {} \;


