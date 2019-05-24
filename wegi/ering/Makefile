
export STAGING_DIR=/home/midas-zhou/openwrt_widora/staging_dir
COMMON_USRDIR=/home/midas-zhou/openwrt_widora/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr

CC= $(STAGING_DIR)/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
#CC = gcc

CFLAGS  += -I$(COMMON_USRDIR)/include
LDFLAGS += -L$(COMMON_USRDIR)/lib

LIBS	+= -lubox -lubus -lblobmsg_json -ljson_script -ljson-c

all:	ering_server ering_client

ering_server:	ering_server.c
	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) ering_server.c -o ering_server

ering_client:	ering_client.c
	$(CC)  $(CFLAGS) $(LDFLAGS) $(LIBS) ering_client.c -o ering_client

clean:
	rm -rf *.o ering_server ering_client

