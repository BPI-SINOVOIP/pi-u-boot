// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_MIPI_H_
#define _SPACEMIT_MIPI_H_

#include <clk.h>
#include <reset.h>

#define HDMI_STATUS				0xc8
#define m_HOTPLUG				(1 << 7)
#define m_MASK_INT_HOTPLUG		(1 << 5)
#define m_INT_HOTPLUG			(1 << 1)
#define v_MASK_INT_HOTPLUG(n)	((n & 0x1) << 5)


struct spacemit_mipi_priv {
	void __iomem *base;

	struct udevice *panel;
	struct mipi_dsi *dsi;

	struct clk pxclk;
	struct clk mclk;
	struct clk hclk;
	struct clk escclk;
	struct clk bitclk;

	struct reset_ctl dsi_reset;
	struct reset_ctl mclk_reset;
	struct reset_ctl esc_reset;
	struct reset_ctl lcd_reset;
};

#endif
