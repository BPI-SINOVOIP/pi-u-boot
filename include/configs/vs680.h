/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Shaojun Feng <sfeng@synaptics.com>
 */

#ifdef CONFIG_SPI_FLASH
/* Environment in SPI NOR flash */
#define CONFIG_ENV_OFFSET		(2 << 20) /* 1MiB in */
#define CONFIG_ENV_SIZE			(64 << 10) /* 64KiB */
#define CONFIG_ENV_SECT_SIZE		(64 << 10) /* 256KiB sectors */
#endif

/* BPI */
#undef CONFIG_BOOTCOMMAND
#ifdef CONFIG_SD_BOOT
/* u-boot env in sd/mmc card */
#define FAT_ENV_INTERFACE	"mmc"
#define FAT_ENV_DEVICE_AND_PART	"1:1"
#define FAT_ENV_FILE		"uboot.env"
/* bootstrap + u-boot + env in sd card */
#define CONFIG_BOOTCOMMAND	"echo BPI: get boot.scr (20240425 support SD/EMMC); " \
				"setenv devnum 1:1; " \
				"mmc rescan ; mmc list; echo BPI:try SD; " \
				"if mmc dev 1; then echo BPI:do SD; " \
				"else setenv devnum 0:1; mmc dev 0; " \
				"echo BPI:try EMMC; fi ; " \
				"load mmc ${devnum} 0x04a80000 boot.scr; " \
				"source 0x04a80000; " \
				"mmc rescan ; mmc list; mmc dev 0; " \
				"load mmc 0:1 0x15a00000 dtb/synaptics/vs680-a0-bananapi-m6.dtb; " \
				"load mmc 0:1 0x0ca00000 uInitrd; " \
				"load mmc 0:1 0x04a80000 Image; " \
				"booti 0x04a80000 0x0ca00000 0x15a00000;"
#undef CONFIG_BOOTARGS
#define CONFIG_BOOTARGS \
	"rootfstype=ext4 root=/dev/mmcblk1p2 rw rootwait"
#endif

#define COUNTER_FREQUENCY			25000000 /* 25MHz */

#define CONFIG_SYS_MAX_NAND_DEVICE     1

#define CONFIG_SYS_LOAD_ADDR		0xcf00000
#define CONFIG_SYS_AUTOLOAD		"n"			/* disable autoload image via tftp after dhcp */

//max malloc length for vs680
#define CONFIG_SYS_MALLOC_LEN		(1480 << 20)

//#define CONFIG_SYS_MALLOC_F_LEN		(4 << 20) /* Serial is required before relocation */

#define CONFIG_SYS_FLASH_BASE		0xF0000000
#define CONFIG_SYS_MAX_FLASH_BANKS		1

#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_TEXT_BASE - (2 << 20))

/* Watchdog configs*/
#define CONFIG_HW_WATCHDOG
#define CONFIG_DESIGNWARE_WATCHDOG

#define CONFIG_DW_WDT_BASE		0xF7FC5000

#define CONFIG_DW_WDT_CLOCK_KHZ	25000
#define CONFIG_WATCHDOG_TIMEOUT_MSECS 	0

/* Fastboot configs*/
#define CONFIG_GICV2
#define GICD_BASE				0xf7901000
#define GICC_BASE				0xf7902000

#ifdef CONFIG_CMD_SYNA_FASTBOOT_GUI
# define CONFIG_VIDEO_BMP_GZIP
# define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE (100*100*4)
# define CONFIG_CMD_BMP
# define CONFIG_BMP_24BMP
# define CONFIG_BMP_32BPP
#endif
