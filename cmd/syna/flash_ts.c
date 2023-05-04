/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2010 Google, Inc.
 *
 * Copyright (C) 2016 Marvell Technology Group Ltd.
 * Author: Xiaoming Lu <xmlu@marvell.com>
 *
 * Copyright (C) 2019 Synaptics Incorporated
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <misc.h>
#include "flash_ts.h"

extern int flash_ts_init(struct udevice *dev);

static int do_fts(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	struct udevice *dev = NULL;
	int res = uclass_get_device_by_name(UCLASS_MISC, "fts", &dev);
	if (res) {
		printf("Can't found misc device named fts\n");
		return CMD_RET_FAILURE;
	}

	if (strcmp(argv[1], "get") == 0) {
		if (argc < 3)
			return CMD_RET_USAGE;
		struct flash_ts_key key;
		key.name = argv[2];
		size_t size = 128;
		if (argc == 4)
			size = simple_strtoul(argv[3], NULL, 10);
		key.value = (char *)malloc(size);
		res = misc_read(dev, 0, (void *)&key, size);
		if (res)
			return CMD_RET_FAILURE;
		printf("fts: %s=%s\n", key.name, key.value);
		free(key.value);
	} else if (strcmp(argv[1], "set") == 0) {
		if (argc < 4)
			return CMD_RET_USAGE;
		struct flash_ts_key key;
		key.name = argv[2];
		key.value = argv[3];
		res = misc_write(dev, 0, (void *)&key, 0);
		if (res)
			return CMD_RET_FAILURE;
		printf("fts: %s set succesfully\n", key.name);
	} else if (strcmp(argv[1], "init") == 0) {
		res = flash_ts_init(dev);
		if (res)
			return CMD_RET_FAILURE;
		printf("fts: init succesfully\n");
	}
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	fts,    4,    0,    do_fts,
	"flash_ts test tools",
	"\n"
	"fts get name [size]\n"
	"fts set name value\n"
	"fts init\n"
);
