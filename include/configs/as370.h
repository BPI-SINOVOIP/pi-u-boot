/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Shaojun Feng <sfeng@synaptics.com>
 */

/* Environment in SPI NOR flash */
#define CONFIG_ENV_OFFSET		(2 << 20) /* 1MiB in */
#define CONFIG_ENV_SIZE			(64 << 10) /* 64KiB */
#define CONFIG_ENV_SECT_SIZE		(256 << 10) /* 256KiB sectors */

#define COUNTER_FREQUENCY			25000000 /* 25MHz */

#define CONFIG_SYS_MAX_NAND_DEVICE     1

#define CONFIG_SYS_LOAD_ADDR		0xcf00000

//max malloc length for as370
#define CONFIG_SYS_MALLOC_LEN		(480 << 20)

//#define CONFIG_SYS_MALLOC_F_LEN		(4 << 20) /* Serial is required before relocation */

#define CONFIG_SYS_FLASH_BASE		0xF0000000
#define CONFIG_SYS_MAX_FLASH_BANKS		1

#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_TEXT_BASE - (2 << 20))