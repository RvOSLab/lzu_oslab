include ../tools/toolchain.mk
objects := $(patsubst %.c, %.o, $(wildcard *.c))
CFLAGS := -mcmodel=medany -Wall -g -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I../include

vpath %.h ../include

.PHONY : clean build
build : lib.a

lib.a : $(objects)
	$(AR) vq lib.a $(objects)

$(objects) : %.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	-rm *.o
	-(if [ -e lib.a ]; then rm lib.a; fi)


