// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Spacemit
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include "ddr_freq.h"

#define AP_ALLOW_FREQ_CHG			BIT(18)
#define MC_REG_TABLE_EN				BIT(10)
#define AP_DCLK_FC_DONE_INT_MSK			BIT(15)

#define K1X_APMU_BASE				0xd4282800
#define PMUAP_REG(n)				(K1X_APMU_BASE + n)
#define PMU_CC_CP				PMUAP_REG(0x000)
/* sub-bits of PMU_CC_AP */
#define CP_RD_ST_CLEAR				BIT(31)

#define PMU_CC_AP				PMUAP_REG(0x004)
/* sub-bits of PMU_CC_AP */
#define AP_RD_ST_CLEAR				BIT(31)
#define AP_ALLOW_FREQ_CHG			BIT(18)

#define PMU_DEBUG_REG				PMUAP_REG(0x088)
/* sub-bits of PMU_DEBUG_REG */
#define AP_WFI_FC				BIT(22)
#define CP_WFI_FC				BIT(21)
#define AP_IDLE_IND				BIT(12)
#define CP_IDLE_IND				BIT(11)
#define AP_CLK_OFF_ACK				BIT(4)
#define CP_CLK_OFF_ACK				BIT(3)
#define MC_HALT					BIT(2)
#define AP_HALT					BIT(1)
#define CP_HALT					BIT(0)

#define PMU_AP_IMR				PMUAP_REG(0x098)
/* sub-bits of  */
#define AP_DCLK_FC_DONE_INT_MSK			BIT(15)
#define DCLK_FC_DONE_INT_MSK			BIT(4)

#define PMU_AP_ISR				PMUAP_REG(0x0a0)
/* sub-bits of  */
#define AP_DCLK_FC_DONE_INT_STS			BIT(15)
#define DCLK_FC_DONE_INT_STS			BIT(4)
#define AP_FC_STS				BIT(1)

#define PMU_MC_HW_SLP_TYPE			PMUAP_REG(0x0b0)
/* sub-bits of PMU_MC_HW_SLP_TYPE */
#define MC_REG_TABLE_EN				BIT(10)


#define DFC_AP					PMUAP_REG(0x180)
/* sub-bits of DFC_AP */
#define DFC_FREQ_LV				0x1
#define DFC_REQ					BIT(0)

#define DFC_STATUS				PMUAP_REG(0x188)
/* sub-bits of DFC_STATUS */
#define DFC_CAUSE_SHIFT				0x7
#define DFC_STS					BIT(0)

#define DFC_LEVEL_0				PMUAP_REG(0x190)
/* sub-bits of  */
#define dfc_level_reg(n)			(DFC_LEVEL_0 + (n << 2))

#define FC_LOCK_STATUS				PMUAP_REG(0x334)
/* sub-bits of FC_LOCK_STATUS */
#define CP_RD_STATUS				BIT(1)
#define AP_RD_STATUS				BIT(0)

#define DDR_BYPASS_26M				26
#define DDR_BYPASS_312M				312
#define DDR_BYPASS_416M				416

#define DDR_BASE				0xc0000000
#define DDR_MR_DATA				(DDR_BASE + 0x370)
#define DDR_MR_REG				(DDR_BASE + 0x24)
#define DDR_RFSH_TIMING				(DDR_BASE + 0x394)

#define DDR_FP_NUM				4
#define DDR_PLL_NUM				3


enum DCLK_BYPASS_sel {
	BYPASS_312M = 0,
	BYPASS_416M,
	BYPASS_26M,
	BYPASS_MAX = BYPASS_26M,
};

#define DCLK_BYPASS_SHIFT		19
#define DCLK_BYPASS_CLK_EN		22
#define DCLK_BYPASS_CLK_FC		23
#define DCLK_BYPASS_MASK		(0x3 << DCLK_BYPASS_SHIFT)

#define DDR_CONS		4
#define KHZ			1000
#define FREQ_MAX		~(0U)

u32 ddr_cs_num = DDR_CS_NUM;

static u32 mode_register_read(u32 MR, u32 CH, u32 CS)
{
	u32 read_data;
	u32 UI3 = 0;

	writel((0x10010000 + ((CS + 1) << 24) + (CH << 18) + MR), (void __iomem*)DDR_MR_REG);
	read_data = readl((void __iomem*)DDR_MR_DATA);

	while(!(read_data & 0x80000000)) {
		read_data = readl((void __iomem*)DDR_MR_DATA);
	}

	UI3 = readl((void __iomem*)(DDR_BASE + 0x234)) & 0xFF;
	return UI3;
}

