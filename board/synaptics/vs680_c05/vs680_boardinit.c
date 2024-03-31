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
#include "gpio.h"
#include "pinmux.h"

#define SOC_SM_MEMMAP_EDID_CTRL_BASE	SOC_SM_MEMMAP_SMREG_BASE + RA_smSysCtl_EDID
#define I2C_EDID_SLAVE_ADDR	0x50

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

/* Load default EDID for HDMI-RX on boot-up */
static void write_edid_data()
{
	T32EDID_I2C_CTRL reg;
	int i;
	unsigned char edid_data[] = {
#include "vs680_edid.h"
	};
	int size = sizeof(edid_data);
	unsigned int edid_sram_addr = SOC_SM_MEMMAP_EDID_CTRL_BASE + RA_EDID_sram;

	for (i = 0; i < size; i++)
	{
		memcpy((void *)((uint64_t)(edid_sram_addr + 4*i)), &edid_data[i], 1);
	}

	reg.u32 = readl(SOC_SM_MEMMAP_EDID_CTRL_BASE + RA_EDID_I2C_CTRL);

	reg.uI2C_CTRL_slv_addr = I2C_EDID_SLAVE_ADDR;
	reg.uI2C_CTRL_en = 1;
	reg.uI2C_CTRL_cpu_wr_done =  1;
	reg.uI2C_CTRL_cpu_wr_done_ac_dis = 1;
	writel(reg.u32, SOC_SM_MEMMAP_EDID_CTRL_BASE + RA_EDID_I2C_CTRL);
}

/* Config HDMI-RX I2C set-up */
static void set_i2c_ds()
{
	T32smSysCtl_SM_TW2_SCLCntl sclReg;
	T32smSysCtl_SM_TW2_SDACntl sdaReg;

	pinmux_write(SM_TW2_SCL, 0);
	pinmux_write(SM_TW2_SDA, 0);
	sclReg.u32 = readl(SOC_SM_MEMMAP_SMREG_BASE + RA_smSysCtl_SM_TW2_SCLCntl);
	sdaReg.u32 = readl(SOC_SM_MEMMAP_SMREG_BASE + RA_smSysCtl_SM_TW2_SDACntl);

	sclReg.uSM_TW2_SCLCntl_DS0 = 1;
	sclReg.uSM_TW2_SCLCntl_DS1 = 1;
	sclReg.uSM_TW2_SCLCntl_DS2 = 1;
	sclReg.uSM_TW2_SCLCntl_DS3 = 0;
	sdaReg.uSM_TW2_SDACntl_DS0 = 1;
	sdaReg.uSM_TW2_SDACntl_DS1 = 1;
	sdaReg.uSM_TW2_SDACntl_DS2 = 1;
	sdaReg.uSM_TW2_SDACntl_DS3 = 0;

	writel(sclReg.u32, SOC_SM_MEMMAP_SMREG_BASE + RA_smSysCtl_SM_TW2_SCLCntl);
	writel(sdaReg.u32, SOC_SM_MEMMAP_SMREG_BASE + RA_smSysCtl_SM_TW2_SDACntl);
}

static inline void set_hdmi_rx_hpd(int value)
{
	//SM_GPIO_PortWrite(SM_GPIO_PORT_HDMI_RX_HPD, value);
	GPIO_PortWrite(55, value);
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
	vs680_pin_init();

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
	printf("board_late_init: start Init HDMI RX\n");
	write_edid_data();
	set_i2c_ds();
	set_hdmi_rx_hpd(1);
	printf("board_late_init: Init HDMI RX done\n");
#endif

	return result;
}
