export STAGING_DIR = /home/midas-zhou/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin
CC = $(STAGING_DIR)/mipsel-openwrt-linux-gcc
APP = ffplay5
SOURCE = $(APP).c
DEPS = ffplay.h
OBJ = $(APP).o play_ffpcm.o
EGI_OBJ = ../egi_timer.o ../egi.o ../egi_math.o ../egi_symbol.o	../egi_pic.o\
		../egi_txt.o ../egi_btn.o ../egi_list.o ../egi_bmpjpg.o ../egi_fbgeom.o ../spi.o

APP2 = alsa_play
SOURCE2 = $(APP2).c

#INCLUDES += -I/home/midas-zhou/Ctest
INCLUDES += -I/home/midas-zhou/Ctest/wegi
INCLUDES += -I/home/midas-zhou/Ctest/wegi/include
INCLUDES += -I/home/midas-zhou/ffmpeg-2.6.9/finish/include
INCLUDES += -I/home/midas-zhou/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include

CFLAGS   += -Wall

LDFLAGS  += -L/home/midas-zhou/ffmpeg-2.6.9/finish/lib
LDFLAGS  += -L/home/midas-zhou/Ctest/wegi/lib
LDFLAGS  += -L/home/midas-zhou/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib

#------ -lz seems no use  ------------
LIBS     += -lavutil -lswscale -lavcodec -lavformat -lswresample
#LIBS     += -lftdi1 -lusb-1.0
LIBS	 += -lm -lasound -ljpeg
LIBS	 += -lpthread

#----------------  APP  --------------------
$(APP): $(OBJ) $(DEPS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP) $(OBJ) $(EGI_OBJ)
$(APP).o: $(SOURCE)
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -c $(SOURCE)
play_ffpcm.o: play_ffpcm.c
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -c play_ffpcm.c

#---------------  APP2 -----------------------
$(APP2):$(SOURCE2)
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP2) $(SOURCE2)



PHONY:all
all:$(APP) $(APP2)

clean:
	rm -rf $(APP) $(APP2) *.o

