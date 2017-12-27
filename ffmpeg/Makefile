CC = ./openwrt-uclibc-gcc
APP = ffplay
SOURCE = ffplay.c

INCLUDES += -I/home/midas/ffmpeg-2.6.9/finish/include
CFLAGS   += -Wall
LDFLAGS  += -L/home/midas/ffmpeg-2.6.9/finish/lib
LDFLAGS  += -L/home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/zlib-1.2.8/ipkg-install/usr/lib
LIBS     := -lavutil -lswscale -lavcodec -lavformat -lswresample -lz


$(APP):$(SOURCE)
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP) $(SOURCE)

