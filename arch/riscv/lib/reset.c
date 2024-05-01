// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 */

#include <common.h>
#include <command.h>
#include <hang.h>

int do_reset(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	pr_info("resetting ...\n");

	pr_info("reset not supported yet\n");
	hang();

	return 0;
}
