#!/bin/sh
BIN_FILE=autorecord4
STR_FILE=$BIN_FILE'.c'
/home/zyc/Documents/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc -o $BIN_FILE $STR_FILE \
-I /home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include/ \
-L /home/midas/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib \
-L. \
-lasound -lm -lmp3lame -lshine
