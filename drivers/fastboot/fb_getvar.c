// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2016 The Android Open Source Project
 */

#include <common.h>
#include <env.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <fb_mmc.h>
#include <fb_nand.h>
#include <fb_mtd.h>
#include <fs.h>
#include <part.h>
#include <version.h>
#include <asm/global_data.h>
#include <mtd.h>
#include <fb_spacemit.h>
#include <command.h>

DECLARE_GLOBAL_DATA_PTR;

static void getvar_version(char *var_parameter, char *response);
static void getvar_version_bootloader(char *var_parameter, char *response);
static void getvar_version_IC(char *var_parameter, char *response);
static void getvar_downloadsize(char *var_parameter, char *response);
static void getvar_serialno(char *var_parameter, char *response);
static void getvar_version_baseband(char *var_parameter, char *response);
static void getvar_product(char *var_parameter, char *response);
static void getvar_platform(char *var_parameter, char *response);
static void getvar_current_slot(char *var_parameter, char *response);
#if CONFIG_IS_ENABLED(SPACEMIT_FLASH)
static void getvar_mtd_size(char *var_parameter, char *response);
static void getvar_blk_size(char *var_parameter, char *response);
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void getvar_has_slot(char *var_parameter, char *response);
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
static void getvar_partition_type(char *part_name, char *response);
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void getvar_partition_size(char *part_name, char *response);
#endif
static void getvar_is_userspace(char *var_parameter, char *response);

static const struct {
	const char *variable;
	void (*dispatch)(char *var_parameter, char *response);
} getvar_dispatch[] = {
	{
		.variable = "version",
		.dispatch = getvar_version
	}, {
		.variable = "version-bootloader",
		.dispatch = getvar_version_bootloader
	}, {
		.variable = "version-IC",
		.dispatch = getvar_version_IC
	}, {
		.variable = "downloadsize",
		.dispatch = getvar_downloadsize
	}, {
		.variable = "max-download-size",
		.dispatch = getvar_downloadsize
	}, {
		.variable = "serialno",
		.dispatch = getvar_serialno
	}, {
		.variable = "version-baseband",
		.dispatch = getvar_version_baseband
	}, {
		.variable = "product",
		.dispatch = getvar_product
	}, {
		.variable = "platform",
		.dispatch = getvar_platform
#if CONFIG_IS_ENABLED(SPACEMIT_FLASH)
	}, {
		.variable = "mtd-size",
		.dispatch = getvar_mtd_size
	}, {
		.variable = "blk-size",
		.dispatch = getvar_blk_size
#endif
	}, {
		.variable = "current-slot",
		.dispatch = getvar_current_slot
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
	}, {
		.variable = "has-slot",
		.dispatch = getvar_has_slot
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	}, {
		.variable = "partition-type",
		.dispatch = getvar_partition_type
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
	}, {
		.variable = "partition-size",
		.dispatch = getvar_partition_size
#endif
	}, {
		.variable = "is-userspace",
		.dispatch = getvar_is_userspace
	}
};

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
/**
 * Get partition number and size for any storage type.
 *
 * Can be used to check if partition with specified name exists.
 *
 * If error occurs, this function guarantees to fill @p response with fail
 * string. @p response can be rewritten in caller, if needed.
 *
 * @param[in] part_name Info for which partition name to look for
 * @param[in,out] response Pointer to fastboot response buffer
 * @param[out] size If not NULL, will contain partition size
 * Return: Partition number or negative value on error
 */
static int getvar_get_part_info(const char *part_name, char *response,
				size_t *size)
{
	int r;
	u32 boot_mode = get_boot_pin_select();
	switch(boot_mode){
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MTD) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MTD)
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
		struct part_info *mtd_part_info;
		r = fastboot_mtd_get_part_info(part_name, &mtd_part_info, response);
		if (r >= 0 && size)
			*size = mtd_part_info->size;
		break;
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
		struct blk_desc *dev_desc;
		struct disk_partition part_info;

		r = fastboot_mmc_get_part_info(part_name, &dev_desc, &part_info,
						response);
		if (r >= 0 && size)
			*size = part_info.size * part_info.blksz;
		break;
