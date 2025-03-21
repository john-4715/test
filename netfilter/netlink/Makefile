
CONFIG_MODULE_SIG=n

ifeq ($(KERNELRELEASE), )
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD :=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD)  
clean:
	rm -rf *.mk .tmp_versions Module.symvers *.mod.c *.o *.ko .*.cmd Module.markers modules.order *.a *.mod
load:
	insmod netlink.ko
unload:
	rmmod netlink
install: default
	rmmod netlink || true
	insmod netlink.ko || true
	mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial/ || true
	cp -f ./netlink.ko /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial/ || true
	depmod -a
uninstall:
	rmmod netlink || true
	rm -rf /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial/netlink.ko || true
	depmod -a
else
	obj-m := netlink.o
	netlink-objs := main.o utils.o
endif