/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023, Spacemit
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define SYS_DRAM_OFFS		0x00000000
#define SZ_1MB			0x00100000
#define SZ_2GB			0x80000000
#define SZ_4GB			0x100000000ULL
#define SZ_8GB			0x200000000ULL
#define SEC_IMG_SIZE		0x0000000
#define CONFIG_SYS_SDRAM_BASE	(SYS_DRAM_OFFS + SEC_IMG_SIZE)

#define RISCV_MMODE_TIMERBASE	0xE4000000
#define RISCV_MMODE_TIMER_FREQ	24000000
#define RISCV_SMODE_TIMER_FREQ	24000000

#define CONFIG_IPADDR		10.0.92.253
#define CONFIG_SERVERIP		10.0.92.134
#define CONFIG_GATEWAYIP	10.0.92.1
#define CONFIG_NETMASK		255.255.255.0

#define DEFAULT_PRODUCT_NAME	"k1-x_deb1"

#define K1X_SPL_BOOT_LOAD_ADDR	(0x20200000)
#define DDR_TRAINING_DATA_BASE	(0xc0832000)

// sram buffer address that save the DDR software training result
#define DDR_TRAINING_INFO_BUFF	(0xC0800000)
#define DDR_TRAINING_INFO_SAVE_ADDR	(0)
// magic string: "DDRT"
#define DDR_TRAINING_INFO_MAGIC	(0x54524444)
// ddr training software version: xx.xx.xxxx
#define DDR_TRAINING_INFO_VER	(0x00010000)
// default ddr channel number
#define DDR_CS_NUM	(1)

/*
 use (ram_base+4MB offset) as the address to loading image.
 use ram_size-32MB as the max size to loading image, if
 (ram_size-32MB) more than 500MB, set load image size as
 500MB.
*/
#define RECOVERY_RAM_SIZE (gd->ram_size - 0x2000000)
#define RECOVERY_LOAD_IMG_SIZE_MAX (RECOVERY_RAM_SIZE > 0x1f400000 ? 0x1f400000 : RECOVERY_RAM_SIZE)
#define RECOVERY_LOAD_IMG_ADDR (gd->ram_base + 0x400000)
#define RECOVERY_LOAD_IMG_SIZE (RECOVERY_LOAD_IMG_SIZE_MAX)

/* boot mode configs */
#define BOOT_DEV_FLAG_REG	(0xD4282D10)
#define BOOT_PIN_SELECT		(0xD4282c20)

#define BOOT_STRAP_BIT_OFFSET	(9)
#define BOOT_STRAP_BIT_STORAGE_MASK (0x3 << BOOT_STRAP_BIT_OFFSET)
#define BOOT_STRAP_BIT_EMMC	(0x0)
#define BOOT_STRAP_BIT_NOR	(0x1)
#define BOOT_STRAP_BIT_NAND	(0x2)
#define BOOT_STRAP_BIT_SD	(0x3)

/*use CIU register to save boot flag*/
#define BOOT_CIU_REG		(0xD4282C00)
#define BOOT_CIU_DEBUG_REG0	(BOOT_CIU_REG + 0x0390)
#define BOOT_CIU_DEBUG_REG1	(BOOT_CIU_REG + 0x0394)
#define BOOT_CIU_DEBUG_REG2	(BOOT_CIU_REG + 0x0398)

#define K1_EFUSE_USER_BANK0		8
#define K1_DEFALT_PMIC_TYPE		0
#define K1_DEFALT_EEPROM_I2C_INDEX	2
#define K1_DEFALT_EEPROM_PIN_GROUP	0

#define TLV_CODE_SDK_VERSION		0x40
#define TLV_CODE_DDR_CSNUM		0x41

#define TLV_CODE_PMIC_TYPE		0x80
#define TLV_CODE_EEPROM_I2C_INDEX	0x81
#define TLV_CODE_EEPROM_PIN_GROUP	0x82

#ifndef __ASSEMBLY__
#include "linux/types.h"

enum board_boot_mode {
	BOOT_MODE_NONE = 0,
	BOOT_MODE_USB = 0x55a,
	BOOT_MODE_EMMC,
	BOOT_MODE_NAND,
	BOOT_MODE_NOR,
	BOOT_MODE_SD,
	BOOT_MODE_SHELL = 0x55f,
};

struct ddr_training_info_t {
	uint32_t magic;
	uint32_t crc32;
	uint64_t chipid;
	uint64_t mac_addr;
	uint32_t version;
	uint32_t cs_num;
	uint8_t reserved[32];
	uint8_t para[1024];
	uint8_t reserved2[448];
};

struct boot_storage_op
{
	uint32_t boot_storage;
	uint32_t address;
	ulong (*read)(ulong byte_addr, ulong byte_size, void *buff);
	bool (*write)(ulong byte_addr, ulong byte_size, void *buff);
};
#endif

#define MMC_DEV_EMMC	(2)
#define MMC_DEV_SD	(0)

#define BOOTFS_NAME	("bootfs")

/* Environment options */

#define BOOT_TARGET_DEVICES(func) \
	func(QEMU, qemu, na)

#include <config_distro_bootcmd.h>

#define BOOTENV_DEV_QEMU(devtypeu, devtypel, instance) \
	"bootcmd_qemu=" \
	"if env exists kernel_start; then " \
	"bootm ${kernel_start} - ${fdtcontroladdr};" \
	"fi;\0"

#define BOOTENV_DEV_NAME_QEMU(devtypeu, devtypel, instance) \
	"qemu "

#define BOOTENV_DEVICE_CONFIG \
	"product_name=" DEFAULT_PRODUCT_NAME "\0" \
	"serial#=123456789ABC\0" \
	"manufacturer=" CONFIG_SYS_VENDOR "\0" \
	"manufacture_date=01/16/2023 11:02:20\0" \
	"device_version=1\0" \
	"sdk_version=1\0" \
	"pmic_type=" __stringify(K1_DEFALT_PMIC_TYPE) "\0" \
	"eeprom_i2c_index=" __stringify(K1_DEFALT_EEPROM_I2C_INDEX) "\0" \
	"eeprom_pin_group=" __stringify(K1_DEFALT_EEPROM_PIN_GROUP) "\0"

/*if env not use for spl, please define to board/spacemit/k1-x/k1-x.env */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"stdout_flash=serial,vidconsole\0" \
	"kernel_comp_addr_r=0x18000000\0" \
	"kernel_comp_size=0x4000000\0" \
	"scriptaddr=0x2c100000\0" \
	"pxefile_addr_r=0x0c200000\0" \
	"ipaddr=192.168.1.15\0" \
	"netmask=255.255.255.0\0" \
	"serverip=10.0.92.134\0" \
	"gatewayip=192.168.1.1\0" \
	"net_data_path=spacemit_flash_file/net_flash_file/\0" \
	"splashimage=" __stringify(CONFIG_FASTBOOT_BUF_ADDR) "\0" \
	"splashpos=m,m\0" \
	"splashfile=bianbu.bmp\0" \
	BOOTENV_DEVICE_CONFIG


#endif /* __CONFIG_H */
