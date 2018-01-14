CC = /home/midas/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
APP = adxl345
SOURCES = i2c_adxl345.c
DEPENDS = i2c_adxl345_2.h  ../L3G4200D/mathwork.h

INCLUDES = -I/home/midas/ctest/L3G4200D
#LDFLAGS  = 
#CFLAGS    = -Wall
#LIBS	  = -lm -lpthread

$(APP): $(SOURCES) $(DEPENDS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LIBS) -o $(APP) $(SOURCES)

PHONY: all
all: $(APP)

clean:
	rm -rf $(APP)
