PREFIX="/home/midas-zhou/openwrt_widora"
ARCH=mips
KSRC=$(PREFIX)/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29
STAGING_DIR=$(PREFIX)/staging_dir
export STAGING_DIR
TOOLCHAIN_DIR=$(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin
CROSS_COMPILE=$(TOOLCHAIN_DIR)/mipsel-openwrt-linux-
PWD=`pwd`

obj-m := hello.o

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PWD) modules
	$(CROSS_COMPILE)gcc user_hello.c -o user_hello

clean:
	rm -f user_hello
	rm -rf *.o *.ko *.o.cmd *.mod.c *.mod.o *.order *.symvers
	rm -rf .*
