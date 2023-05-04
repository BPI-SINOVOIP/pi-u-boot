/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <reset.h>
#include <asm/io.h>

#include "global.h"

#define SRC_SYSPLL 8
#define CLKEN       (1 << 0)
#define CLKPLLSEL_MASK  7
#define CLKPLLSEL_SHIFT 1
#define CLKPLLSWITCH    (1 << 4)
#define CLKSWITCH   (1 << 5)
#define CLKD3SWITCH (1 << 6)
#define CLKSEL_MASK 7
#define CLKSEL_SHIFT    7

static int div_t[] = { 3, 2, 4, 6, 8, 12, 0};

struct syna_clk_priv {
	fdt_addr_t base;
	u32 src;
	u32 div;
	u32 freq;
};

static int convert_div(u32 div)
{
	int i = 0;
	for(i = 0; i < sizeof(div_t)/sizeof(int); i++) {
		if(div_t[i] == div)
			return i;
	}
	return -EINVAL;
}

static ulong syna_clk_get_rate(struct clk *clk)
{
	struct syna_clk_priv *priv = dev_get_priv(clk->dev);

	//freq may be used to check if it matchs with src and div
	return priv->freq;
}

static ulong syna_clk_set_rate(struct clk *clk, ulong rate)
{
	return 0;
}

static int syna_clk_enable(struct clk *clk)
{
	struct syna_clk_priv *priv = dev_get_priv(clk->dev);
	u32 val;

	u32 src = priv->src;
	u32 div = priv->div;
	u32 div_factor = (u32)convert_div(div);
	val = readl(priv->base);

	if(src == SRC_SYSPLL) {
		val &= ~CLKPLLSWITCH;
	} else {
		val |= CLKPLLSWITCH;
		val &= ~(CLKPLLSEL_MASK << CLKPLLSEL_SHIFT);
		val |= src << CLKPLLSEL_SHIFT;
	}

	if(div == 0) {
		val &= ~CLKSWITCH;
	} else {
		val |= CLKSWITCH;
		if(div == 3) {
			val |= CLKD3SWITCH;
		} else {
			val &= ~CLKD3SWITCH;
			val &= ~(CLKSEL_MASK << CLKSEL_SHIFT);
			val |= div_factor << CLKSEL_SHIFT;
		}
	}

	val |= CLKEN;
	writel(val, priv->base);

	return 0;
}

static int syna_clk_disable(struct clk *clk)
{
	struct syna_clk_priv *priv = dev_get_priv(clk->dev);
	T32clkD2_ctrl * clkctrl = (T32clkD2_ctrl *)(priv->base);

	clkctrl->uctrl_ClkEn = 0;

	return 0;
}

int syna_clk_probe(struct udevice *dev)
{
	int ret;
	struct syna_clk_priv *priv = dev_get_priv(dev);
	u32 src = 0, div = 0, freq = 0;

	priv->base = dev_read_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE) {
		debug("%s: Failed to find address (err=%d\n)", __func__, ret);
		return -EINVAL;
	}

	ret = dev_read_u32(dev, "clk_src", &src);
	if (ret) {
		debug("%s: Failed to get clock src (err=%d\n)", __func__, ret);;
		return -EINVAL;
	}

	priv->src = src;

	ret = dev_read_u32(dev, "clk_div", &div);
	if (ret) {
		debug("%s: Failed to get clock div (err=%d\n)", __func__, ret);;
		return -EINVAL;
	}

	if(convert_div(div) < 0) {
		debug("%s: Failed to validate clock div (err=%d\n)", __func__, ret);;
		return -EINVAL;
	}

	priv->div = div;

	ret = dev_read_u32(dev, "clk_freq", &freq);
	if (ret) {
		debug("%s: Failed to get clock frequency (err=%d\n)", __func__, ret);;
	}

	priv->freq = freq;

	//FIXME: done in module refer to it or simply finish the setting in probe

	return 0;
}

struct clk_ops syna_clk_ops = {
	.get_rate	= syna_clk_get_rate,
	.set_rate	= syna_clk_set_rate,
	.enable		= syna_clk_enable,
	.disable	= syna_clk_disable,
};

static const struct udevice_id syna_clk_ids[] = {
	{ .compatible = "synaptics,generic-clock" },
	{ }
};

U_BOOT_DRIVER(clk_syna) = {
	.name		= "syna_clk",
	.id			= UCLASS_CLK,
	.of_match	= syna_clk_ids,
	.priv_auto_alloc_size	= sizeof(struct syna_clk_priv),
	.ops		= &syna_clk_ops,
	.probe		= syna_clk_probe,
};
