/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016~2023 Synaptics Incorporated. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * later as published by the Free Software Foundation.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

#include <common.h>
#include <console.h>
#include <asm/io.h>
#include <dm.h>
#include <flash_ts.h>
#include <misc.h>
#include <usb.h>
#include <mmc.h>
#include "global.h"
#include "Galois_memmap.h"

DECLARE_GLOBAL_DATA_PTR;

#define BL_ARG_MAGIC 0xdeadbeaf
struct bl_boot_arg {
	unsigned long param[8];		// the first param is set to the address of bl_boot_arg.
	unsigned int flash_param[3];	// the original param
	unsigned int leakage;		// leakage id
	unsigned int chip_id[2];	// unique chip id
	unsigned int soc_tsen_id;	// soc tsen_id
	unsigned int cpu_tsen_id;	// cpu tsen_id
	unsigned int chip_revision;	// chip revision: Z1/A0/A1/A2
};

static unsigned long param2_save = 0xFFFFFFFF;

typedef struct boot_info {
	char mac_addr[32];
	char serialno[64];
	char command[64];
	char boot_option[32];
	int  debeg_level;
	char reserved1[32];
	char reserved2[32];
	char reserved3[32];
} boot_info_t;

bool fts_exist = false; 	 //fts partition exist
bool devinfo_exist = false;  //devinfo partition exist
static boot_info_t Boot_info;

#define FTS_NAME_LENGTH		128
#define FTS_VALUE_LENGTH	128

static void check_devinfo_partition(void)
{
	struct blk_desc *dev_desc = NULL;
	disk_partition_t info;

	struct mmc *mmc = find_mmc_device(0);
	if (mmc == NULL) {
		printf("invalid mmc device\n");
	}

	dev_desc = blk_get_dev("mmc", 0);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
	}

	if (part_get_info_by_name(dev_desc, "fts", &info) != -1)
	{
		fts_exist = true;
		printf("Found misc device named fts\n");
	}
	else if (part_get_info_by_name(dev_desc, "devinfo", &info) != -1)
	{
		char * pBuff = malloc(info.blksz);

		memset((void *)&Boot_info, 0, sizeof(boot_info_t));
		if(pBuff)
		{
			blk_dread(dev_desc, info.start, 1, pBuff);
			memcpy((void *)&Boot_info, pBuff, sizeof(boot_info_t));
			free(pBuff);
		}
		devinfo_exist = true;
		printf("Found misc device named devinfo\n");
	}
	else
	{
		printf("Can't found misc device named fts or devinfo \n");
	}

}

#if CONFIG_IS_ENABLED(FASTBOOT)
static int getenv_fastboot(void)
{
	const char *value_fdt = NULL;

	if (fts_exist)
	{
		struct udevice *dev = NULL;
		int res = uclass_get_device_by_name(UCLASS_MISC, "fts", &dev);
		if (res) {
			printf("Can't found misc device named fts\n");
			return -ENODEV;
		}
		char name[FTS_NAME_LENGTH], value[FTS_VALUE_LENGTH];
		struct flash_ts_key key = {name, value};

		strncpy(name, "serialno", FTS_NAME_LENGTH);
		res = misc_read(dev, 0, (void *)&key, FTS_VALUE_LENGTH);
		if (strnlen(value, FTS_VALUE_LENGTH) > 0) {
			printf("Serial#: %s\n", value);
			env_set("serial#", value);
		} else {
			puts("Serial#: 123456789ABCDEF(default)\n");
			env_set("serial#", "123456789ABCDEF");
		}
	}
	else if (devinfo_exist)
	{
		if (strnlen(Boot_info.serialno, FTS_VALUE_LENGTH) > 0) {
			printf("Serial#: %s\n", Boot_info.serialno);
			env_set("serial#", Boot_info.serialno);
		} else {
			puts("Serial#: 123456789ABCDEF(default)\n");
			env_set("serial#", "123456789ABCDEF");
		}
	}

	if ((value_fdt = fdt_getprop(gd->fdt_blob, 0, "fastboot,product-name", NULL))) {
		printf("Product: %s(default)\n", value_fdt);
		env_set("product", value_fdt);
	} else {
		printf("Error: product name not specified.\n");
		env_set("product", "N/A");
	}

	return 0;
}
#endif

#if CONFIG_IS_ENABLED(DM_ETH)
static int getenv_ethaddr(void)
{
	if ((env_get("ethaddr") == NULL) || (env_get("eth1addr") == NULL)) {
		if (fts_exist)
		{
			struct udevice *dev = NULL;
			int res = uclass_get_device_by_name(UCLASS_MISC, "fts", &dev);
			if (res) {
				printf("Can't found misc device named fts\n");
				return -ENODEV;
			}
			char name[FTS_NAME_LENGTH], value[FTS_VALUE_LENGTH];
			struct flash_ts_key key = {name, value};
			strncpy(name, "macaddr", FTS_NAME_LENGTH);
			res = misc_read(dev, 0, (void *)&key, FTS_VALUE_LENGTH);
			if (strnlen(value, FTS_VALUE_LENGTH) <= 0) {
				if (env_get("ethaddr") == NULL)
					puts("get ethaddr error\n");
			} else {
				if (env_get("ethaddr") == NULL)
					env_set("ethaddr", value);
			}
		}
		else if (devinfo_exist)
		{
			if (strnlen(Boot_info.mac_addr, FTS_VALUE_LENGTH) > 0) {
				printf("ethaddr#: %s\n", Boot_info.mac_addr);
				env_set("ethaddr#", Boot_info.mac_addr);
			} else {
				puts("ethaddr#: 11:22:33:44:55:66(default)\n");
				env_set("ethaddr#", "11:22:33:44:55:66");
			}
		}
	}
	return 0;
}
#endif

int arch_misc_init(void)
{
	int ret = 0;

	check_devinfo_partition();

#if CONFIG_IS_ENABLED(FASTBOOT)
	ret = getenv_fastboot();
	if (ret)
		return ret;
#endif
#if CONFIG_IS_ENABLED(DM_ETH)
	ret = getenv_ethaddr();
	if (ret)
		return ret;
#endif
	return ret;
}

void save_boot_params(unsigned long param1, unsigned long param2, unsigned long param3)
{
	if (param1 == BL_ARG_MAGIC)
		param2_save = param2;

	save_boot_params_ret();
}

int get_chip_type(void)
{
	if (param2_save != 0xFFFFFFFF)
		return ((struct bl_boot_arg *)param2_save)->chip_revision >> 16;

	return -1;
}

unsigned long long get_chip_id(void)
{
	unsigned long long system_serial_low = 0, system_serial_high = 0;

	if (param2_save != 0xFFFFFFFF) {
		system_serial_low = ((struct bl_boot_arg *)param2_save)->chip_id[0];
		system_serial_high = ((struct bl_boot_arg *)param2_save)->chip_id[1];
		return system_serial_low << 32 | system_serial_high;
	}

	return 0;
}

int get_chip_rev(void)
{
	int rev_major = -1;
	int rev_minor = -1;

	if (param2_save != 0xFFFFFFFF) {
		rev_major = readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ProductId_ext);
		rev_minor = ((struct bl_boot_arg *)param2_save)->chip_revision & 0xFFFF;

		return rev_major + rev_minor;
	}

	return -1;
}

int get_leakage(void)
{
	if (param2_save != 0xFFFFFFFF)
		return ((struct bl_boot_arg *)param2_save)->leakage;

	return -1;
}