static u32 format_size(u32 density, u32 io_width)
{
	u32 size = 0;

	switch (density) {
	case DDR_2Gb:
		size = 256;
		break;
	case DDR_3Gb:
		size = 384;
		break;
	case DDR_4Gb:
		size = 512;
		break;
	case DDR_6Gb:
		size = 768;
		break;
	case DDR_8Gb:
		size = 1024;
		break;
	case DDR_12Gb:
		size = 1536;
		break;
	case DDR_16Gb:
		size = 2048;
		break;
	default:
		pr_err("donot support such density=0x%x device\n", density);
		return -EINVAL;
	}
	if (io_width == 1)
		size *= 2;

	return size;
}

u32 ddr_get_mr8(void)
{
	u32 mr8;
	mr8 = mode_register_read(8, 0, 0);
	return (mr8&0xff);
}

u32 ddr_get_density(void)
{
	u32 ddr_size = 0;
	u32 mr8_cs00, mr8_cs01, mr8_cs10, mr8_cs11;
	u32 io_width_cs00, io_width_cs01, io_width_cs10, io_width_cs11;
	u32 cs0_size = 0;
	u32 cs1_size = 0;

	mr8_cs00 = mode_register_read(8, 0, 0);
	mr8_cs01 = mode_register_read(8, 1, 0);

	io_width_cs00 = mr8_cs00 ? mr8_cs00 >> 6 : 0;
	io_width_cs01 = mr8_cs01 ? mr8_cs01 >> 6 : 0;

	cs0_size = mr8_cs00 ? format_size(((mr8_cs00 >> 2) & 0xf), io_width_cs00) : 0;
	cs0_size += mr8_cs01 ? format_size(((mr8_cs01 >> 2) & 0xf), io_width_cs01) : 0;

	if (ddr_cs_num > 1) {
		mr8_cs10 = mode_register_read(8, 0, 1);
		mr8_cs11 = mode_register_read(8, 1, 1);

		io_width_cs10 = mr8_cs10 ? mr8_cs10 >> 6 : 0;
		io_width_cs11 = mr8_cs11 ? mr8_cs11 >> 6 : 0;

		cs1_size = mr8_cs10 ? format_size(((mr8_cs10 >> 2) & 0xf), io_width_cs10) : 0;
		cs1_size += mr8_cs11 ? format_size(((mr8_cs11 >> 2) & 0xf), io_width_cs11) : 0;
	}

	ddr_size = cs0_size + cs1_size;
	pr_info("DDR size = %d MB\n", ddr_size);

	return ddr_size;
}

uint32_t get_manufacture_id(void)
{
	uint32_t mr5;

	mr5 = mode_register_read(5, 0, 0);
	pr_info("MR5 = 0x%x\n",mr5);
	return (mr5&0xff);
}

uint32_t get_ddr_rev_id(void)
{
	uint32_t mr6;

	mr6 = mode_register_read(6, 0, 0);
	pr_info("MR6 = 0x%x\n",mr6);
	return (mr6&0xff);
}


/* adjust ddr frequency to the max value */
int ddr_freq_max(void)
{
//	return ddr_freq_change(MAX_FREQ_LV - 1);
	return 0;
}

#ifndef CONFIG_SPL_BUILD

static struct dfc_level_config freq_levels[MAX_FREQ_LV] =
{
/*	freq_lv, timing, pll, pll_div, data_rate, high_freq, vol_lv */
/*	fp 0 == fp 1 just fill in the blanks */
	{0, 0, DPLL_PLL1, DPLL_DIV4, DPLL_DIV1,  600, 0, 0},
	{1, 0, DPLL_PLL1, DPLL_DIV4, DPLL_DIV1,  600, 0, 0},
	{2, 0, DPLL_PLL1, DPLL_DIV3, DPLL_DIV1,  800, 0, 0},
	{3, 0, DPLL_PLL2, DPLL_DIV3, DPLL_DIV1, 1066, 0, 0},
	{4, 0, DPLL_PLL1, DPLL_DIV2, DPLL_DIV1, 1200, 0, 1},
	{5, 1, DPLL_PLL2, DPLL_DIV2, DPLL_DIV1, 1600, 0, 2},
	{6, 2, DPLL_PLL1, DPLL_DIV1, DPLL_DIV1, 2400, 1, 3},
	{7, 3, DPLL_PLL2, DPLL_DIV1, DPLL_DIV1, 3200, 1, 3},
};


