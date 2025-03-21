
CONFIG_MODULE_SIG=n

ifneq ($(KERNELRELEASE),)
	obj-m += capture.o
else
	PWD := $(shell pwd)
	KVER := $(shell uname -r)
	KDIR := /lib/modules/$(KVER)/build
	
	CURRENT_PATH :=$(shell pwd)
	VERSION_NUM :=$(shell uname -r)
	LIUNX_PATH :=/usr/src/linux-headers-$(VERSION_NUM)
default:    
	$(MAKE) -C $(KDIR)  M=$(PWD) modules
all:
	make -C $(KDIR) M=$(PWD) modules 
clean:
	make -C $(LIUNX_PATH) M=$(CURRENT_PATH) clean
endif
