KERNEL_BUILD_PATH ?= "/build/webos-14x/beta/linux-2.6.24.7-test"

XVM = sub-projects/allocators/xvmalloc-kmod
EXTRA_CFLAGS	:=	-DCONFIG_BLK_DEV_RAMZSWAP_STATS	\
			-I$(PWD)/$(XVM)			\
			-Wall -mcpu=cortex-a8 -mtune=cortex-a8

obj-m	+=	$(XVM)/xvmalloc.o \
		ramzswap.o

all:
	make -C $(KERNEL_BUILD_PATH) M=$(PWD)/$(XVM) modules
	make -C $(KERNEL_BUILD_PATH) M=$(PWD) modules
	@ln -sf $(XVM)/xvmalloc.ko

clean:
	make -C $(KERNEL_BUILD_PATH) M=$(PWD) clean
	make -C $(KERNEL_BUILD_PATH) M=$(PWD)/$(XVM) clean
	@rm -rf *.ko
