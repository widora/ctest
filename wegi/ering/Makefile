
export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
#CC = gcc

CFLAGS  += -I$(COMMON_USRDIR)/include
LDFLAGS += -L$(COMMON_USRDIR)/lib

LIBS	+= -lubox -lubus -lblobmsg_json -ljson_script -ljson-c

all:	egi_ring

egi_ring:	egi_ring.c
	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) egi_ring.c -o egi_ring

clean:
	rm -rf *.o egi_ring

