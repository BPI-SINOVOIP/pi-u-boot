// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Benson Gui <begu@synaptics.com>
 * Cheng Lu <Cheng.Lu@synaptics.com>
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <reset.h>
#include <asm/io.h>
#include <generic-phy.h>
#include "usb_phy_syna.h"

#define VS680_USB_PHY_CTRL0		0x0
#define VS680_USB_PHY_CTRL1		0x4
#define VS680_PHY_CTRL0_DEF		0x533DADF0
#define VS680_PHY_CTRL1_DEF		0x01B10000

static int phy_vs680_usb2_init(struct phy *phy)
{
	int ret;
	struct udevice *dev = phy->dev;
	struct phy_syna_usb_priv *priv = dev_get_priv(dev);

	ret = reset_assert_bulk(&priv->rsts);
	if (ret) {
		reset_release_bulk(&priv->rsts);
		dev_err(dev, "Failed to reset: %d\n", ret);
		return ret;
	}
	udelay(1);

	return 0;
}

static int phy_vs680_usb2_power_on(struct phy *phy)
{
	struct phy_syna_usb_priv *priv = dev_get_priv(phy->dev);
	int ret;

	/* setup USB_PHY_CTRL0 */
	writel(VS680_PHY_CTRL0_DEF, priv->base + VS680_USB_PHY_CTRL0);
	/* setup USB_PHY_CTRL1 */
	writel(VS680_PHY_CTRL1_DEF, priv->base + VS680_USB_PHY_CTRL1);

	ret = reset_deassert_bulk(&priv->rsts);
	if (ret) {
		reset_release_bulk(&priv->rsts);
		dev_err(phy->dev, "Failed to reset: %d\n", ret);
		return ret;
	}
	//must have a delay here, followed by usb0CoreRstn
	mdelay(1);
	return 0;
}

static const struct phy_ops phy_vs680_usb2_ops = {
	.init		= phy_vs680_usb2_init,
	.power_on	= phy_vs680_usb2_power_on,
};

#define USB3_PHY_CLK_CTRL		0x001C
#define REF_SSP_EN			BIT(16)

static int phy_vs680_usb3_init(struct phy *phy)
{
	struct phy_syna_usb_priv *priv = dev_get_priv(phy->dev);
	int ret;

	ret = reset_deassert_bulk(&priv->rsts);
	if (ret) {
		reset_release_bulk(&priv->rsts);
		dev_err(phy->dev, "Failed to reset: %d\n", ret);
		return ret;
	}
	udelay(1);
	return 0;
}

static int phy_vs680_usb3_power_on(struct phy *phy)
{
	struct phy_syna_usb_priv *priv = dev_get_priv(phy->dev);
	u32 val;
	int ret;

	val = readl(priv->base + USB3_PHY_CLK_CTRL);
	val |= REF_SSP_EN;
	writel(val, priv->base + USB3_PHY_CLK_CTRL);
	udelay(1);

	ret = reset_deassert_bulk(&priv->rsts);
	if (ret) {
		reset_release_bulk(&priv->rsts);
		dev_err(phy->dev, "Failed to reset: %d\n", ret);
		return ret;
	}
	udelay(10);
	return 0;
}

static const struct phy_ops phy_vs680_usb3_ops = {
	.init		= phy_vs680_usb3_init,
	.power_on	= phy_vs680_usb3_power_on,
};

static int vs680_usb_phy_probe(struct udevice *dev)
{
	struct phy_ops *phy_ops = (struct phy_ops *)dev_get_driver_data(dev);
	return syna_usb_phy_probe(dev, phy_ops);
}

static const struct udevice_id vs680_phy_of_match[] = {

	{
		.compatible = "syna,vs680-usb2-phy",
		.data = (ulong)&phy_vs680_usb2_ops,
	},
	{
		.compatible = "syna,vs680-usb3-phy",
		.data = (ulong)&phy_vs680_usb3_ops,
	},
	{ },
};

U_BOOT_DRIVER(syna_usb_phy) = {
	.name = "syna_usb_phy",
	.id = UCLASS_PHY,
	.of_match = vs680_phy_of_match,
	.probe = vs680_usb_phy_probe,
	.ops = &syna_usb_phy_ops,
	.priv_auto_alloc_size = sizeof(struct phy_syna_usb_priv),
};

