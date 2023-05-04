/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */
#include <common.h>
#include <asm/io.h>
#include "Galois_memmap.h"
#include "global.h"

static void board_specific_init(void)
{
#if defined(CONFIG_MMC) && defined(CONFIG_MMC_DWCMSHC)
	u32 data;

	//PXBAR_ASIB_SEL: 1: Pxbar asib connected to EMMC.
	data = readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_PERIF + RA_PERIF_PXBAR_ASIB_SEL);
	data |= (1 << LSb32PERIF_PXBAR_ASIB_SEL_NAND_EMMC);
	writel(data, (MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_PERIF + RA_PERIF_PXBAR_ASIB_SEL));
#endif
}

int board_init(void)
{
	//board related things like clock may be inited here
	board_specific_init();
	return 0;
}
