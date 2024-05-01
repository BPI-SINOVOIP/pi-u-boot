// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_HDMI_H_
#define _SPACEMIT_HDMI_H_

#include <clk.h>
#include <reset.h>


#define HDMI_STATUS				0xc8
#define m_HOTPLUG				(1 << 7)
#define m_MASK_INT_HOTPLUG		(1 << 5)
#define m_INT_HOTPLUG			(1 << 1)
#define v_MASK_INT_HOTPLUG(n)	((n & 0x1) << 5)


struct spacemit_hdmi_priv {
	struct dw_hdmi hdmi;
	void __iomem *base;

	struct clk hdmi_mclk;
	struct reset_ctl hdmi_reset;
};

#endif
