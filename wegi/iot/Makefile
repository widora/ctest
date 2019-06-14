
export STAGING_DIR = /home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR = /home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc

APP=egi_iotclient

INCLUDES += -I../ -I../utils -I$(COMMON_USRDIR)/include/json-c
CFLAGS += -Wall
CFLAGS += -D_GNU_SOURCE         ## for O_CLOEXEC flag ##
LDFLAGS += -L$(COMMON_USRDIR)/lib
LIBS += -ljson-c -lpthread


$(APP):	$(APP).c
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) -c $(APP).c

clean:
	rm -rf $(APP) $(APP).o

#CFLAGS += $(shell pkg-config --cflags json-c)
#LDFLAGS += $(shell pkg-config --libs json-c)

