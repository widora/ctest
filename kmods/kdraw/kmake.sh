#!/bin/sh
PREFIX="/home/midas/openwrt_widora"
ARCH=mips
KSRC="$PREFIX/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29"
STAGING_DIR="$PREFIX/staging_dir"
TOOLCHAIN_DIR="$STAGING_DIR/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin"
CROSS_COMPILE="$TOOLCHAIN_DIR/mipsel-openwrt-linux-"
PWD=`pwd`
#export STAGING_DIR=$STAGING_DIR
#export PATH=$TOOLCHAIN_DIR:$PATH
echo "------clean old files ------"
make clean
echo "------ start compiling modules -------"
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE -C $KSRC M=$PWD modules