#endif
	default:
		fastboot_fail("this storage is not supported in bootloader", response);
		r = -ENODEV;
	}

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_NAND)
	struct part_info *part_info;
	r = fastboot_nand_get_part_info(part_name, &part_info, response);
	if (r >= 0 && size)
		*size = part_info->size;
#endif

	return r;
}
#endif

static void getvar_version(char *var_parameter, char *response)
{
	fastboot_okay(FASTBOOT_VERSION, response);
}

static void getvar_version_bootloader(char *var_parameter, char *response)
{
	fastboot_okay(U_BOOT_VERSION, response);
}

static void getvar_version_IC(char *var_parameter, char *response)
{
	struct fdt_header *working_fdt = (struct fdt_header *)gd->fdt_blob;
	int len_fdt_size;
	int  nodeoffset = fdt_path_offset(working_fdt, "/");
	const char *nodep = fdt_getprop(working_fdt, nodeoffset, "compatible", &len_fdt_size);

	if (nodep && len_fdt_size > 0) {
		fastboot_okay(nodep, response);
	}else
		fastboot_okay("", response);
}

static void getvar_downloadsize(char *var_parameter, char *response)
{
	fastboot_response("OKAY", response, "0x%08x", fastboot_buf_size);
}

static void getvar_serialno(char *var_parameter, char *response)
{
	const char *tmp = env_get("serial#");

	if (tmp)
		fastboot_okay(tmp, response);
	else
		fastboot_fail("Value not set", response);
}

static void getvar_version_baseband(char *var_parameter, char *response)
{
	fastboot_okay("N/A", response);
}

static void getvar_product(char *var_parameter, char *response)
{
	const char *board = env_get("board");

	if (board)
		fastboot_okay(board, response);
	else
		fastboot_fail("Board not set", response);
}

static void getvar_platform(char *var_parameter, char *response)
{
	const char *p = env_get("platform");

	if (p)
		fastboot_okay(p, response);
	else
		fastboot_fail("platform not set", response);
}

static void getvar_current_slot(char *var_parameter, char *response)
{
	/* A/B not implemented, for now always return "a" */
	fastboot_okay("a", response);
}

#if CONFIG_IS_ENABLED(SPACEMIT_FLASH)
/**
 * @brief Get the mtd size and return, if not mtd dev exists, it would return NULL.
	if there have multi mtd devices, it would only return the first one.
 *
 * @param var_parameter
 * @param response
 * @return return
*/
static void getvar_mtd_size(char *var_parameter, char *response)
{
	u32 boot_mode = get_boot_pin_select();
	switch(boot_mode){
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MTD) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MTD)
		/*if select nor/nand, it would check if mtd dev exists or not*/
		struct mtd_info *mtd;
		mtd_probe_devices();
		mtd_for_each_device(mtd) {
			if (!mtd_is_partition(mtd)) {
				if (mtd->size / 0x40000000){
					fastboot_response("OKAY", response, "%lldG", mtd->size / 0x40000000);
					return;
				}
				if (mtd->size / 0x100000){
					fastboot_response("OKAY", response, "%lldM", mtd->size / 0x100000);
					return;
				}
				if (mtd->size / 0x400){
					fastboot_response("OKAY", response, "%lldK", mtd->size / 0x400);
					return;
				}
				return;

			}
		}
		fastboot_fail("flash to mtd dev but can not get mtd size", response);
		return;
#endif
	default:
		fastboot_okay("NULL", response);
		return;
	}
}

/**
 * @brief Get the var blk size object,  if has blk device, it would return
	string universal, or return NULL.
 *
 * @param var_parameter
 * @param response
 */
