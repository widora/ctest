CC = /home/midas/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
APP = adxl345
SOURCES = adxl_test.c
DEPENDS = i2c_adxl345.h  ../L3G4200D/data_server.h ../L3G4200D/mathwork.h  ../L3G4200D/filters.h
OBJS = ../L3G4200D/mathwork.o ../L3G4200D/filters.o i2c_adxl345.o

INCLUDES = -I/home/midas/ctest/L3G4200D
#LDFLAGS  = 
#CFLAGS    = -Wall
LIBS	  = -lm 

$(APP): $(SOURCES) $(DEPENDS) $(OBJS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LIBS) -o $(APP) $(SOURCES) $(OBJS)

i2c_adxl345.o : i2c_adxl345.c i2c_adxl345.h
	$(CC) $(INCLUDES) $(CFLAGS) $(LIBS) -c i2c_adxl345.c

PHONY: all
all: $(APP)

clean:
	rm -rf $(APP)
	rm -rf i2c_adxl345.o
