/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2016 Marvell Technology Group Ltd.
 *
 * Author: Xiaoming Lu <xmlu@marvell.com>

 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */

#include <common.h>
#include <dm.h>
#include <reset-uclass.h>
#include <asm/io.h>

#define BERLIN_MAX_RESETS	32

struct syna_reset_priv {
	fdt_addr_t	base;
};

static int syna_reset_of_xlate(struct reset_ctl *reset_ctl,
				  struct ofnode_phandle_args *args)
{
	if (args->args_count != 4) {
		debug("Invaild args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	/* Use .data field as register offset and .id field as bit shift */
	reset_ctl->data = args->args[0];
	reset_ctl->id = args->args[1];
	reset_ctl->sticky = args->args[2];
	reset_ctl->polarity = args->args[3];

	if (reset_ctl->id >= BERLIN_MAX_RESETS)
		return -EINVAL;

	return 0;
}

static int syna_reset_request(struct reset_ctl *reset_ctl)
{
	return 0;
}

static int syna_reset_free(struct reset_ctl *reset_ctl)
{
	return 0;
}

static int syna_reset_assert(struct reset_ctl *reset_ctl)
{
	struct syna_reset_priv *priv = dev_get_priv(reset_ctl->dev);
	u32 reg = 0;

	if(reset_ctl->sticky)
		reg = readl(priv->base + reset_ctl->data);
	debug("read:  %016llx %08x\n", priv->base + reset_ctl->data, reg);
	if(reset_ctl->polarity) {
		reg |= 1 << reset_ctl->id;
	} else {
		reg &= ~(1 << reset_ctl->id);
	}
	writel(reg, priv->base + reset_ctl->data);
	debug("write: %016llx %08x\n", priv->base + reset_ctl->data, reg);

	return 0;
}

static int syna_reset_deassert(struct reset_ctl *reset_ctl)
{
	struct syna_reset_priv *priv = dev_get_priv(reset_ctl->dev);
	u32 reg = 0;

	if(!reset_ctl->sticky)
		return 0;

	reg = readl(priv->base + reset_ctl->data);
	debug("read:  %016llx %08x\n", priv->base + reset_ctl->data, reg);
	if(reset_ctl->polarity) {
		reg &= ~(1 << reset_ctl->id);
	} else {
		reg |= 1 << reset_ctl->id;
	}
	writel(reg, priv->base + reset_ctl->data);
	debug("write: %016llx %08x\n", priv->base + reset_ctl->data, reg);

	return 0;
}

static int syna_reset_probe(struct udevice *dev)
{
	struct syna_reset_priv *priv = dev_get_priv(dev);
	priv->base = dev_read_addr(dev);

	debug("%s(dev=%p) (base=%llx)\n", __func__, dev, priv->base);

	return 0;
}

static const struct udevice_id syna_reset_ids[] = {
	{ .compatible = "synaptics,chipctrl-reset" },
	{ }
};

struct reset_ops syna_reset_ops = {
	.of_xlate = syna_reset_of_xlate,
	.request = syna_reset_request,
	.free = syna_reset_free,
	.rst_assert = syna_reset_assert,
	.rst_deassert = syna_reset_deassert,
};

U_BOOT_DRIVER(syna_chipctrl_reset) = {
	.name		= "syna_chipctrl_reset",
	.id			= UCLASS_RESET,
	.of_match	= syna_reset_ids,
	.probe		= syna_reset_probe,
	.ops		= &syna_reset_ops,
	.priv_auto_alloc_size = sizeof(struct syna_reset_priv),
};