static void getvar_blk_size(char *var_parameter, char *response)
{
	struct blk_desc *dev_desc = NULL;
	const char *blk_name;
	int blk_index;

	u32 boot_mode = get_boot_pin_select();
	switch(boot_mode){
	case BOOT_MODE_NOR:
#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
		blk_name = CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME;
		blk_index = CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_INDEX;

		/*nvme devices need scan at first*/
		if (!strncmp("nvme", CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME, 4)){
			run_command("nvme scan", 0);
		}

		dev_desc = blk_get_devnum_by_typename(blk_name, blk_index);
		if (dev_desc != NULL)
			fastboot_okay("universal", response);
		else
			fastboot_okay("NULL", response);
		return;
#endif
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
		blk_name = "mmc";
		blk_index = CONFIG_FASTBOOT_FLASH_MMC_DEV;
		dev_desc = blk_get_devnum_by_typename(blk_name, blk_index);
		if (dev_desc != NULL)
			fastboot_okay("universal", response);
		else
			fastboot_okay("NULL", response);
		return;
#endif
	default:
		fastboot_okay("NULL", response);
		return;
	}
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void getvar_has_slot(char *part_name, char *response)
{
	char part_name_wslot[PART_NAME_LEN];
	size_t len;
	int r;

	if (!part_name || part_name[0] == '\0')
		goto fail;

	/* part_name_wslot = part_name + "_a" */
	len = strlcpy(part_name_wslot, part_name, PART_NAME_LEN - 3);
	if (len > PART_NAME_LEN - 3)
		goto fail;
	strcat(part_name_wslot, "_a");

	r = getvar_get_part_info(part_name_wslot, response, NULL);
	if (r >= 0) {
		fastboot_okay("yes", response); /* part exists and slotted */
		return;
	}

	r = getvar_get_part_info(part_name, response, NULL);
	if (r >= 0)
		fastboot_okay("no", response); /* part exists but not slotted */

	/* At this point response is filled with okay or fail string */
	return;

fail:
	fastboot_fail("invalid partition name", response);
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
static void getvar_partition_type(char *part_name, char *response)
{
	int r;
	struct blk_desc *dev_desc;
	struct disk_partition part_info;

	r = fastboot_mmc_get_part_info(part_name, &dev_desc, &part_info,
				       response);
	if (r >= 0) {
		r = fs_set_blk_dev_with_part(dev_desc, r);
		if (r < 0)
			fastboot_fail("failed to set partition", response);
		else
			fastboot_okay(fs_get_type_name(), response);
	}
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void getvar_partition_size(char *part_name, char *response)
{
	int r;
	size_t size = 0;

	r = getvar_get_part_info(part_name, response, &size);
	if (r >= 0)
		fastboot_response("OKAY", response, "0x%016zx", size);
}
#endif

static void getvar_is_userspace(char *var_parameter, char *response)
{
	fastboot_okay("no", response);
}

/**
 * fastboot_getvar() - Writes variable indicated by cmd_parameter to response.
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 *
 * Look up cmd_parameter first as an environment variable of the form
 * fastboot.<cmd_parameter>, if that exists return use its value to set
 * response.
 *
 * Otherwise lookup the name of variable and execute the appropriate
 * function to return the requested value.
 */
void fastboot_getvar(char *cmd_parameter, char *response)
{
	if (!cmd_parameter) {
		fastboot_fail("missing var", response);
	} else {
#define FASTBOOT_ENV_PREFIX	"fastboot."
		int i;
		char *var_parameter = cmd_parameter;
		char envstr[FASTBOOT_RESPONSE_LEN];
		const char *s;

		snprintf(envstr, sizeof(envstr) - 1,
			 FASTBOOT_ENV_PREFIX "%s", cmd_parameter);
		s = env_get(envstr);
		if (s) {
			fastboot_response("OKAY", response, "%s", s);
			return;
		}

		strsep(&var_parameter, ":");
		for (i = 0; i < ARRAY_SIZE(getvar_dispatch); ++i) {
			if (!strcmp(getvar_dispatch[i].variable,
				    cmd_parameter)) {
				getvar_dispatch[i].dispatch(var_parameter,
							    response);
				return;
			}
		}
		pr_warn("WARNING: unknown variable: %s\n", cmd_parameter);
		fastboot_fail("Variable not implemented", response);
	}
}