static int get_cur_freq_level(void)
{
	u32 level = readl((void __iomem *)DFC_STATUS);
	level = (level >> 1) & 0x7;

	return level;
}

static int dfc_bypass_conf(struct dfc_level_config *cfg)
{
	int bypass_sel, timeout = 1000;
	void __iomem *reg;
	u32 val;

	switch (cfg->data_rate) {
	case DDR_BYPASS_26M:
		bypass_sel = BYPASS_26M;
		break;
	case DDR_BYPASS_312M:
		bypass_sel = BYPASS_312M;
		break;
	case DDR_BYPASS_416M:
		bypass_sel = BYPASS_416M;
		break;
	default:
		return -EINVAL;
	};

	/* set bypass clock */
	reg = (void __iomem *)PMU_MC_HW_SLP_TYPE;
	val = readl(reg);
	val &= ~DCLK_BYPASS_MASK;
	val |= (bypass_sel << DCLK_BYPASS_SHIFT);

	/* bypass clk enable */
	val |= (1 << DCLK_BYPASS_CLK_EN);
	/* bypass clk frequency change request */
	val |= (1 << DCLK_BYPASS_CLK_FC);

	writel(val, reg);

	val = readl(reg);
	while ((val & (1 << DCLK_BYPASS_CLK_FC)) && timeout--) {
		udelay(10);
		val = readl(reg);
	}

	if (timeout <= 0) {
		pr_err("error: switch to bypass clk fail. bypass_sel: %d\n", bypass_sel);
		return -EBUSY;
	}

	return 0;
}

static int dfc_level_cfg(struct dfc_level_config *cfg)
{
	union dfc_level_ctrl level_ctrl;
	u32 level = cfg->freq_lv;
	void __iomem *reg;

	if (level > MAX_FREQ_LV) {
		pr_err("%s: invalid freq level: 0x%x.\n", __func__, level);
		return -EINVAL;
	}

	reg = (void __iomem *)dfc_level_reg((long)level);
	level_ctrl.ctrl = readl(reg);

	if (cfg->pll_sel == DPLL_PLL_BYPASS) {
		level_ctrl.bit.pll_bypass = 1;
		dfc_bypass_conf(cfg);
	} else
		level_ctrl.bit.pll_bypass = 0;

	level_ctrl.bit.vol_lv = cfg->vol_lv & 0xf;
	level_ctrl.bit.mc_timing_num = cfg->mc_timing_num & 0x3;
	level_ctrl.bit.pll_sel = cfg->pll_sel & 0x1;
	level_ctrl.bit.dclk_pdiv = cfg->dclk_pdiv & 0x1;
	level_ctrl.bit.pll_div = cfg->pll_div & 0x3;
	level_ctrl.bit.high_freq = cfg->high_freq & 0x1;
	level_ctrl.bit.pll_off = 0x1;
	writel(level_ctrl.ctrl, reg);

	return 0;
}

static int ddr_vftbl_cfg(void)
{
	int i, ret;

	for (i = 0; i < MAX_FREQ_LV; i++) {
		ret = dfc_level_cfg(&freq_levels[i]);
		if (ret < 0) {
			pr_err("%s: config freq table failed, %d\n", __func__, ret);
			return ret;
		}
	}

	return 0;
}

static int config_core(void)
{
	u32 val;

	/* ap allow freq-change voting */
	val = readl((void __iomem *)PMU_CC_AP);
	val |= AP_ALLOW_FREQ_CHG;
	writel(val, (void __iomem *)PMU_CC_AP);

	/* enable FFC DDR clock change */
	val = readl((void __iomem *)PMU_MC_HW_SLP_TYPE);
	val &= ~MC_REG_TABLE_EN;
	writel(val, (void __iomem *)PMU_MC_HW_SLP_TYPE);

	return 0;
}

/* enable/disable ddr frequency change done interrupt */
static void enable_dfc_int(bool en)
{
	u32 val;

	val = readl((void __iomem *)PMU_AP_IMR);
	if (en) {
		val |= AP_DCLK_FC_DONE_INT_MSK;
	} else {
		val &= ~AP_DCLK_FC_DONE_INT_MSK;
	}
	writel(val, (void __iomem *)PMU_AP_IMR);
}

/* clear ddr frequency change done interrupt status*/
static void clear_dfc_int_status(void)
{
	u32 val;

	val = readl((void __iomem *)PMU_AP_ISR);
	val &= ~(AP_DCLK_FC_DONE_INT_STS | AP_FC_STS);
	writel(val, (void __iomem *)PMU_AP_ISR);
}

