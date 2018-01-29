#----  NOTE:
#----- 1. DEPENDS and INCLUDES are for two separate process, you must present directory for both if necessary.
#----- 2. Only dependent files will be checked for update.
#----- 3. Adding keyword 'static' for some functions may pass compiling, but cause running error! be careful.
#----- 4. Errors may arise if dependent obj files found not updated.

CC = /home/midas/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
APP = wbiroll
SOURCES = wbiroll.c
DEPENDS = ../ADXL345/i2c_adxl345.h \
	  ../L3G4200D/gyro_l3g4200d.h \
	  ../L3G4200D/gyro_spi.h \
	  ../L3G4200D/filters.h \
	  ../L3G4200D/mathwork.h \
	  ../L3G4200D/i2c_oled_128x64.h

OBJS =   ../ADXL345/i2c_adxl345.o \
	 ../L3G4200D/i2c_oled_128x64.o \
	 ../L3G4200D/gyro_l3g4200d.o \
	 ../L3G4200D/mathwork.o \
	 ../L3G4200D/filters.o \
	 ../L3G4200D/gyro_spi.o

INCLUDES = -I/home/midas/ctest
INCLUDES += -I/home/midas/ctest/ADXL345
INCLUDES += -I/home/midas/ctest/L3G4200D
#LDFLAGS  = 
#CFLAGS    = -Wall
LIBS	  = -lm -lpthread

$(APP) :  $(DEPENDS) $(OBJS) wbiroll.c
	$(CC) wbiroll.c -o $(APP) $(CFLAGS) $(LIBS) $(INCLUDES) $(OBJS) 



PHONY: all
all: $(APP)

clean:
	rm -rf $(APP)
	rm -rf *.o
