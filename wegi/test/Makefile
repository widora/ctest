export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr
SRC_PATH=/home/midas-zhou/Ctest/wegi

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

# !!!--- to be created and cleaned ---!!!
OBJ_IOT = ../iot/egi_iotclient.o

CFLAGS  = -I$(COMMON_USRDIR)/include  -I../. -I../utils  -I../iot
CFLAGS += -I$(COMMON_USRDIR)/include/freetype2
CFLAGS += -I$(COMMON_USRDIR)/include/json-c

#-I$(SRC_PATH)/page
CFLAGS  += -Wall -O2
CFLAGS  += -D_GNU_SOURCE

LDFLAGS += -L$(SRC_PATH)/lib -L$(COMMON_USRDIR)/lib

LIBS	=  -lepages -lesound
LIBS	+= -lpng -lz -lm -pthread -ljpeg -lasound
LIBS	+= -lcurl -lssl -lcrypto -pthread
LIBS	+= -lubox -lubus -lblobmsg_json -ljson_script -ljson-c
LIBS    += -lfreetype  -lm -lz -lbz2

APPS = test_fb test_sym tmp_app test_pic  test_bigiot test_math

all:	$(APPS)


#### --- use dynamic and static libs in compilatoin!!! ---
# 1. Exclude dynamic -legi first.
# 2. Use -Wl,Bdynamic and -Wl,Bstatic to switch to dynamic/static libs
# 3. At last, we must switch back to dynamic libs, as default for Linux system runtime setup.
# 4. Source/obj files must be put before all -Wl.

tmp_app:	tmp_app.c
	$(CC) -o tmp_app tmp_app.c $(CFLAGS) $(LDFLAGS) $(LIBS) -legi  #--use shared egilib
#	$(CC) tmp_app.c -o tmp_app $(OBJ_IOT) $(CFLAGS) $(LDFLAGS) -Wl,-Bdynamic $(LIBS) \
#-Wl,-Bstatic -legi -Wl,-Bdynamic
#---use static egilib

test_bigiot:	test_bigiot.c  $(OBJ_IOT)
	$(CC) test_bigiot.c -o test_bigiot $(OBJ_IOT) $(CFLAGS) $(LDFLAGS) -Wl,-Bdynamic $(LIBS) \
-Wl,-Bstatic -legi -Wl,-Bdynamic


test_math:	test_math.c  ../egi_math.h
#	$(CC) -o test_math test_math.c $(CFLAGS) $(LDFLAGS) $(LIBS) -legi  #--use shared egilib
	$(CC) test_math.c -o test_math $(CFLAGS) $(LDFLAGS) -Wl,-Bdynamic $(LIBS) \
-Wl,-Bstatic -legi -Wl,-Bdynamic
#---use static egilib


### !!! NOTE: put '-o $@  $@.c' ahead of FLAGS and LIBS !!!!
%:%.c
	$(CC) -o $@  $@.c $(CFLAGS) $(LDFLAGS) $(LIBS) -legi
#	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS)  $@.c -o $@

clean:
	rm -rf *.o $(APPS) $(OBJ_IOT)