static int ddrc_freq_chg(u32 level)
{
	u32 timeout;

	if (level >= MAX_FREQ_LV) {
		pr_err("%s: invalid %d freq level\n", __func__, level);
		return -EINVAL;
	}

	/* check if dfc in progress */
	timeout = 1000;
	while (--timeout) {
		if (!(readl((void __iomem *)DFC_STATUS) & DFC_STS))
			break;
		udelay(10);
	}

	if (!timeout) {
		pr_err("%s: another dfc is in pregress. status:0x%x\n", __func__, readl((void __iomem *)DFC_STATUS));
		return -EBUSY;
	}

	/* trigger frequence change */
	writel(((level & 0x7) << DFC_FREQ_LV) | DFC_REQ, (void __iomem *)DFC_AP);

	timeout = 1000;
	while (--timeout) {
		udelay(10);
		if (!(readl((void __iomem *)DFC_STATUS) & DFC_STS))
			break;
	}

	if (!timeout) {
		pr_err("dfc error! status:0x%x\n", readl((void __iomem *)DFC_STATUS));
		return -EBUSY;
	}

	return 0;
}

static int wait_freq_change_done(void)
{
	int timeout = 100;
	u32 val;

	while (--timeout) {
		udelay(10);
		val = readl((void __iomem *)PMU_AP_ISR);
		if (val & AP_DCLK_FC_DONE_INT_STS)
			break;
	}

	if (!timeout) {
		pr_err("%s: timeout! can not wait dfc done interrupt\n", __func__);
		return -EBUSY;
	}

	return 0;
}

static int ddr_freq_init(void)
{
	int ret;
	static bool ddr_freq_init_flag = false;	

	if(ddr_freq_init_flag == true) {
		return 0;
	}

	#ifdef CONFIG_K1_X_BOARD_ASIC
	ret = ddr_vftbl_cfg();
	if (ret < 0) {
		pr_err("%s failed!\n", __func__);
		return ret;
	}
	#endif

	ddr_freq_init_flag = true;

	return 0;
}

static int ddr_freq_change(u32 freq_level)
{
	int ret, freq_curr;

	ret = ddr_freq_init();
	if (ret < 0) {
		pr_err("ddr_freq_init failed: %d\n", -ret);
		return ret;
	}

	freq_curr = get_cur_freq_level();

	if(freq_curr == freq_level) {
		/* dram frequency is same as the target already */
		return 0;
	}

	config_core();

	/* request change begin */
	enable_dfc_int(true);
	ret = ddrc_freq_chg(freq_level);
	if (ret < 0) {
		pr_err("%s: ddrc_freq_chg fail. ret = %d\n", __func__, ret);
		return ret;
	}

	/* wait for frequency change done */
	ret = wait_freq_change_done();
	if (ret < 0) {
		pr_err("%s: wait_freq_change_done timeout. ret = %d\n", __func__, ret);
		return ret;
	}

	clear_dfc_int_status();
	enable_dfc_int(false);

	pr_info("%s: ddr frequency change from level %d to %d\n", __func__, freq_curr, get_cur_freq_level());

	return 0;
}

int do_ddr_freq(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 freq_level;
	int i;

	if (argc <= 1 || argc > 2) {
		/* invalid parameter, report error */
		return CMD_RET_USAGE;
	}

	if (0 == strcmp(argv[1], "list")) {
		/* show valid frequency list */
		pr_info("support frequency list as shown below:\n");
		for (i = 0; i < ARRAY_SIZE(freq_levels); i++) {
			pr_info("Frequency level: %d, data rate: %dMT/s\n",
				freq_levels[i].freq_lv, freq_levels[i].data_rate);
		}

		return CMD_RET_SUCCESS;
	}

	freq_level = simple_strtoul(argv[1], NULL, 0);
	if(freq_level >= MAX_FREQ_LV) {
		/* invalid parameter, report error */
		return CMD_RET_USAGE;
	}

	ddr_freq_change(freq_level);
	pr_info("Change DDR data rate to %dMT/s\n", freq_levels[get_cur_freq_level()].data_rate);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	ddrfreq, CONFIG_SYS_MAXARGS, 1, do_ddr_freq,
	"Adjusting the DRAM working frequency",
	"ddrfreq list	- display the valid frequncy points\n"
	"ddrfreq [0~7]	- adjust dram working frequency to level[0~7]"
);
#endif
