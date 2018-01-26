#----- !!!! NOTE:  comment out 'include <stdint.h>' in sooall_pwm.h  ------

PREFIX="/home/midas/openwrt_widora"
ARCH=mips
KSRC=$(PREFIX)/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29
STAGING_DIR=$(PREFIX)/staging_dir
TOOLCHAIN_DIR=$(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin
CROSS_COMPILE=$(TOOLCHAIN_DIR)/mipsel-openwrt-linux-
PWD=`pwd`

obj-m := sooall_pwm.o

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PWD) modules

clean:
	rm -f *.o
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f *.order
	rm -f *.sysvers
