#!/bin/sh
BIN_FILE=autorecord3
STR_FILE=$BIN_FILE'.c'
../openwrt-gcc -o $BIN_FILE $STR_FILE \
-I /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include/ \
-L /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib \
-L. \
-lasound -lm -lmp3lame
