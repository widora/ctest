PREFIX="/home/midas-zhou/openwrt_widora"
ARCH=mips
KSRC=$(PREFIX)/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29
STAGING_DIR=$(PREFIX)/staging_dir
export STAGING_DIR
TOOLCHAIN_DIR=$(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin
CROSS_COMPILE=$(TOOLCHAIN_DIR)/mipsel-openwrt-linux-
PWD=`pwd`

obj-m := lirc.o

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PWD) modules
	$(CROSS_COMPILE)gcc lirc_app.c -o lirc_app
	$(CROSS_COMPILE)gcc lirc_mplay.c -o lirc_mplay

clean:
	rm -f lirc_app lirc_mplay
	rm -f *.o *.ko *.mod.c *.mod.o *.order *.symvers
