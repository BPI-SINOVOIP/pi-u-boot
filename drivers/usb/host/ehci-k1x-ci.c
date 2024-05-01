// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * EHCI Driver Wrapper for Spacemit k1x SoCs
 *
 * Copyright (c) 2023 Spacemit Inc.
 */

#include <common.h>
#include <clk.h>
#include <log.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <dm/ofnode.h>
#include <reset.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <dm.h>
#include "ehci.h"
#include <stdbool.h>
#include <power/regulator.h>

/* PMU */
#define PMU_SD_ROT_WAKE_CLR				0x7C
#define PMU_SD_ROT_WAKE_CLR_VBUS_DRV	(0x1 << 21)

struct ehci_mv_priv {
	struct ehci_ctrl ctrl;
	struct clk_bulk clocks;
	struct reset_ctl_bulk resets;
	struct phy phy;
	void __iomem *apmu_base;
	struct udevice *vbus_supply;
};

static int mv_ehci_enable_vbus_supply(struct udevice *dev, bool enable)
{
	struct ehci_mv_priv *priv = dev_get_priv(dev);
	if (priv->vbus_supply)
		return regulator_set_enable(priv->vbus_supply, enable);
	else
		return 0;
}

static void mv_ehci_enable(struct udevice *dev, bool enable)
{
	struct ehci_mv_priv *priv = dev_get_priv(dev);
	uint32_t temp;

	temp = readl(priv->apmu_base + PMU_SD_ROT_WAKE_CLR);
	if (enable)
		writel(PMU_SD_ROT_WAKE_CLR_VBUS_DRV | temp, priv->apmu_base + PMU_SD_ROT_WAKE_CLR);
	else
		writel(temp & ~PMU_SD_ROT_WAKE_CLR_VBUS_DRV , priv->apmu_base + PMU_SD_ROT_WAKE_CLR);
}

static int ehci_mv_probe(struct udevice *dev)
{
	struct ehci_mv_priv *priv = dev_get_priv(dev);
	struct ehci_hccr *hccr;
	struct ehci_hcor *hcor;
	ofnode apmu_node;

	int err, ret;
	err = 0;

	dev_info(dev, "ehci_mv_probe Enter ... \n");

	apmu_node = ofnode_by_compatible(ofnode_null(), "spacemit,k1x-pm-domain");
	if (ofnode_valid(apmu_node)) {
		priv->apmu_base = (void*)ofnode_get_addr_index(apmu_node, 1);
	} else {
		dev_err(dev, "Cannot find apmu node, check dts!\n");
		return -ENOENT;
	}

	ret = clk_get_bulk(dev, &priv->clocks);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "Failed to get clocks (ret=%d)\n", ret);
		return ret;
	}

	err = clk_enable_bulk(&priv->clocks);
	if (err) {
		dev_err(dev, "Failed to enable clocks (err=%d)\n", err);
		goto err_clk;
	}

	err = reset_get_bulk(dev, &priv->resets);
	if (err && err != -ENOENT) {
		dev_err(dev, "Failed to get resets (err=%d)\n", err);
		goto err_clk;
	}

	err = reset_deassert_bulk(&priv->resets);
	if (err) {
		dev_err(dev, "Failed to get deassert resets (err=%d)\n", err);
		goto err_reset;
	}

	err = ehci_setup_phy(dev, &priv->phy, 0);
	if (err)
		goto err_reset;

	mv_ehci_enable(dev, true);

	hccr = (struct ehci_hccr *)dev_read_addr(dev);
	hcor = (struct ehci_hcor *)((uintptr_t)hccr +
					HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	dev_info(dev, "ehci-k1x-ci: init hccr %lx and hcor %lx hc_length %ld\n",
		  (uintptr_t)hccr, (uintptr_t)hcor, (uintptr_t)HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	err = device_get_supply_regulator(dev, "vbus-supply",
					  &priv->vbus_supply);
	if (err && err != -ENOENT) {
		dev_err(dev, "Failed to retrieve vbus-supply regulator, (err=%d)", err);
		goto err_vbus;
	}
	err = mv_ehci_enable_vbus_supply(dev, true);
	if (err) {
		dev_err(dev, "Failed to enable vbus (err=%d)\n", err);
		priv->vbus_supply = NULL;
		goto err_vbus;
	}

	err = ehci_register(dev, hccr, hcor, NULL, 0, USB_INIT_HOST);
	if (err)
		goto err_vbus;

	return 0;

err_vbus:
	mv_ehci_enable_vbus_supply(dev, false);
	mv_ehci_enable(dev, false);
	ehci_shutdown_phy(dev, &priv->phy);
err_reset:
	reset_release_bulk(&priv->resets);
err_clk:
	clk_release_bulk(&priv->clocks);
	return err;
}

static int ehci_usb_remove(struct udevice *dev)
{
	struct ehci_mv_priv *priv = dev_get_priv(dev);
	int ret;

	ret = ehci_deregister(dev);
	if (ret)
		return ret;

	mv_ehci_enable_vbus_supply(dev, false);
	mv_ehci_enable(dev, false);

	ret = ehci_shutdown_phy(dev, &priv->phy);
	if (ret)
		return ret;

	ret = reset_release_bulk(&priv->resets);
	if (ret)
		return ret;

	return clk_release_bulk(&priv->clocks);
}

static const struct udevice_id ehci_mv_ids[] = {
	{ .compatible = "spacemit,mv-ehci" },
	{ }
};

U_BOOT_DRIVER(ehci_generic) = {
	.name	= "ehci_k1x_ci",
	.id	= UCLASS_USB,
	.of_match = ehci_mv_ids,
	.probe = ehci_mv_probe,
	.remove = ehci_usb_remove,
	.ops	= &ehci_usb_ops,
	.priv_auto	= sizeof(struct ehci_mv_priv),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
