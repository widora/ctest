#ifneq ($(KERNELRELEASE),)	
obj-m :=hello.o
#else	
KDIR :=/home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29/include
PWD :=$(shell pwd)
all:
	$(MAKE) -C $(KDIR) SUBDIR=$(PWD) modules
clean:
	rm -rf *.ko *.o *.mod.* *.symvers *.order

#endif
