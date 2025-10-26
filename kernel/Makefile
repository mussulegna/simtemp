
#obj-m += nxp_simtemp.o

# Kernel libraries
KDIR = /lib/modules/$(shell uname -r)/build

# Make targets
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
