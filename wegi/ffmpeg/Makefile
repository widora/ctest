export STAGING_DIR = /home/midas-zhou/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin
CC = $(STAGING_DIR)/mipsel-openwrt-linux-gcc

COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr

APP = ffplay6
#APP = filtering_video
SOURCE = $(APP).c
DEPS = ff_utils.h ff_utils.c ff_pcm.h ff_pcm.c
OBJ = $(APP).o ff_pcm.o ff_utils.o
EGI_OBJ = ../egi_timer.o ../egi.o ../egi_math.o ../egi_symbol.o	../egi_pic.o ../egi_log.o ../utils/egi_utils.o\
	  ../utils/egi_filo.o	../egi_txt.o ../egi_btn.o ../egi_list.o ../egi_bmpjpg.o ../egi_fbgeom.o ../spi.o \
	  ../egi_slider.o

APP2 = alsa_play
SOURCE2 = $(APP2).c

#INCLUDES += -I/home/midas-zhou/Ctest
#INCLUDES += -I/home/midas-zhou/Ctest/wegi
#INCLUDES += -I/home/midas-zhou/Ctest/wegi/include
INCLUDES += -I/home/midas-zhou/ffmpeg-2.8.15/finish/include
INCLUDES += -I/home/midas-zhou/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include

### for -Wall for __attribute__
CFLAGS   += -Wall

CFLAGS	 += -I/home/midas-zhou/Ctest/wegi/utils
CFLAGS   += -I/home/midas-zhou/Ctest/wegi
CFLAGS   += -I$(COMMON_USRDIR)/include

LDFLAGS  += -L/home/midas-zhou/ffmpeg-2.8.15/finish/lib
LDFLAGS  += -L/home/midas-zhou/Ctest/wegi/lib
LDFLAGS  += -L/home/midas-zhou/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib
LDFLAGS  += -L$(COMMON_USRDIR)/lib

#------ -lz seems no use  ------------
LIBS     += -lavutil -lswscale -lavcodec -lavformat -lswresample -lavfilter -lpostproc
#LIBS     += -lftdi1 -lusb-1.0
LIBS	 += -lm -lasound -ljpeg
LIBS	 += -lpthread

#----------------  APP  --------------------
$(APP): $(OBJ) $(DEPS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJ) $(EGI_OBJ) -o $(APP)

$(APP).o: $(SOURCE) $(DEPS)
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -c $(SOURCE)

ff_pcm.o: ff_pcm.c ff_pcm.h
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -c ff_pcm.c

ff_utils.o: ff_utils.c ff_utils.h
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -c ff_utils.c


#---------------  APP2 -----------------------
$(APP2):$(SOURCE2) $(DEPS)
	$(CC)  $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(APP2) $(SOURCE2)



PHONY:all
all:$(APP) $(APP2)

clean:
	rm -rf $(APP) $(APP2) *.o

