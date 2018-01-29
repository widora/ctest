#----  NOTE:
#----- 1. DEPENDS and INCLUDES are for two separate process, you must present directory for both if necessary.
#----- 2. Only dependent files will be checked for update.
#----- 3. Adding keyword 'static' for some functions may pass compiling, but cause running error! be careful.

CC = /home/midas/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
APP = gyro
SOURCES = gyro_test.c
DEPENDS = gyro_l3g4200d.h gyro_spi.h filters.h mathwork.h i2c_oled_128x64.h
OBJS =i2c_oled_128x64.o gyro_l3g4200d.o filters.o mathwork.o gyro_spi.o

INCLUDES = -I/home/midas/ctest
#LDFLAGS  = 
#CFLAGS    = -Wall
LIBS	  = -lm -lpthread

$(APP) : gyro_test.c $(OBJS) filters.h mathwork.h
	$(CC) -o $(APP) $(CFLAGS) -lpthread $(INCLUDES) gyro_test.c $(OBJS)

filters.o : filters.c mathwork.h mathwork.o
	$(CC) $(CFLAGS) -lm mathwork.o -c filters.c 

mathwork.o : mathwork.c mathwork.h
	$(CC) $(CFLAGS) -lm -c mathwork.c  
	
gyro_l3g4200d.o : gyro_l3g4200d.c i2c_oled_128x64.o gyro_spi.o
	$(CC) $(INCLUDES) $(CFLAGS) -c gyro_l3g4200d.c 

i2c_oled_128x64.o : i2c_oled_128x64.c i2c_oled_128x64.h /home/midas/ctest/ascii2.h
	$(CC) $(INCLUDES) $(CFLAGS) -c i2c_oled_128x64.c 

gyro_spi.o : gyro_spi.c
	$(CC) $(CFLAGS) -c gyro_spi.c

PHONY: all
all: $(APP)

clean:
	rm -rf $(APP)
	rm -rf *.o
