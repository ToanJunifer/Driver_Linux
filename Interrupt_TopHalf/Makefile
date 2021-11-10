# work with: sudo make -C /usr/src/linux-headers-5.4.0-89-generic M=$('pwd') modules
obj-m	:= interrupt_top_half.o

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD		:= $(shell pwd)
INC		:= -I/usr/src/linux-headers-4.15.0-142-generic/include/

default:
	$(MAKE) -C $(KDIR)	SUBDIRS=$(PWD) modules

install:
	$(MAKE)	-C $(KDIR)	SUBDIRS=$(PWD) module_install

clean:
	$(MAKE) -C $(KDIR)	SUBDIRS=$(PWD) clean