// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Spacemit
 */

#ifndef _DDR_FREQ_H_
#define _DDR_FREQ_H_

#define PMUA_REG_BASE			0xd4282800
#define DCLK_BYPASS_CLK			(PMUA_REG_BASE + 0x0b0)
#define DDR_CKPHY_PLL1_CTRL1_ADDR	(PMUA_REG_BASE + 0x39c)
#define DDR_CKPHY_PLL1_CTRL2_ADDR	(PMUA_REG_BASE + 0x3a0)
#define DDR_CKPHY_PLL2_CTRL1_ADDR	(PMUA_REG_BASE + 0x3a8)
#define DDR_CKPHY_PLL2_CTRL2_ADDR	(PMUA_REG_BASE + 0x3ac)

#define MAX_FREQ_LV		8

typedef enum {
	DDR_1Gb = 12,
	DDR_2Gb = 0,
	DDR_3Gb,
	DDR_4Gb,
	DDR_6Gb,
	DDR_8Gb,
	DDR_12Gb,
	DDR_16Gb,
	RESERVEDX,
} lpddr4_density_type;

typedef enum {
	DPLL_DIV1,
	DPLL_DIV2,
	DPLL_DIV3,
	DPLL_DIV4,
} pll_div_v2;

typedef enum {
	DPLL_PLL2,
	DPLL_PLL1,
	DPLL_PLL_BYPASS,
} pll_sel_v2;

struct dfc_level_config {
	u32 freq_lv;
	u32 mc_timing_num;
	u32 pll_sel;
	u32 pll_div;
	u32 dclk_pdiv;
	u32 data_rate;
	u32 high_freq;
	u32 vol_lv;
};

union dfc_level_ctrl {
	struct {
		u32 vol_lv:4;
		u32 mc_timing_num:2;
		u32 high_freq:1;
		u32 pll_off:1;
		u32 pll_sel:1;
		u32 kvco:3;
		u32 pll_bypass:1;
		u32 dclk_pdiv:1;
		u32 pll_div:2;
		u32 pll_fbdiv:7;
		u32 band_sel:1;
		u32 pll_frcdiv:8;
	} bit;
	u32 ctrl;
};

u32 ddr_get_density(void);

#endif /* _DDR_FREQ_H_ */
