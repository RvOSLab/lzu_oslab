include ../tools/toolchain.mk
CFLAGS := -mcmodel=medany -Wall -g3 -fno-builtin -fno-stack-protector -fno-strict-aliasing -nostdinc -I../include

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


