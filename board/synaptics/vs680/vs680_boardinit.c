/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */
#include <common.h>
#include <asm/io.h>
#if !CONFIG_IS_ENABLED(SYNA_FASTBOOT)
#include "diag_clock.h"
#endif
#include "Galois_memmap.h"
#include "global.h"
#include "SysMgr.h"
#include "watchdog_def.h"

static void set_drive_strength(void)
{
	unsigned int offset, val, drv;

	for (offset = RA_Gbl_SDIO_CDnCntl; offset <= RA_Gbl_I2S3_DICntl; offset += 4)
	{
		switch (offset){
			case RA_Gbl_SDIO_CDnCntl ... RA_Gbl_SDIO_WPCntl:
			case RA_Gbl_TW0_SCLCntl ... RA_Gbl_TW0_SDACntl:
			case RA_Gbl_TX_EDDC_SCLCntl ... RA_Gbl_TX_EDDC_SDACntl:
				drv = 0x7;
				break;
			case RA_Gbl_RGMII_RXCTLCntl+4 ... RA_Gbl_I2S1_DO0Cntl - 4:
				continue;
			default:
				drv = 0x3;
				break;
		}

		val = readl((long)(MEMMAP_CHIP_CTRL_REG_BASE + offset));
		val |= drv;
		writel(val, (long)(MEMMAP_CHIP_CTRL_REG_BASE + offset));
	}

	for (offset = RA_smSysCtl_SM_TW3_SCLCntl; offset <= RA_smSysCtl_SM_TW3_SDACntl; offset += 4 )
	{
		switch (offset){
			case RA_smSysCtl_SM_TW3_SCLCntl ... RA_smSysCtl_SM_TW3_SDACntl:
				drv = 0x7;
				break;
			default:
				continue;
		}

		val = readl(SOC_SM_SYS_CTRL_REG_BASE + offset);
		val |= drv;
		writel(val, SOC_SM_SYS_CTRL_REG_BASE + offset);
	}
}

#if !CONFIG_IS_ENABLED(SYNA_FASTBOOT)
static void early_clock_config(void)
{
	diag_clock_set_clocks(CLOCK_SET_T2, 0xf);
	list_speed(LIST_ALL_SPEEDS);
}
#endif

int board_init(void)
{
	//board related things like clock may be inited here
#if !(CONFIG_IS_ENABLED(SYNA_FASTBOOT) || CONFIG_IS_ENABLED(SYNA_OEMBOOT))
	early_clock_config();
#endif
	set_drive_strength();
	return 0;
}
