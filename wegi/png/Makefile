
export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

OBJS =  ../egi_fbgeom.o ../egi_bmpjpg.o
OBJS += ../utils/egi_utils.o ../utils/egi_fifo.o ../utils/egi_filo.o
OBJS += ../egi_color.o ../egi_symbol.o
OBJS += ../egi_page.o ../egi_touch.o
OBJS += ../egi.o ../egi_btn.o ../egi_pic.o ../egi_txt.o ../egi_list.o ../egi_slider.o
OBJS += ../egi_log.o ../egi_timer.o  ../egi_math.o
OBJS += ../spi.o ../xpt2046.o

CFLAGS  += -I$(COMMON_USRDIR)/include	-I../. -I../utils/
LDFLAGS += -L$(COMMON_USRDIR)/lib

#LIBS	+= -lubox -lubus -lblobmsg_json -ljson_script -ljson-c -pthread
#LIBS	+= -lcurl -lssl -lcrypto -pthread
LIBS	+= -lpng -lz -lm -pthread -ljpeg

all:	test_png pngtest

pngtest:	pngtest.c
	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) pngtest.c -o pngtest

test_png:	test_png.c
	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) test_png.c -o test_png

clean:
	rm -rf *.o pngtest test_png

