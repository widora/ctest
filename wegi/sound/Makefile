export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

APP = recmp3
#shine_test2
#autorecord4

CFLAGS = -I./ -I$(COMMON_USRDIR)/include
#CFLAGS += -I/home/midas-zhou/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.2/include
CFLAGS += -Wall -O2

LDFLAGS  += -L.
LDFLAGS += -L./lib -L$(COMMON_USRDIR)/lib
#LDFLAGS += -I/home/midas-zhou/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.2/ipkg-install/usr/lib

LIBS	 += -lasound -lm  -lshine

$(APP): $(APP).c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP) $(APP).c

PHONY:all
all: $(APP)

clean:
	rm -rf $(APP)
	
