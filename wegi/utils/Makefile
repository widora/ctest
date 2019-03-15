
export STAGING_DIR = /home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR = /home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

APP=egi_fifo

OBJS= egi_fifo.o ../egi_log.o ../egi_timer.o ../egi_utils.o

INCLUDES += -I../ -I$(COMMON_USRDIR)/include/json-c
CFLAGS += -Wall
LDFLAGS += -L$(COMMON_USRDIR)/lib
LIBS += -lpthread



egi_fifo.o: egi_fifo.h egi_fifo.c
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -c $(APP).c

test_fifo: test_fifo.c $(OBJS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) test_fifo.c -o test_fifo


clean:
	rm -rf $(APP) $(APP).o test_fifo

#CFLAGS += $(shell pkg-config --cflags json-c)
#LDFLAGS += $(shell pkg-config --libs json-c)

