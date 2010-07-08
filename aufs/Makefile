#FIXME: the -s switch in local.mk doesn't seem to be working when called through recursive make
#       so I'm temporarily hardcoding SUBLEVEL and EXTRAVERSION
modules:
	$(MAKE) CONFIG_AUFS_SYSAUFS= KDIR=$(KERNEL_BUILD_PATH) SUBLEVEL=24 EXTRAVERSION=palm aufs.ko -f local.mk

modules_install:
	$(MAKE) -C $(KERNEL_BUILD_PATH) M=$(PWD)/fs/aufs modules_install
