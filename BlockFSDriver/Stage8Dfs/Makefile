# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE),)

	KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build
	#KERNEL_SOURCE := /home/pradeep/GIT/LKI/bbb-builds/OS/linux-4.19.103
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) clean

# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else

	obj-m := dfs_final.o
	dfs_final-y := real_dfs.o real_dfs_ops.o

endif
