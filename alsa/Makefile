SOURCE = record_control.c
OBJECTS = record_control.o
OUTFILE = record_control
CC = mipsel-openwrt-linux-gcc
#LIBS = lasound
INC = /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include/*.h
LIBS = /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib

$(OUTFILE) : $(OBJECTS) 
	$(CC) -L/$(LIBS) -o $(OUTFILE) $(OBJECTS)

$(OBJECTS) : $(SOURCE) $(INC)
	$(CC) -c $(OBJECTS) $(SOURCE)

.PHONY : clean
clean:
	rm $(OBJECTS) $(OUTFILE)