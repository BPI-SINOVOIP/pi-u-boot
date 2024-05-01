// SPDX-License-Identifier: GPL-2.0
/*
 * phy-spacemit-k1x-combphy.c - Spacemit k1-x combo PHY for USB3 and PCIE
 *
 * Copyright (c) 2023 Spacemit Co., Ltd.
 *
 */
#include <common.h>
#include <reset.h>
#include <clk.h>
#include <dm.h>
#include <generic-phy.h>
#include <dt-bindings/phy/phy.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>


#define SPACEMIT_COMBPHY_WAIT_TIMEOUT 1000
#define SPACEMIT_COMBPHY_MODE_SEL BIT(3)

// Registers for USB3 PHY
#define SPACEMIT_COMBPHY_USB_REG1 0x68
#define SPACEMIT_COMBPHY_USB_REG1_VAL 0x0
#define SPACEMIT_COMBPHY_USB_REG2 (0x12 << 2)
#define SPACEMIT_COMBPHY_USB_REG2_VAL 0x603a2276
#define SPACEMIT_COMBPHY_USB_REG3 (0x02 << 2)
#define SPACEMIT_COMBPHY_USB_REG3_VAL 0x97c
#define SPACEMIT_COMBPHY_USB_REG4 (0x06 << 2)
#define SPACEMIT_COMBPHY_USB_REG4_VAL 0x0
#define SPACEMIT_COMBPHY_USB_PLL_REG 0x8
#define SPACEMIT_COMBPHY_USB_PLL_MASK 0x1
#define SPACEMIT_COMBPHY_USB_PLL_VAL 0x1

struct spacemit_combphy_priv {
	struct udevice *dev;
	struct reset_ctl_bulk phy_rst;
	void __iomem *phy_sel;
	void __iomem *puphy_base;
	struct phy *phy;
	u8 type;
};

static inline void spacemit_reg_updatel(void __iomem *reg, u32 offset, u32 mask, u32 val)
{
	u32 tmp;
	tmp = readl(reg + offset);
	tmp = (tmp & ~(mask)) | val;
	writel(tmp, reg + offset);
}

static int spacemit_combphy_wait_ready(struct spacemit_combphy_priv *priv,
					   u32 offset, u32 mask, u32 val)
{
	int timeout = SPACEMIT_COMBPHY_WAIT_TIMEOUT;
	while (((readl(priv->puphy_base + offset) & mask) != val) && --timeout)
		;
	if (!timeout) {
		return -ETIMEDOUT;
	}
	dev_dbg(priv->dev, "phy init timeout remain: %d", timeout);
	return 0;
}

static int spacemit_combphy_set_mode(struct spacemit_combphy_priv *priv)
{
	u8 mode = priv->type;
	if (mode == PHY_TYPE_USB3) {
		spacemit_reg_updatel(priv->phy_sel, 0, 0,
					 SPACEMIT_COMBPHY_MODE_SEL);
	} else {
		dev_err(priv->dev, "PHY type %x not supported\n", mode);
		return -EINVAL;
	}
	return 0;
}

static int spacemit_combphy_init_usb(struct spacemit_combphy_priv *priv)
{
	int ret;
	void __iomem *base = priv->puphy_base;
	dev_info(priv->dev, "USB3 PHY init.\n");

	writel(SPACEMIT_COMBPHY_USB_REG1_VAL, base + SPACEMIT_COMBPHY_USB_REG1);
	writel(SPACEMIT_COMBPHY_USB_REG2_VAL, base + SPACEMIT_COMBPHY_USB_REG2);
	writel(SPACEMIT_COMBPHY_USB_REG3_VAL, base + SPACEMIT_COMBPHY_USB_REG3);
	writel(SPACEMIT_COMBPHY_USB_REG4_VAL, base + SPACEMIT_COMBPHY_USB_REG4);

	ret = spacemit_combphy_wait_ready(priv, SPACEMIT_COMBPHY_USB_PLL_REG,
					  SPACEMIT_COMBPHY_USB_PLL_MASK,
					  SPACEMIT_COMBPHY_USB_PLL_VAL);

	if (ret)
		dev_err(priv->dev, "USB3 PHY init timeout!\n");

	return ret;
}


static int spacemit_combphy_init(struct phy *phy)
{
	struct spacemit_combphy_priv *priv = dev_get_priv(phy->dev);
	int ret;

	ret = spacemit_combphy_set_mode(priv);

	if (ret) {
		dev_err(priv->dev, "failed to set mode for PHY type %x\n",
			priv->type);
		goto out;
	}

	ret = reset_deassert_bulk(&priv->phy_rst);
	if (ret) {
		dev_err(priv->dev, "failed to deassert rst\n");
		goto out;
	}

	switch (priv->type) {
	case PHY_TYPE_USB3:
		ret = spacemit_combphy_init_usb(priv);
		break;
	default:
		dev_err(priv->dev, "PHY type %x not supported\n", priv->type);
		ret = -EINVAL;
		break;
	}

	if (ret)
		goto err_rst;

	return 0;

err_rst:
	reset_assert_bulk(&priv->phy_rst);
out:
	return ret;
}

static int spacemit_combphy_exit(struct phy *phy)
{
	struct spacemit_combphy_priv *priv =  dev_get_priv(phy->dev);

	reset_assert_bulk(&priv->phy_rst);

	return 0;
}

static int spacemit_combphy_xlate(struct phy *phy, struct ofnode_phandle_args *args)
{
	struct spacemit_combphy_priv *priv =  dev_get_priv(phy->dev);

	if (args->args_count != 1) {
		dev_err(phy->dev, "invalid number of arguments\n");
		return -EINVAL;
	}

	if (priv->type != PHY_NONE && priv->type != args->args[0])
		dev_warn(phy->dev, "PHY type %d is selected to override %d\n",
			 args->args[0], priv->type);

	priv->type = args->args[0];

	return 0;
}

static int spacemit_combphy_probe(struct udevice *dev)
{
	struct spacemit_combphy_priv *priv = dev_get_priv(dev);
	int ret;

	if (!priv)
		return -ENOMEM;

	priv->puphy_base = (void*)dev_read_addr_name(dev, "puphy");
	if (IS_ERR(priv->puphy_base)) {
		ret = PTR_ERR(priv->puphy_base);
		return ret;
	}

	priv->phy_sel = (void*)dev_read_addr_name(dev, "phy_sel");
	if (IS_ERR(priv->phy_sel)) {
		ret = PTR_ERR(priv->phy_sel);
		return ret;
	}

	priv->dev = dev;
	priv->type = PHY_NONE;

	ret = reset_get_bulk(dev, &priv->phy_rst);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "Failed to get resets (err=%d)\n", ret);
		return ret;
	}

	ret = reset_assert_bulk(&priv->phy_rst);
	if (ret) {
		dev_err(dev, "failed to reset phy\n");
		return ret;
	}

	return 0;
}

static const struct udevice_id spacemit_combphy_ids[] = {
	{.compatible = "spacemit,k1x-combphy",},
	{ /* sentinel */ }
};

static struct phy_ops spacemit_combphy_ops = {
	.init = spacemit_combphy_init,
	.exit = spacemit_combphy_exit,
	.of_xlate = spacemit_combphy_xlate,
};

U_BOOT_DRIVER(k1x_combphy) = {
	.name	= "k1x_combphy",
	.id	= UCLASS_PHY,
	.of_match = spacemit_combphy_ids,
	.ops = &spacemit_combphy_ops,
	.probe = spacemit_combphy_probe,
	.priv_auto	= sizeof(struct spacemit_combphy_priv),
};