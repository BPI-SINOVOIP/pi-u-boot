/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2008 - 2009 Windriver, <www.windriver.com>
 * Author: Tom Rix <Tom.Rix@windriver.com>
 *
 * (C) Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * Copyright (C) 2019 Synaptics Incorporated
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 *
 */

#include <common.h>
#include <command.h>
#include <stdio.h>

extern unsigned int get_max_malloc_size(void);


static int do_reg_tas(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("%s:%s\n",__FILE__,__func__);

	extern int reg_preload_tas(void);
	int ret = reg_preload_tas();
	if(ret) {
		printf("register ta failed: %d", ret);
		do_reset(NULL, 0, 0, NULL);
		return CMD_RET_FAILURE;
	}
	printf("register ta success: %d\n", ret);
	return 0;
}


U_BOOT_CMD(
	reg_tas, 1, 0, do_reg_tas,
	"register ta",
	"register Trusted Applications which the system needed\n"
	"example:\n"
	"	reg_tas\n"
);
