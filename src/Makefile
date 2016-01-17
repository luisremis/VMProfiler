EXTRA_CFLAGS +=
APP_EXTRA_FLAGS:= -O2 -ansi -pedantic
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(PWD)
GCC:=gcc
RM:=rm

.PHONY : clean

all: clean modules app

obj-m:= mp3.o

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SUBDIR) modules 
	
app: work.c monitor.c
	$(GCC) -o workapp work.c
	$(GCC) -o monitor monitor.c

clean:
	$(RM) -f monitor workapp *~ *.ko *.o *.mod.c Module.symvers modules.order
