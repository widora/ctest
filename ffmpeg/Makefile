CC = ./openwrt-uclibc-gcc
APP = ffplay2
SOURCE = $(APP).c

APP2 = alsa_play
SOURCE2 = $(APP2).c

INCLUDES += -I/home/midas/ctest
INCLUDES += -I/home/midas/ffmpeg-2.6.9/finish/include
INCLUDES += -I/home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include

CFLAGS   += -Wall

LDFLAGS  += -L/home/midas/ffmpeg-2.6.9/finish/lib
LDFLAGS  += -L/home/midas/ctest
LDFLAGS  += -L/home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib

#------ -lz seems no use  ------------
LIBS     := -lavutil -lswscale -lavcodec -lavformat -lswresample
LIBS     += -lftdi1 -lusb-1.0
LIBS	 += -lasound


$(APP):$(SOURCE)
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP) $(SOURCE)

$(APP2):$(SOURCE2)
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP2) $(SOURCE2)

PHONY:all
all:$(APP) $(APP2)

clean:
	rm -rf $(APP) $(APP2)
	
