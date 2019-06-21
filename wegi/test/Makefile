export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr
SRC_PATH=/home/midas-zhou/Ctest/wegi

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

#OBJS = ../ffmpeg/ff_pcm.o

CFLAGS  = -I$(COMMON_USRDIR)/include  -I../. -I../utils/
#-I$(SRC_PATH)/page
CFLAGS  += -Wall -O2
LDFLAGS += -L$(SRC_PATH)/lib -L$(COMMON_USRDIR)/lib

LIBS	= -lepages -legi -lesound
LIBS	+= -lpng -lz -lm -pthread -ljpeg -lasound
LIBS	+= -lcurl -lssl -lcrypto -pthread
LIBS	+= -lubox -lubus -lblobmsg_json -ljson_script -ljson-c

APPS = test_fb test_sym tmp_app test_pic

all:	$(APPS)

tmp:	tmp_app.c
	$(CC) -o tmp_app tmp_app.c $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS)

### !!! NOTE: put '-o $@  $@.c' ahead of FLAGS and LIBS !!!!
%:%.c
	$(CC) -o $@  $@.c $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS)
#	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS)  $@.c -o $@

clean:
	rm -rf *.o $(APPS)

