# work with: sudo make -C /usr/src/linux-headers-5.11.0-40-generic M=$('pwd') modules
obj-m	:= workQueueStatic.o

KDIR	:= /usr/src/linux-headers-$(shell uname -r)
PWD		:= $(shell 'pwd')

default:
	$(MAKE) -C $(KDIR)	M=$(PWD) modules

install:
	$(MAKE)	-C $(KDIR)	M=$(PWD) module_install

clean:
	$(MAKE) -C $(KDIR)	M=$(PWD) clean