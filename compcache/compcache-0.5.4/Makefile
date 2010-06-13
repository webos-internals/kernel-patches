KERNEL_BUILD_PATH ?= "/build/webos-14x/beta/linux-2.6.24.7-test"

XVM = sub-projects/allocators/xvmalloc-kmod
EXTRA_CFLAGS	:=	-DCONFIG_BLK_DEV_RAMZSWAP_STATS	\
			-I$(PWD)/$(XVM)			\
			-Wall -mcpu=cortex-a8 -mtune=cortex-a8

obj-m	+=	ramzswap.o

modules:
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD)/$(XVM) modules
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) modules

modules_install:
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD)/$(XVM) modules_install
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) modules_install

clean:
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) clean
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD)/$(XVM) clean
