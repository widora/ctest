CC = /home/midas/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
APP = gyro
SOURCES = gyro_l3g4200d.c
DEPENDS = gyro_l3g4200d.h gyro_spi.h filters.h

INCLUDES = -I/home/midas/cc1101_ting_oled
#LDFLAGS  = 
#CFLAGS    = -Wall
LIBS	  = -lm -lpthread

$(APP): $(SOURCES) $(DEPENDS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LIBS) -o $(APP) $(SOURCES)

PHONY: all
all: $(APP)

clean:
	rm -rf $(APP)
