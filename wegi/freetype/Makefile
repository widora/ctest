
export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
#CC = gcc

#OBJ	= ../egi_image.o

CFLAGS  += -I../ -I../utils
CFLAGS  += -I$(COMMON_USRDIR)/include
CFLAGS  += -I$(COMMON_USRDIR)/include/freetype2

LDFLAGS += -L$(COMMON_USRDIR)/lib
LDFLAGS += -L../lib
#libblobmsg_json.so  libjson_script.so  libubox.so
#LIBS   += -lubus -lubox -lblobmsg_json -ljson_script -ljson-c
LIBS    = -lfreetype  -lm -lz -lbz2
LIBS    += -lepages -legi -lesound
LIBS    += -lpng -lz -lm -pthread -ljpeg -lasound
LIBS    += -lcurl -lssl -lcrypto -pthread
LIBS    += -lubox -lubus -lblobmsg_json -ljson_script -ljson-c


all:	 test_freetype get_ascii

test_freetype:	test_freetype.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJ) test_freetype.c -o test_freetype

get_ascii:	get_ascii.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJ) get_ascii.c -o get_ascii


clean:
	rm -rf *.o test_freetype get_ascii


