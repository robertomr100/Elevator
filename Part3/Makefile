MODULE_NAME = elevator
PWD := $(shell pwd)

KDIR := /lib/modules/$(shell uname -r)/build

obj-y := wrapper.c

obj-m := init.o # Compile files as module.

$(MODULE_NAME)-objs += init.o

obj-m := $(MODULE_NAME).o

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean







