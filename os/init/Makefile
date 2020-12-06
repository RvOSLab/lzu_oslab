include ../tools/toolchain.mk
CFLAGS := -mcmodel=medany -Wall -g -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I../include

vpath %.h ../include
vpath $.a ../lib

.PHONY : clean build

build : lib.a init.o 

init.o : entry.s main.c

clean :
	-(for obj in $(ls *.o); do rm $obj; done)


