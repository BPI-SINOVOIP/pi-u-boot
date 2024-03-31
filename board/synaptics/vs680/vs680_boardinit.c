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
#include "gpio.h"
#include "pinmux.h"
#include "avioGbl.h"
#include "avio_memmap.h"

#define SOC_SM_MEMMAP_EDID_CTRL_BASE	SOC_SM_MEMMAP_SMREG_BASE + RA_smSysCtl_EDID
#define I2C_EDID_SLAVE_ADDR	0x50

    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKPRI                                 0x0290
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKSEC                                 0x0294
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKHD                                  0x0298
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKHDARCTX                             0x029C
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKSPF                                 0x02A0
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKSPF1                                0x02A4
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKPDM                                 0x02A8
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKMIC1                                0x02AC
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_MCLKMIC2                                0x02B0
    ///////////////////////////////////////////////////////////
    #define     RA_AIO_SW_RST                                  0x02B4


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

/*enable master clocks default to 24.576Mhz*/
static void audio_mclk_config(void)
{
	/* copy below definiation from aio.h
		* Audio Port Control register
		###
		%unsigned 1  clk_Enable                0x0
							 ###
							 * Clock Enable
							 ###
							 : DISABLE                   0x0
							 : ENABLE                    0x1
		%unsigned 2  src_sel                   0x0
							 ###
							 * Source Clock Selection
							 * 0: APLL-0 is used as clock source
							 * 1: APLL-1 is used as clock source
							 * 2: External MCLK is used as clock source
							 ###
							 : AVPLL_A3                  0x0
							 : AVPLL_A4                  0x1
							 : MCLK_EXT                  0x2
		%unsigned 1  clkSwitch                 0x0
							 ###
							 * Clock Source Selection
							 * 0: use source clock
							 * 1: use divided clock (decided by clkD3Switch and clkSel)
							 ###
							 : SRC_CLK                   0x0
							 : DIV_CLK                   0x1
		%unsigned 1  clkD3Switch               0x0
							 ###
							 * Divide Factor
							 * 0 : divide factor is decided by clkSwitch and clkSel
							 * 1: use divide by 3 clock when clkSwitch = 1
							 ###
							 : DIV_NOR                   0x0
							 : DIV_3                     0x1
		%unsigned 3  clkSel                    0x0
							 ###
							 * Use the following divide factor if clkD3Switch=0
							 ###
							 : d2                        0x1
							 : d4                        0x2
							 : d6                        0x3
							 : d8                        0x4
							 : d12                       0x5
		%unsigned 1  sw_sync_rst               0x1
							 ###
							 * Soft Reset to Audio Modules
							 ###
							 : ASSERT                    0x0
							 : DEASSERT                  0x1
		%%        23         # Stuffing bits...
	*/
	unsigned int avio_gbl_base_addr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE;
	unsigned int avio_i2s_base_addr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_I2S_REG_BASE;

	/*the value of 0x189 is to divide apll - 196.608MHz by 8 and enable clock*/
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKPRI);
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKSEC);
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKHD);
#ifdef CHIP_VER_Z1
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKHDARCTX);
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKSPF1);
#endif
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKSPF);
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKPDM);
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKMIC1);
	writel(0x189, avio_i2s_base_addr + RA_AIO_MCLKMIC2);

	/*zero AVPLL_CTRL0 to have Normal operation, by default is power down status*/
	writel(0x0, avio_gbl_base_addr + RA_avioGbl_AVPLL_CTRL0);
	/*Clock controls for APLL CLK after APLL*/
	writel(0x269, avio_gbl_base_addr + RA_avioGbl_APLL_WRAP0);
	writel(0x269, avio_gbl_base_addr + RA_avioGbl_APLL_WRAP1);
	/*AVPLLA_CLK_EN -- Enable channel output*/
	writel(0x4D, avio_gbl_base_addr + RA_avioGbl_AVPLLA_CLK_EN);
	/*enable I2S1_MCLK_OEN& PDM_CLK_OEN to output mclk and PDM clock*/
	writel(0x2400, avio_gbl_base_addr + RA_avioGbl_CTRL1);
}

static inline void set_hdmi_rx_hpd(int value)
{
	//SM_GPIO_PortWrite(SM_GPIO_PORT_HDMI_RX_HPD, value);
	GPIO_PortWrite(17, value);
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
	vs680_pin_init();

	audio_mclk_config();

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

	env_set("chip", get_chip_type() & BIT(4) ? "SL1680": "VS680");

	return result;
}
