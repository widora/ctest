CC = ./openwrt-uclibc-gcc
APP = ffplay2
SOURCE = ffplay2.c

INCLUDES += -I/home/midas/ctest
INCLUDES += -I/home/midas/ffmpeg-2.6.9/finish/include

CFLAGS   += -Wall

LDFLAGS  += -L/home/midas/ffmpeg-2.6.9/finish/lib
LDFLAGS  += -L/home/midas/ctest

#------ -lz seems no use  ------------
LIBS     := -lavutil -lswscale -lavcodec -lavformat -lswresample
LIBS     += -lftdi1 -lusb-1.0

$(APP):$(SOURCE)
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP) $(SOURCE)

clean:
	rm -rf $(APP)
