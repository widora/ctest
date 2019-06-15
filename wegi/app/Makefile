export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr
SRC_PATH=/home/midas-zhou/Ctest/wegi

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

CFLAGS  = -I$(COMMON_USRDIR)/include  -I../. -I../utils/ -I../page/
#-I$(SRC_PATH)/page
CFLAGS  += -Wall -O2
LDFLAGS += -L$(SRC_PATH)/lib -L$(COMMON_USRDIR)/lib

LIBS	= -lepages -legi -lesound
LIBS	+= -lpng -lz -lm -pthread -ljpeg -lasound
LIBS	+= -lcurl -lssl -lcrypto -pthread
LIBS	+= -lubox -lubus -lblobmsg_json -ljson_script -ljson-c

OBJS = egi_pageffplay.o
APPS = app_ffplay


all:	$(APPS)
### !!! NOTE: put '-o $@  $@.c' ahead of FLAGS and LIBS in LD !!!!

app_ffplay: 	app_ffplay.c $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) -o $@ $@.c

egi_pageffplay.o:	egi_pageffplay.c  egi_pageffplay.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS)  -c egi_pageffplay.c -o egi_pageffplay.o

clean:
	rm -rf *.o $(APPS) *.dep

