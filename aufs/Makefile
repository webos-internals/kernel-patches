modules:
	$(MAKE) KDIR=${KERNEL_BUILD_PATH} CONFIG_AUFS_SYSAUFS= -f local.mk

modules_install:
	$(MAKE) -C ${KERNEL_BUILD_PATH} M=$(PWD)/fs/aufs modules_install
