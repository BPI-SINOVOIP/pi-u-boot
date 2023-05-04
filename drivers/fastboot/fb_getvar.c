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
#include <fs.h>
#include <version.h>
#include "fastboot_syna.h"

static void getvar_version(char *var_parameter, char *response);
static void getvar_version_bootloader(char *var_parameter, char *response);
static void getvar_downloadsize(char *var_parameter, char *response);
static void getvar_serialno(char *var_parameter, char *response);
static void getvar_version_baseband(char *var_parameter, char *response);
static void getvar_product(char *var_parameter, char *response);
static void getvar_unlocked(char *var_parameter, char *response);
static void getvar_unlocked_critical(char *var_parameter, char *response);
static void getvar_platform(char *var_parameter, char *response);
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void getvar_has_slot(char *var_parameter, char *response);
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC)
static void getvar_partition_type(char *part_name, char *response);
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void getvar_partition_size(char *part_name, char *response);
#endif
static void getvar_is_userspace(char *var_parameter, char *response);
#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_AB)
static void getvar_current_slot(char *var_parameter, char *response);
static void getvar_slot_count(char *var_parameter, char *response);
static void getvar_slot_successful(char *var_parameter, char *response);
static void getvar_slot_unbootable(char *var_parameter, char *response);
static void getvar_slot_retry_count(char *var_parameter, char *response);
static void getvar_slot_suffixes(char *var_parameter, char *response);
#endif

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
		.variable = "unlocked",
		.dispatch = getvar_unlocked
	}, {
		.variable = "unlocked-critical",
		.dispatch = getvar_unlocked_critical
	}, {
		.variable = "platform",
		.dispatch = getvar_platform
# if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
	}, {
		.variable = "has-slot",
		.dispatch = getvar_has_slot
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC)
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
	},
#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_AB)
	{
		.variable = "current-slot",
		.dispatch = getvar_current_slot
	}, {
		.variable = "slot-count",
		.dispatch = getvar_slot_count
	}, {
		.variable = "slot-successful",
		.dispatch = getvar_slot_successful
	}, {
		.variable = "slot-unbootable",
		.dispatch = getvar_slot_unbootable
	}, {
		.variable = "slot-retry-count",
		.dispatch = getvar_slot_retry_count
	}, {
		.variable = "slot-suffixes",
		.dispatch = getvar_slot_suffixes
	}
#endif
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
 * @param[out] size If not NULL, will contain partition size (in bytes)
 * @return Partition number or negative value on error
 */
static int getvar_get_part_info(const char *part_name, char *response,
				size_t *size)
{
	int r;
# if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC)
	struct blk_desc *dev_desc;
	disk_partition_t part_info;

	r = fastboot_mmc_get_part_info(part_name, &dev_desc, &part_info,
				       response);
	if (r >= 0 && size)
		*size = part_info.size * part_info.blksz;
# elif CONFIG_IS_ENABLED(FASTBOOT_FLASH_NAND)
	struct part_info *part_info;

	r = fastboot_nand_get_part_info(part_name, &part_info, response);
	if (r >= 0 && size)
		*size = part_info.size * part_info.blksz;
# else
	fastboot_fail("this storage is not supported in bootloader", response);
	r = -ENODEV;
# endif

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
	const char *board = env_get("product");

	if (board)
		fastboot_okay(board, response);
	else
		fastboot_fail("Board not set", response);
}

static void getvar_unlocked(char *var_parameter, char *response)
{
	unsigned int lock_flag, lock_critical_flag;
	int ret = get_lock_flag(&lock_flag, &lock_critical_flag);
	if (ret) {
		fastboot_fail("get_oem_lock_flag failed", response);
	}

	if (lock_flag)
		fastboot_okay("no", response);
	else
		fastboot_okay("yes", response);

}

static void getvar_unlocked_critical(char *var_parameter, char *response)
{
	unsigned int lock_flag, lock_critical_flag;
	int ret = get_lock_flag(&lock_flag, &lock_critical_flag);
	if (ret) {
		fastboot_fail("get_oem_lock_flag failed", response);
	}

	if (lock_critical_flag)
		fastboot_okay("no", response);
	else
		fastboot_okay("yes", response);

}

static void getvar_platform(char *var_parameter, char *response)
{
	const char *p = env_get("platform");

	if (p)
		fastboot_okay(p, response);
	else
		fastboot_fail("platform not set", response);
}

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

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC)
static void getvar_partition_type(char *part_name, char *response)
{
	char *s;
	s = fastboot_check_partition_type(part_name);
	if (!strcmp(s, "partition not found!"))
		fastboot_fail(s,response);
	else
		fastboot_okay(s, response);
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void getvar_partition_size(char *part_name, char *response)
{
	int r;
	size_t size;

	r = getvar_get_part_info(part_name, response, &size);
	if (r >= 0)
		fastboot_response("OKAY", response, "0x%016zx", size);
}
#endif

static void getvar_is_userspace(char *var_parameter, char *response)
{
	fastboot_okay("no", response);
}

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_AB)
static void getvar_current_slot(char *var_parameter, char *response)
{
	int slot_index = get_current_slot();

	if (slot_index != BOOTSEL_INVALID)
		fastboot_response("OKAY", response, "%s", SLOT_NAME(slot_index));
	else
		fastboot_fail("current-slot not set", response);
}

static void getvar_slot_count(char *var_parameter, char *response)
{
	char str_num[12];
	int slot_count = get_slot_count();
	if (slot_count > 0) {
		sprintf(str_num, "%d", slot_count);
		fastboot_response("OKAY", response, "%s", str_num);
	} else
		fastboot_fail("slot-count not set", response);
}

static void getvar_slot_successful(char *var_parameter, char *response)
{
	char *cmd = var_parameter;
	int slot_index = BOOTSEL_INVALID;

	if (cmd)
		slot_index = SLOT_INDEX(cmd);

	if (!INDEX_VALID(slot_index))
		fastboot_fail("slot-successful failed", response);
	else {
		if (is_slot_successful(slot_index))
			fastboot_okay("yes", response);
		else
			fastboot_okay("no", response);
	}
}

static void getvar_slot_unbootable(char *var_parameter, char *response)
{
	char *cmd = var_parameter;
	int slot_index = BOOTSEL_INVALID;

	if (cmd)
		slot_index = SLOT_INDEX(cmd);

	if (!INDEX_VALID(slot_index))
		fastboot_fail("slot-unbootable failed", response);
	else {
		if (is_slot_unbootable(slot_index))
			fastboot_okay("yes", response);
		else
			fastboot_okay("no", response);
	}
}

static void getvar_slot_retry_count(char *var_parameter, char *response)
{
	char *cmd = var_parameter;
	int slot_index = BOOTSEL_INVALID;

	if (cmd)
		slot_index = SLOT_INDEX(cmd);

	if (!INDEX_VALID(slot_index))
		fastboot_fail("slot-retry-count failed", response);
	else {
		fastboot_response("OKAY", response, "%d", get_slot_retry_count(slot_index));
	}
}

static void getvar_slot_suffixes(char *var_parameter, char *response)
{
	int slot_index = get_current_slot();

	if (slot_index != BOOTSEL_INVALID)
		fastboot_response("OKAY", response, "%s", SLOT_NAME(slot_index));
	else
		fastboot_fail("slot-suffixes not set", response);
}
#endif

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
