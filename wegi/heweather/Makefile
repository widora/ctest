
export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

#OBJS = ../egi_log.o ../utils/egi_utils.o ../egi_timer.o

CFLAGS  += -I$(COMMON_USRDIR)/include	-I../. -I../utils/
CFLAGS  += -I$(COMMON_USRDIR)/include/freetype2
CFLAGS  += -D_GNU_SOURCE	# for strdupa

LDFLAGS += -L$(COMMON_USRDIR)/lib  -L../lib/

#LIBS	+= -lubox -lubus -lblobmsg_json -ljson_script -ljson-c -pthread
LIBS   =  -legi -ljpeg -lpng -lz
LIBS  +=  -lcurl -lssl -lcrypto -pthread -ljson-c
LIBS  +=  -lfreetype  -lm -lz -lbz2

all:	test_weather he_weather.o

#https:	https.c
#	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) https.c -o https

test_weather:	test_weather.c  he_weather.o
	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS)  he_weather.o test_weather.c -o test_weather


he_weather.o:  he_weather.c he_weather.h
	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS)  -c he_weather.c


clean:
	rm -rf *.o  test_weather

