BIN_FILE = libmain.so
OBJECTS = main.c record.c key.c oled_iic.c mem_gpio.c
CC = mipsel-openwrt-linux-gcc

$(BIN_FILE) : $(OBJECTS)
	$(CC) -shared -fpic -o $(BIN_FILE) $(OBJECTS) -L /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib -L. -lasound -lm -lmp3lame -lshine -lpthread \
-I /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include 

clean : 
	rm libmain.so *.o
