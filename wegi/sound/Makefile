export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
AR= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-ar

SRC_PATH = /home/midas-zhou/Ctest/wegi

APP = recmp3
#shine_test2
#autorecord4

#OBJS = egi_pcm.o

CFLAGS = -I./ -I$(COMMON_USRDIR)/include -I../
#CFLAGS += -I/home/midas-zhou/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.2/include
CFLAGS += -Wall -O2

LDFLAGS  += -L.
LDFLAGS += -L../lib -L$(COMMON_USRDIR)/lib
#LDFLAGS += -I/home/midas-zhou/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.2/ipkg-install/usr/lib

LIBS	 += -lasound  -lshine -legi -lpng -ljpeg -lz
LIBS += -lcurl -lssl -lcrypto


all:	$(APP) libesound.a

$(APP): $(APP).c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP) $(APP).c

libesound.a: egi_pcm.o
	$(AR) crv $@ egi_pcm.o

egi_pcm.o: egi_pcm.c egi_pcm.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -c egi_pcm.c

install:
	cp -rf libesound.a $(SRC_PATH)/lib
	rm libesound.a

clean:
	rm -rf $(APP) *.o libesound.a

