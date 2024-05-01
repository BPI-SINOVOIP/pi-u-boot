// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * USB2.0 Phy for Spacemit k1x SoCs
 *
 * Copyright (c) 2023 Spacemit Inc.
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <generic-phy.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>

#define PHY_28LP	0x2800
#define PHY_40LP	0x4000
#define PHY_55LP	0x5500

#define MV_PHY_FLAG_PLL_LOCK_BYPASS	(1 << 0)

#define USB2_PHY_REG01			0x4
#define USB2_PHY_REG01_PLL_IS_READY	(0x1 << 0)
#define USB2_PHY_REG04			0x10
#define USB2_PHY_REG04_EN_HSTSOF	(0x1 << 0)
#define USB2_PHY_REG04_AUTO_CLEAR_DIS	(0x1 << 2)
#define USB2_PHY_REG08			0x20
#define USB2_PHY_REG08_DISCON_DET	(0x1 << 9)
#define USB2_PHY_REG0D			0x34
#define USB2_PHY_REG26			0x98
#define USB2_PHY_REG22			0x88
#define USB2_CFG_FORCE_CDRCLK		(0x1 << 6)
#define USB2_PHY_REG06			0x18
#define USB2_CFG_HS_SRC_SEL		(0x1 << 0)

#define USB2D_CTRL_RESET_TIME_MS	50

struct mv_usb_phy_priv {
	void __iomem		*phy_base;
};

static int mv_usb_phy_init(struct phy *phy)
{
	struct mv_usb_phy_priv *priv = dev_get_priv(phy->dev);
	void __iomem *base = priv->phy_base;
	uint32_t loops, temp;

	// make sure the usb controller is not under reset process before any configuration
	udelay(50);
	writel(0xbec4, base + USB2_PHY_REG26); //24M ref clk
	udelay(150);

	loops = USB2D_CTRL_RESET_TIME_MS * 1000;

	//wait for usb2 phy PLL ready
	do {
		temp = readl(base + USB2_PHY_REG01);
		if (temp & USB2_PHY_REG01_PLL_IS_READY)
			break;
		udelay(50);
	} while(--loops);

	if (loops == 0)
		dev_err(phy->dev, "Wait PHY_REG01[PLLREADY] timeout\n");

	//release usb2 phy internal reset and enable clock gating
	writel(0x60ef, base + USB2_PHY_REG01);
	writel(0x1c, base + USB2_PHY_REG0D);

	//select HS parallel data path
	temp = readl(base + USB2_PHY_REG06);
	// temp |= USB2_CFG_HS_SRC_SEL;
	temp &= ~(USB2_CFG_HS_SRC_SEL);
	writel(temp, base + USB2_PHY_REG06);

	/* auto clear host disc*/
	temp = readl(base + USB2_PHY_REG04);
	temp |= USB2_PHY_REG04_AUTO_CLEAR_DIS;
	writel(temp, base + USB2_PHY_REG04);

	return 0;
}

static int mv_usb_phy_exit(struct phy *phy)
{
	return 0;
}

static int mv_usb_phy_power_on(struct phy *phy)
{
	return 0;
}

static int mv_usb_phy_power_off(struct phy *phy)
{
	return 0;
}

static int mv_usb_phy_probe(struct udevice *dev)
{
	struct mv_usb_phy_priv *priv = dev_get_priv(dev);
	dev_info(dev, "k1x-ci-usb-phy-probe: Enter...\n");
	priv->phy_base = (void*)dev_read_addr(dev);
	if (priv->phy_base == NULL){
		dev_err(dev, "cannot get phy base addr");
		return -ENODEV;
	}
	return 0;
}

static const struct udevice_id mv_usb_phy_ids[] = {
	{.compatible = "spacemit,usb2-phy",},
	{ /* sentinel */ }
};

static struct phy_ops mv_usb_phy_ops = {
	.init = mv_usb_phy_init,
	.exit = mv_usb_phy_exit,
	.power_on = mv_usb_phy_power_on,
	.power_off = mv_usb_phy_power_off,
};

U_BOOT_DRIVER(mv_usb_phy) = {
	.name	= "mv_usb_phy",
	.id	= UCLASS_PHY,
	.of_match = mv_usb_phy_ids,
	.ops = &mv_usb_phy_ops,
	.probe = mv_usb_phy_probe,
	.priv_auto	= sizeof(struct mv_usb_phy_priv),
};
