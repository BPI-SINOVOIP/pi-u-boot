/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 */

#include <common.h>
#include <dm.h>
#include <generic-phy.h>
#include <usb.h>
#include <reset.h>
#include <clk.h>
#include <power/regulator.h>
#include <asm-generic/gpio.h>

struct dwc3_syna_priv {
	struct udevice		*dev;
	struct phy		phy;
	struct clk_bulk		clks;
	struct reset_ctl_bulk		rsts;
#if CONFIG_IS_ENABLED(DM_REGULATOR)
	struct udevice		*vbus_supply;
#ifdef CONFIG_DM_GPIO
	struct gpio_desc reset_gpio;
#endif
#endif
};

static int dwc3_syna_reset_init(struct dwc3_syna_priv *priv)
{
	int ret;

	ret = reset_get_bulk(priv->dev, &priv->rsts);
	if (ret) {
		dev_warn(priv->dev, "Can't get reset: %d\n", ret);
		/* Return 0 if error due to !CONFIG_DM_RESET and reset
		 * DT property is not present.
		 */
		if (ret == -ENOENT || ret == -ENOTSUPP)
			return 0;
		else
			return ret;
	}

	ret = reset_assert_bulk(&priv->rsts);
	if (ret) {
		reset_release_bulk(&priv->rsts);
		dev_err(priv->dev, "Failed to reset: %d\n", ret);
		return ret;
	}

	return 0;
}

static int dwc3_clock_init(struct dwc3_syna_priv *priv)
{
	int ret;

	ret = clk_get_bulk(priv->dev, &priv->clks);
	if (ret == -ENOSYS)
		return 0;
	if (ret)
		return ret;

	ret = clk_enable_bulk(&priv->clks);
	if (ret) {
		clk_release_bulk(&priv->clks);
		return ret;
	}

	return 0;
}

static int dwc3_syna_probe(struct udevice *dev)
{
	struct dwc3_syna_priv *priv = dev_get_priv(dev);
	int ret;
#ifdef CONFIG_DM_GPIO
	int reset_flags = GPIOD_IS_OUT;
	int reset_val = 0;
#endif

	priv->dev = dev;
	ret = dwc3_clock_init(priv);
	if(ret)
		return ret;

#ifdef CONFIG_DM_GPIO
	/* For usb2.0 hub chip reset */
	ret = gpio_request_by_name(dev, "snps,reset-gpio", 0,
		&priv->reset_gpio, reset_flags);
	if(!ret) {
		if (dev_read_bool(dev, "snps,reset-active-high"))
			reset_val = 1;
		dm_gpio_set_value(&priv->reset_gpio, reset_val);
	}
#endif

	ret = dwc3_syna_reset_init(priv);
	if (ret)
		return ret;

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	ret = device_get_supply_regulator(priv->dev, "vbus-supply",
					  &priv->vbus_supply);
	if (ret && ret != -ENOENT) {
		pr_err("Failed to get PHY regulator\n");
		return ret;
	}

	if (priv->vbus_supply) {
		ret = regulator_set_enable(priv->vbus_supply, true);
		if (ret)
			return ret;
	}
#endif

	return 0;
}

static const struct udevice_id dwc3_syna_ids[] = {
	{ .compatible = "syna,vs680-dwusb3" },
	{ }
};

U_BOOT_DRIVER(dwc3_syna_driver) = {
	.name	= "dwc3-syna",
	.id	= UCLASS_SIMPLE_BUS,
	.of_match = dwc3_syna_ids,
	.probe = dwc3_syna_probe,
	.priv_auto_alloc_size = sizeof(struct dwc3_syna_priv),
};
