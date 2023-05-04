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

struct syna_gate_priv {
	fdt_addr_t base;
	u32 gate_bit;
};

static int syna_clk_of_xlate(struct clk *clk,
				struct ofnode_phandle_args *args)
{
	struct syna_gate_priv *priv = dev_get_priv(clk->dev);
	debug("%s(clk=%p)\n", __func__, clk);

	if (args->args_count != 1) {
		debug("Invaild args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	priv->gate_bit = args->args[0];

	return 0;
}

static int syna_gate_enable(struct clk *clk)
{
	struct syna_gate_priv *priv = dev_get_priv(clk->dev);
	u32 reg = 0;

	reg = readl((void *)(priv->base));
	reg |= 1 << (priv->gate_bit);
	writel(reg, (void *)(priv->base));

	return 0;
}

static int syna_gate_disable(struct clk *clk)
{
	struct syna_gate_priv *priv = dev_get_priv(clk->dev);
	u32 reg = 0;

	reg = readl((void *)(priv->base));
	reg &= ~(1 << (priv->gate_bit));
	writel(reg, (void *)(priv->base));

	return 0;
}

int syna_gate_probe(struct udevice *dev)
{
	int ret;
	struct syna_gate_priv *priv = dev_get_priv(dev);

	//get base
	priv->base = dev_read_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE) {
		debug("%s: Failed to find address (err=%d\n)", __func__, ret);
		return -EINVAL;
	}

	return 0;
}

struct clk_ops syna_gate_ops = {
	.of_xlate	= syna_clk_of_xlate,
	.enable		= syna_gate_enable,
	.disable	= syna_gate_disable,
};

static const struct udevice_id syna_gate_ids[] = {
	{ .compatible = "synaptics,generic-gate" },
	{ }
};

U_BOOT_DRIVER(gate_syna) = {
	.name		= "syna_gate",
	.id			= UCLASS_CLK,
	.of_match	= syna_gate_ids,
	.priv_auto_alloc_size	= sizeof(struct syna_gate_priv),
	.ops		= &syna_gate_ops,
	.probe		= syna_gate_probe,
};
