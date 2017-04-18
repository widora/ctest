BIN_FILE = main
OBJECTS = main.o record.o key.o oled_iic.o mem_gpio.o cJSON.o
CC = mipsel-openwrt-linux-gcc

$(BIN_FILE) : $(OBJECTS)
	$(CC) -o $(BIN_FILE) $(OBJECTS) -L /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib -L. -lasound -lm -lmp3lame -lshine -lpthread

mem_gpio.o : mem_gpio.c
	$(CC) -c mem_gpio.c

key.o : key.c
	$(CC) -c key.c

oled_iic.o : oled_iic.c
	$(CC) -c oled_iic.c

record.o : record.c
	$(CC) -c record.c  -I /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include \
-L /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib -L. -lasound -lm -lmp3lame -lshine

cJSON.o : cJSON.c
	$(CC) -c cJSON.c

main.o : main.c
	$(CC) -c main.c -lpthread

clean : 
	rm main *.o
