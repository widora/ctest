#VPATH ../:../iot:../utils

export STAGING_DIR = /home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR = /home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

APP=egi_fifo

OBJS= egi_utils.o egi_fifo.o egi_iwinfo.o ../egi_log.o ../egi_timer.o

## Shall also include all sys libs head file dir
CFLAGS += -Wall -I../ -I../utils -I$(COMMON_USRDIR)/include
LDFLAGS += -L$(COMMON_USRDIR)/lib
LIBS += -lpthread



test_fifo: test_fifo.c $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) test_fifo.c -o test_fifo

egi_fifo.o: egi_fifo.h egi_fifo.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -c egi_fifo.c

egi_utils.o: egi_utils.h egi_utils.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -c egi_utils.c

egi_iwinfo.o: egi_iwinfo.h egi_iwinfo.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -c egi_iwinfo.c


.PHONY:	clean
clean:
	rm -rf $(APP) $(APP).o test_fifo egi_utils.o egi_fifo.o egi_iwinfo.o

#CFLAGS += $(shell pkg-config --cflags json-c)
#LDFLAGS += $(shell pkg-config --libs json-c)

