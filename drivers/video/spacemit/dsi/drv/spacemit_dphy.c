// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <stdio.h>
#include <linux/delay.h>

#include "spacemit_dsi_hw.h"
#include "spacemit_dphy.h"


static unsigned int spacemit_dphy_lane[5] = {0, 0x1, 0x3, 0x7, 0xf};

static void dphy_ana_reset(void)
{
	dsi_clear_bits(DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_RESET);
	udelay(5);
	dsi_set_bits(DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_RESET);
}

static void dphy_set_power(bool poweron)
{
	if(poweron) {
		dsi_set_bits(DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_RESET);
		dsi_set_bits(DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_PU);
	} else {
		dsi_clear_bits(DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_PU);
		dsi_clear_bits(DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_RESET);
	}
}

static void dphy_set_cont_clk(bool cont_clk)
{
	if(cont_clk)
		dsi_set_bits(DSI_PHY_CTRL_1, CFG_DPHY_CONT_CLK);
	else
		dsi_clear_bits(DSI_PHY_CTRL_1, CFG_DPHY_CONT_CLK);

	dsi_set_bits(DSI_PHY_CTRL_1, CFG_DPHY_ADD_VALID);
	dsi_set_bits(DSI_PHY_CTRL_1, CFG_DPHY_VDD_VALID);
}

static void dphy_set_lane_num(uint32_t lane_num)
{
	dsi_write_bits(DSI_PHY_CTRL_2,
		CFG_DPHY_LANE_EN_MASK, spacemit_dphy_lane[lane_num] << CFG_DPHY_LANE_EN_SHIFT);
}

static void dphy_set_bit_clk_src(uint32_t bit_clk_src,
	uint32_t half_pll5)
{
	if(bit_clk_src >= DPHY_BIT_CLK_SRC_MAX) {
		pr_info("%s: Invalid bit clk src (%d)\n", __func__, bit_clk_src);
		return;
	}
}

static void dphy_set_timing(struct spacemit_dphy_ctx *dphy_ctx)
{
    uint32_t bitclk;
	int ui, wakeup;
	int hs_prep, hs_zero, hs_trail, hs_exit, ck_zero, ck_trail;
	int esc_clk, esc_clk_t;
	struct spacemit_dphy_timing *phy_timing;

	if(NULL == dphy_ctx) {
		pr_info("%s: Invalid param!\n", __func__);
		return;
	}

	phy_timing = &(dphy_ctx->dphy_timing);

	esc_clk = dphy_ctx->esc_clk/1000;
	esc_clk_t = 1000/esc_clk;

	bitclk = dphy_ctx->phy_freq / 1000;
	ui = 1000/bitclk + 1;

	/*Jessica: Why no wakeup_ui?*/
	wakeup = phy_timing->wakeup_constant;
	wakeup = wakeup / esc_clk_t + 1;

	hs_prep = phy_timing->hs_prep_constant + phy_timing->hs_prep_ui * ui;
	hs_prep = hs_prep / esc_clk_t + 1;

	/* Our hardware added 3-byte clk automatically.
	 * 3-byte 3 * 8 * ui.
	 */
	hs_zero = phy_timing->hs_zero_constant + phy_timing->hs_zero_ui * ui -
		(hs_prep + 1) * esc_clk_t;
	hs_zero = (hs_zero - (3 * ui << 3)) / esc_clk_t + 4;
	if (hs_zero < 0)
		hs_zero = 0;

	hs_trail = phy_timing->hs_trail_constant + phy_timing->hs_trail_ui * ui;
	hs_trail = ((8 * ui) >= hs_trail) ? (8 * ui) : hs_trail;
	hs_trail = hs_trail / esc_clk_t + 1;
	if (hs_trail > 3)
		hs_trail -= 3;
	else
		hs_trail = 0;

	hs_exit = phy_timing->hs_exit_constant + phy_timing->hs_exit_ui * ui;
	hs_exit = hs_exit / esc_clk_t + 1;

	ck_zero = phy_timing->ck_zero_constant + phy_timing->ck_zero_ui * ui -
		(hs_prep + 1) * esc_clk_t;
	ck_zero = ck_zero / esc_clk_t + 1;

	ck_trail = phy_timing->ck_trail_constant + phy_timing->ck_trail_ui * ui;
	ck_trail = ck_trail / esc_clk_t + 1;

	//dsi_write(DSI_PHY_TIME_0, reg);
	dsi_write(DSI_PHY_TIME_0, 0x06010603);

	//dsi_write(DSI_PHY_TIME_1, reg);
	dsi_write(DSI_PHY_TIME_1, 0x130fcd98);

	//dsi_write(DSI_PHY_TIME_2, reg);
	dsi_write(DSI_PHY_TIME_2, 0x06040c04);

	//dsi_write(DSI_PHY_TIME_3, reg);
	dsi_write(DSI_PHY_TIME_3, 0x43c);

	/* calculated timing on brownstone:
	 * DSI_PHY_TIME_0 0x06080204
	 * DSI_PHY_TIME_1 0x6d2bfff0
	 * DSI_PHY_TIME_2 0x603130a
	 * DSI_PHY_TIME_3 0xa3c
	 */
}

