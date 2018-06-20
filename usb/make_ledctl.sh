#!/bin/sh

CROSS_CC="/home/midas/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-uclibc-gcc"
$CROSS_CC -I./ -L./ -lusb-1.0 -o libusb_ledctl libusb_ledctl.c
