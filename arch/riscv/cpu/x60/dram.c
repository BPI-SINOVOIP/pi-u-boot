// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <fdtdec.h>
#include <init.h>
#include <asm/global_data.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

__weak int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

__weak int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