void spacemit_dphy_get_status(struct spacemit_dphy_ctx *dphy_ctx)
{
	if(NULL == dphy_ctx){
		pr_info("%s: Invalid param\n", __func__);
		return;
	}

	dphy_ctx->dphy_status0 = dsi_read(DSI_PHY_STATUS_0);
	dphy_ctx->dphy_status1 = dsi_read(DSI_PHY_STATUS_1);
	dphy_ctx->dphy_status2 = dsi_read(DSI_PHY_STATUS_2);
}

void spacemit_dphy_reset(struct spacemit_dphy_ctx *dphy_ctx)
{
	if(NULL == dphy_ctx){
		pr_info("%s: Invalid param\n", __func__);
		return;
	}

	dphy_ana_reset();
}

/**
 * spacemit_dphy_init - int spacemit dphy
 *
 * @dphy_ctx: pointer to the spacemit_dphy_ctx
 *
 * This function will be called by the dsi driver in order to init the dphy
 * This function will do phy power on, enable continous clk, set dphy timing
 * and set lane number.
 *
 * This function has no return value.
 *
 */
void spacemit_dphy_init(struct spacemit_dphy_ctx *dphy_ctx)
{
	if(NULL == dphy_ctx){
		pr_info("%s: Invalid param\n", __func__);
		return;
	}

	if(DPHY_STATUS_UNINIT != dphy_ctx->status){
		pr_info("%s: dphy_ctx has been initialized (%d)\n",
			__func__, dphy_ctx->status);
		return;
	}

	/*use DPHY_BIT_CLK_SRC_MUX as default clk src*/
	dphy_set_bit_clk_src(dphy_ctx->clk_src, dphy_ctx->half_pll5);

	/* digital and analog power on */
	dphy_set_power(true);

	/* turn on DSI continuous clock for HS */
	dphy_set_cont_clk(true);

	/* set dphy */
	dphy_set_timing(dphy_ctx);

	/* enable data lanes */
	dphy_set_lane_num(dphy_ctx->lane_num);

	dphy_ctx->status = DPHY_STATUS_INIT;

	/* add delay for dsi phy stable */
	mdelay(1);
}

/**
 * spacemit_dphy_uninit - unint spacemit dphy
 *
 * @dphy_ctx: pointer to the spacemit_dphy_ctx
 *
 * This function will be called by the dsi driver in order to unint the dphy
 * This function will disable continous clk, reset dephy, power down dphy
 *
 * This function has no return value.
 *
 */
void spacemit_dphy_uninit(struct spacemit_dphy_ctx *dphy_ctx)
{
	if(NULL == dphy_ctx){
		pr_info("%s: Invalid param\n", __func__);
		return;
	}

	if(DPHY_STATUS_INIT != dphy_ctx->status){
		pr_info("%s: dphy_ctx has not been initialized (%d)\n",
			__func__, dphy_ctx->status);
		return;
	}

	dphy_set_cont_clk(false);
	dphy_ana_reset();
	dphy_set_power(false);

	dphy_ctx->status = DPHY_STATUS_UNINIT;
}
