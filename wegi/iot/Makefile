
export STAGING_DIR = /home/midas-zhou/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/
COMMON_USRDIR = /home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/

CC=$(STAGING_DIR)mipsel-openwrt-linux-gcc


APP=egi_iotclient

INCLUDES += -I$(COMMON_USRDIR)/include/json-c
CFLAGS += -Wall
LDFLAGS += -L$(COMMON_USRDIR)/lib
LIBS += -ljson-c -lpthread


$(APP):	$(APP).c
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS) $(APP).c -o $(APP)

#CFLAGS += $(shell pkg-config --cflags json-c)
#LDFLAGS += $(shell pkg-config --libs json-c)

