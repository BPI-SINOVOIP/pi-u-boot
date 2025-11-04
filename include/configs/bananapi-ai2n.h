/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023 Renesas Electronics Corporation
 */

#ifndef __BANANAPI_AI2N_H
#define __BANANAPI_AI2N_H

#include <asm/arch/rmobile.h>

#define CONFIG_REMAKE_ELF

#ifdef CONFIG_SPL
#define CONFIG_SPL_TARGET	"spl/u-boot-spl.scif"
#endif

/* boot option */

#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG

/* Generic Interrupt Controller Definitions */
/* RZ/V2N use GIC-v3 */
#define CONFIG_GICV3
#define GICD_BASE	0x14900000
#define GICR_BASE	0x14940000

/* console */
#define CONFIG_SYS_CBSIZE		2048
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BAUDRATE_TABLE	{ 115200, 38400 }

/* PHY needs a longer autoneg timeout */
#define PHY_ANEG_TIMEOUT		20000

/* MEMORY */
#define CONFIG_SYS_INIT_SP_ADDR		CONFIG_SYS_TEXT_BASE

/* SDHI clock freq */
#define CONFIG_SH_SDHI_FREQ		133000000

#define DRAM_RSV_SIZE			0x08000000
#define CONFIG_SYS_SDRAM_BASE		(0x40000000 + DRAM_RSV_SIZE)
#define CONFIG_SYS_SDRAM_SIZE		(0x200000000u - DRAM_RSV_SIZE) //total 8GB
#define CONFIG_SYS_LOAD_ADDR		0x58000000
/* Default load address for tfpt,bootp */
#define CONFIG_LOADADDR			CONFIG_SYS_LOAD_ADDR
#define CONFIG_VERY_BIG_RAM
#define CONFIG_MAX_MEM_MAPPED		(0x80000000u - DRAM_RSV_SIZE)

#define CONFIG_SYS_MONITOR_BASE		0x00000000
#define CONFIG_SYS_MONITOR_LEN		(1 * 1024 * 1024)
#define CONFIG_SYS_MALLOC_LEN		(64 * 1024 * 1024)
#define CONFIG_SYS_BOOTM_LEN		(64 << 20)

/* The HF/QSPI layout permits up to 1 MiB large bootloader blob */
#define CONFIG_BOARD_SIZE_LIMIT		1048576

/* ENV setting */
#define ENV_MMC_LIST_DEFAULT            "mmc_list=1 0\0"
#define ENV_BOOT_ORDER_DEFAULT          "boot_order=mmc\0"
#define ENV_BOOTSCRIPTS_PREFIX          "boot_prefixes=/ /boot/\0"
#define ENV_BOOT_ATTEMPT_DEFAULT                        \
        "boot_attempt="                                         \
            "for prefix_idx in ${boot_prefixes}; do "           \
                "setenv prefix ${prefix_idx}; "                 \
	        "if test \"${prefix}\" = \"/\"; then "          \
                    "setenv rootdev /dev/mmcblk${devnum}p2; "   \
                "elif test \"${prefix}\" = \"/boot/\"; then "   \
                    "setenv rootdev /dev/mmcblk${devnum}p1; "   \
                "fi; "                                          \
                "echo \"## Attempting fetch ${prefix}${bootscript} in ${devtype}:${devnum}...\"; "  \
                "if test -e ${devtype} ${devnum} ${prefix}${bootscript}; then "       \
                    "load ${devtype} ${devnum} ${bootscript_addr} ${prefix}${bootscript}; " \
                    "source ${bootscript_addr}; "                               \
                    "echo SCRIPT FAILED: continuing...; "   \
                "fi; "                                          \
          "done\0"

#define ENV_MMC_DEFAULT                                 \
        "boot_mmc="                                     \
            "echo \"## run boot_mmc\"; "        \
                "setenv devtype mmc; "                  \
                "for n in ${mmc_list}; do "             \
                        "setenv devnum ${n}; "          \
                        "run boot_attempt; "            \
                "done\0"

#define ENV_BOOT_DEFAULT                                \
        "boot_default="                                 \
                "for type in ${boot_order}; do "        \
                        "run boot_${type}; "            \
                "done\0"

#define CONFIG_EXTRA_ENV_SETTINGS	\
	ENV_MMC_DEFAULT \
	ENV_MMC_LIST_DEFAULT \
	ENV_BOOTSCRIPTS_PREFIX \
	ENV_BOOT_ORDER_DEFAULT \
	ENV_BOOT_DEFAULT \
	ENV_BOOT_ATTEMPT_DEFAULT \
	"rootdev=/dev/mmcblk0p2\0" \
	"fdtfile=bananapi-ai2n.dtb\0" \
	"bootscript=boot.scr\0"       \
	"bootscript_addr=0x48070000\0"  \
	"kernel_addr_r=0x48080000\0"  \
	"fdt_addr_r=0x48000000\0"  \
	"ramdisk_addr_r=0x50000000\0"  \
	"usb_pgood_delay=2000\0" \
	"bootm_size=0x10000000\0" \
	"ipaddr=192.168.1.11\0" \
	"serverip=192.168.1.10\0" \
	"netmask=255.255.255.0\0" \
	"prodsdbootargs=setenv bootargs rw rootwait earlycon root=/dev/mmcblk1p2 net.ifnames=0 \0" \
	"prodemmcbootargs=setenv bootargs rw rootwait earlycon root=/dev/mmcblk0p2 net.ifnames=0 \0" \
	"bootimage=booti ${kernel_addr_r} - ${fdt_addr_r} \0" \
	"ocaaddr=0xA8000000 \0"     \
	"ocabin=OpenCV_Bin.bin \0"  \
	"codaddr=0xAFD00000 \0"     \
	"codbin=Codec_Bin.bin \0"   \
	"emmcload=ext4load mmc 0:2 ${ocaaddr} boot/${ocabin}; ext4load mmc 0:2 ${codaddr} boot/${codbin}; ext4load mmc 0:2 ${kernel_addr_r} boot/Image;ext4load mmc 0:2 ${fdt_addr_r} boot/${fdtfile};run prodemmcbootargs \0" \
	"sd1load=ext4load mmc 1:2 ${ocaaddr} boot/${ocabin}; ext4load mmc 1:2 ${codaddr} boot/${codbin}; ext4load mmc 1:2 ${kernel_addr_r} boot/Image;ext4load mmc 1:2 ${fdt_addr_r} boot/${fdtfile};run prodsdbootargs \0" \
	"bootcmd_check=if mmc dev 1; then run sd1load; else run emmcload; fi \0"

#define CONFIG_BOOTCOMMAND     "run boot_default;run bootcmd_check;run bootimage"

/* For board */
/* Ethernet RAVB */
#define CONFIG_BITBANGMII_MULTI

#endif /* __BANANAPI_AI2N_H */
