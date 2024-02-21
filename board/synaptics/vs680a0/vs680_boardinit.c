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

extern int get_chip_type(void);
extern int get_chip_rev(void);
extern unsigned long long get_chip_id(void);
int checkboard(void)
{
	int chip_type = get_chip_type();
	int chip_rev = get_chip_rev();
	unsigned long long chip_id = get_chip_id();

	if (chip_type == -1)
		return 0;

	printf("Chip:  %s", chip_type & BIT(4) ? "SL1680": "VS680");

	if (chip_rev == -1)
		printf("\n");
	else
		printf(" %2X\n", chip_rev);

	printf("CHIP_ID (byte[0:7]) = %16llx\n", chip_id);
	return 0;
}

int board_init(void)
{
	//board related things like clock may be inited here
#if !(CONFIG_IS_ENABLED(SYNA_FASTBOOT) || CONFIG_IS_ENABLED(SYNA_OEMBOOT))
	early_clock_config();
#endif
	set_drive_strength();
	return 0;
}

#ifdef CONFIG_SYNA_OEMBOOT
//extern int reg_preload_tas(void);

#ifdef CONFIG_SYNA_SM
extern int syna_init_sm(void);
#endif
#endif

int board_late_init(void) {
	unsigned int result = 0;

#ifdef CONFIG_SYNA_OEMBOOT
	//result = reg_preload_tas();
	//if (result) {
	//	printf("register TA failed, ret = 0x%08x\n", result);
	//}

#ifdef CONFIG_SYNA_SM
	result = syna_init_sm();
	if (result) {
		printf("Init SM failed, ret= 0x%08x\n", result);
	}
#endif

	writel(0x01, 0xF7FE2050); //for vcore pmic i2c port select
#endif

	env_set("chip", get_chip_type() & BIT(4) ? "SL1680": "VS680");

	return result;
}
