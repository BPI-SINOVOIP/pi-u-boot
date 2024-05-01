// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_DPU_H_
#define _SPACEMIT_DPU_H_
#include <clk.h>
#include <reset.h>
#include "./dsi/include/spacemit_video_tx.h"


#define DPU_INT_REG_24	0x960
#define DPU_INT_REG_14	0x938

#define OUTFMT_RGB121212	0
#define OUTFMT_RGB101010	1
#define OUTFMT_RGB888		2
#define OUTFMT_RGB666		12
#define OUTFMT_RGB565		13

enum dpu_modes {
	DPU_MODE_EDP = 0,
	DPU_MODE_MIPI,
	DPU_MODE_HDMI,
	DPU_MODE_LVDS,
	DPU_MODE_DP,
};

enum dpu_features {
	DPU_FEATURE_OUTPUT_10BIT = (1 << 0),
};

enum {
	POWER_INVALID = 0,
	POWER_OFF,
	POWER_ON,
};

struct fb_info {
	struct spacemit_mode_modeinfo mode;
	struct video_tx_device *tx;
};

struct spacemit_dpu_priv {
	void __iomem *regs_dsi;
	void __iomem *regs_hdmi;
	struct udevice *conn_dev;
	struct display_timing timing;
};

struct spacemit_dpu_driverdata {
	/* configuration */
	u32 features;
	/* block-specific setters/getters */
	void (*set_pin_polarity)(struct udevice *, enum dpu_modes, u32);
};

#endif
