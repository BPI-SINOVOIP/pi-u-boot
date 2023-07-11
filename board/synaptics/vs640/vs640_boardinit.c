/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 */
#include <common.h>
#include <asm/io.h>
#include <dm/ofnode.h>
#include <linux/delay.h>
#if !CONFIG_IS_ENABLED(SYNA_FASTBOOT)
#include "diag_clock.h"
#endif
#include "Galois_memmap.h"
#include "global.h"

/* for fephy configuration */
#define EPHY_CTRL		0
#define  EPHY_RST_N		(1 << 0)
#define  EPHY_SHUTDOWN		(1 << 1)

#if !CONFIG_IS_ENABLED(SYNA_FASTBOOT)
static void early_clock_config(void)
{
	diag_clock_set_clocks(CLOCK_SET_VH, 0xf);
	list_speed(LIST_ALL_SPEEDS);
}
#endif

static void set_drive_strength(void)
{
	unsigned int offset, val, drv;

	for (offset = RA_Gbl_GPIO_A2Cntl; offset <= RA_Gbl_I2S3_DICntl; offset += 4)
	{
		switch (offset){
			case RA_Gbl_GPIO_A2Cntl ... RA_Gbl_SPI1_SDICntl:
			case RA_Gbl_USB2_DRV_VBUSCntl:
			case RA_Gbl_SCRD0_CRD_PRESCntl ... RA_Gbl_SCRD0_DIOCntl:
			case RA_Gbl_I2S1_DO0Cntl ... RA_Gbl_I2S2_MCLKCntl:
			case RA_Gbl_I2S3_DOCntl ... RA_Gbl_I2S3_DICntl:
				drv = 0x3;
				break;
			case RA_Gbl_TW0_SCLCntl ... RA_Gbl_TW0_SDACntl:
			case RA_Gbl_SDIO0_CDnCntl ... RA_Gbl_SDIO0_WPCntl:
			case RA_Gbl_HDMI_TX_EDDC_SCLCntl ... RA_Gbl_HDMI_TX_EDDC_SDACntl:
				drv = 0x7;
				break;
			case RA_Gbl_SCRD0_DIOCntl + 4 ... RA_Gbl_I2S1_DO0Cntl - 4:
				continue;
			default:
				drv = 0x3;
				break;
		}

		val = readl((long)(MEMMAP_CHIP_CTRL_REG_BASE + offset));
		val |= drv;
		writel(val, (long)(MEMMAP_CHIP_CTRL_REG_BASE + offset));
	}
}

void ephy_poweron(void)
{
	u32 val, ephy_base;
	ofnode node;
	node = ofnode_path("/soc/ephy@fe203c");
	if (!ofnode_valid(node)) {
		debug("%s: no /soc/ephy@fe203c node?\n", __func__);
		return;
	}

	ephy_base = ofnode_get_addr(node);
	if (ephy_base == FDT_ADDR_T_NONE) {
		printf("%s: failed to get ephy base\n", __func__);
		return;
	}

	val = readl((long)(ephy_base + EPHY_CTRL));
	val &= ~EPHY_SHUTDOWN;
	writel(val, (long)(ephy_base + EPHY_CTRL));
	mdelay(10);
	val |= EPHY_RST_N;
	writel(val, (long)(ephy_base + EPHY_CTRL));
	udelay(12);
}

int board_init(void)
{
	//board related things like clock may be inited here
#if !(CONFIG_IS_ENABLED(SYNA_FASTBOOT) || CONFIG_IS_ENABLED(SYNA_OEMBOOT))
	early_clock_config();
#endif
	set_drive_strength();
	ephy_poweron();
	return 0;
}
