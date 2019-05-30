
export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr

SRC_PATH=/home/midas-zhou/Ctest/wegi

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

SRC_FILES = $(wildcard *.c)
APPS = $(patsubst %.c, %, $(SRC_FILES))
#### ingore some apps
APPS := $(APPS:tmp_app=)
#APPS := $(APPS:test_.o=)
#APPS := $(APPS:tmp_app.o=)



OBJS += ../utils/egi_utils.o ../utils/egi_fifo.o ../utils/egi_filo.o

CFLAGS  += -I$(COMMON_USRDIR)/include	-I../. -I../utils/ -I../page
LDFLAGS += -L$(SRC_PATH)/lib -L$(COMMON_USRDIR)/lib

LIBS	= -legi
LIBS	+= -lpng -lz -lm -pthread -ljpeg
#LIBS	+= -lubox -lubus -lblobmsg_json -ljson_script -ljson-c -pthread
#LIBS	+= -lcurl -lssl -lcrypto -pthread



all:	$(APPS)

#test_geom:	test_geom.c
#	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) test_geom.c -o test_geom

%:%.c
	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) -o $@ $@.c

clean:
	rm -rf *.o test_geom

