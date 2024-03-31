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
