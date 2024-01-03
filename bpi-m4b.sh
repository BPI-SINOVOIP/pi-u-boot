#!/bin/bash

set -e

BPIBOARD=bpi-m4berry
MODE=$1

generate_board_config() {
        cat <<-EOT
	CONFIG_ARMV7_BOOT_SEC_DEFAULT=y
	CONFIG_OLD_SUNXI_KERNEL_COMPAT=y
	CONFIG_BPI_CHIP="a64"
	CONFIG_BPI_BOARD="${BPIBOARD}"
	CONFIG_BPI_SERVICE="linux"
	CONFIG_BPI_PATH="bananapi/${BPIBOARD}/linux/"
	CONFIG_BPI_UENVFILE="bananapi/${BPIBOARD}/linux/uEnv.txt"
	EOT

}

export BOARD=${BPIBOARD}
export ARCH=arm
BPICONF=bananapi_m4berry_defconfig
NEWBPICONF=$BPICONF

if [ -z "$MODE" ]; then
	MODE=mainline
else
	MODE=legacy
	export BOARD=${BPIBOARD}-legacy
	NEWBPICONF=BPI-$BPICONF
	cat configs/$BPICONF >configs/$NEWBPICONF
	generate_board_config "$1" >> configs/$NEWBPICONF
fi
echo MODE=$MODE

KBUILD_OUTPUT=out/$BOARD
export KBUILD_OUTPUT
rm -rf $KBUILD_OUTPUT
mkdir -p $KBUILD_OUTPUT
#export CROSS_COMPILE=arm-linux-gnueabihf-
export CROSS_COMPILE=aarch64-linux-gnu-
#export CROSS_COMPILE=aarch64-none-linux-gnu-

#BL31=`pwd`/../arm-trusted-firmware/sunxi/build/sun50iw1p1/debug/bl31.bin
#export BL31
#(cd `pwd`/../arm-trusted-firmware/sunxi ; ./bpi-sunxi64.sh)
#BL31=`pwd`/../arm-trusted-firmware-sunxi-mainline/v2.2/build/sun50i_h6/debug/bl31.bin
#BL31=`pwd`/../arm-trusted-firmware-sunxi-mainline/master/build/sun50i_h616/debug/bl31.bin
BL31=`pwd`/../arm-trusted-firmware/build/sun50i_h616/debug/bl31.bin
export BL31
(cd `pwd`/../ ; ./build_atf.sh)

make $NEWBPICONF
make -j30
./bpi-uimgz.sh $BOARD

