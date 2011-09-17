ifndef $(KERNEL_BUILD_PATH)
        KERNEL_BUILD_PATH= ${KERNEL_SOURCE}
endif

obj-m		+=	xvmalloc.o

modules:
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) modules

modules_install:
	echo ${KERNEL_BUILD_PATH}
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) modules_install

clean:
	${MAKE} -C $(KERNEL_BUILD_PATH) M=$(PWD) clean
	@$(RM) -rf *.o *~ *.c.gcov *.gcda *.gcno cscope.* tags

xref:
	@cscope -bkqRv
	@ctags -R

