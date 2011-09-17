ifndef $(KERNEL_BUILD_PATH)
        KERNEL_BUILD_PATH= ${KERNEL_SOURCE}
endif

XVM = sub-projects/allocators/xvmalloc-kmod
EXTRA_CFLAGS	:=	-DCONFIG_BLK_DEV_RAMZSWAP_STATS	\
			-I$(PWD)/$(XVM)			\
			-Wall

obj-m	+= ramzswap.o

modules:
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD)/$(XVM) modules
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) modules

modules_install:
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD)/$(XVM) modules_install
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) modules_install

clean:
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) clean
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD)/$(XVM) clean
