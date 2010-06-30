modules:
	$(MAKE) KDIR=$(KERNEL_BUILD_PATH) aufs.ko -f local.mk

modules_install:
	$(MAKE) -C $(KERNEL_BUILD_PATH) M=$(PWD)/fs/aufs modules_install
