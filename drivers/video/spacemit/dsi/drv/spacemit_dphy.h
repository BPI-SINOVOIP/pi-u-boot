// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_DPHY_H_
#define _SPACEMIT_DPHY_H_

#include <linux/types.h>

enum spacemit_dphy_lane_map {
	DPHY_LANE_MAP_0123 = 0,
	DPHY_LANE_MAP_0312 = 1,
	DPHY_LANE_MAP_0231 = 2,
	DPHY_LANE_MAP_MAX
};

enum spacemit_dphy_status {
	DPHY_STATUS_UNINIT = 0,
	DPHY_STATUS_INIT = 1,
	DPHY_STATUS_MAX
};

enum spacemit_dphy_bit_clk_src {
	DPHY_BIT_CLK_SRC_PLL5 = 1,
	DPHY_BIT_CLK_SRC_MUX = 2,
	DPHY_BIT_CLK_SRC_MAX
};

struct spacemit_dphy_timing {
	uint32_t hs_prep_constant;    /* Unit: ns. */
	uint32_t hs_prep_ui;
	uint32_t hs_zero_constant;
	uint32_t hs_zero_ui;
	uint32_t hs_trail_constant;
	uint32_t hs_trail_ui;
	uint32_t hs_exit_constant;
	uint32_t hs_exit_ui;
	uint32_t ck_zero_constant;
	uint32_t ck_zero_ui;
	uint32_t ck_trail_constant;
	uint32_t ck_trail_ui;
	uint32_t req_ready;
	uint32_t wakeup_constant;
	uint32_t wakeup_ui;
	uint32_t lpx_constant;
	uint32_t lpx_ui;
};


struct spacemit_dphy_ctx {
	uint32_t phy_freq; /*kHz*/
	uint32_t lane_num;
	uint32_t esc_clk; /*kHz*/
	uint32_t half_pll5;
	enum spacemit_dphy_bit_clk_src clk_src;
	struct spacemit_dphy_timing dphy_timing;
	uint32_t dphy_status0; /*status0 reg*/
	uint32_t dphy_status1; /*status1 reg*/
	uint32_t dphy_status2; /*status2 reg*/

	enum spacemit_dphy_status status;
};

void spacemit_dphy_init(struct spacemit_dphy_ctx *dphy_ctx);
void spacemit_dphy_get_status(struct spacemit_dphy_ctx *dphy_ctx);
void spacemit_dphy_reset(struct spacemit_dphy_ctx *dphy_ctx);
void spacemit_dphy_uninit(struct spacemit_dphy_ctx *dphy_ctx);

#endif /*_SPACEMIT_DPHY_H_*/
