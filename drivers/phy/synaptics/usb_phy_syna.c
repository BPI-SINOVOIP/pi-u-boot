// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Cheng Lu <Cheng.Lu@synaptics.com>
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <reset.h>
#include <asm/io.h>
#include <generic-phy.h>
#include "usb_phy_syna.h"

static int syna_usb_phy_init(struct phy *phy)
{
	struct phy_syna_usb_priv *priv = dev_get_priv(phy->dev);
	return priv->ops->init(phy);
}

static int syna_usb_phy_power_on(struct phy *phy)
{
	struct phy_syna_usb_priv *priv = dev_get_priv(phy->dev);
	return priv->ops->power_on(phy);
}

const struct phy_ops syna_usb_phy_ops = {
	.init		= syna_usb_phy_init,
	.power_on	= syna_usb_phy_power_on,
};

int syna_usb_phy_probe(struct udevice *dev, struct phy_ops *desc)
{
	struct phy_syna_usb_priv *priv = dev_get_priv(dev);
	int ret;

	priv->base = dev_read_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE) {
		debug("%s: Failed to find address (err=%d\n)", __func__, ret);
		return -EINVAL;
	}

	priv->dev = dev;
	priv->ops = desc;
	ret = reset_get_bulk(dev, &priv->rsts);
	if (ret) {
		dev_warn(dev, "Can't get reset: %d\n", ret);
		/* Return 0 if error due to !CONFIG_DM_RESET and reset
		 * DT property is not present.
		 */
		if (ret == -ENOENT || ret == -ENOTSUPP)
			return 0;
		else
			return ret;
	}

	return 0;
}

