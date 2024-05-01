// SPDX-License-Identifier: GPL-2.0
/*
 * spacemit-onboard-hub.c - Spacemit k1-x onboard usb hub
 *
 * Copyright (c) 2023 Spacemit Co., Ltd.
 *
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <generic-phy.h>
#include <dt-bindings/phy/phy.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <asm/gpio.h>

#define GPIOD_OUT_HIGH (GPIOD_IS_OUT | GPIOD_PULL_UP)

struct spacemit_hub_priv {
	struct gpio_desc *gpio_usb;
	struct gpio_desc *gpio_hub;
	struct gpio_desc *gpio_reset;
};

static int spacemit_hub_probe(struct udevice *dev)
{
	struct spacemit_hub_priv *spacemit;

	spacemit = dev_get_priv(dev);
	if (!spacemit)
		return -ENOMEM;

	spacemit->gpio_usb = devm_gpiod_get(dev, "usb", GPIOD_OUT_HIGH);
	if (IS_ERR_OR_NULL(spacemit->gpio_usb)) {
		dev_err(dev, "can not find usb-gpio\n");
		return -ENODEV;
	}
	dm_gpio_set_value(spacemit->gpio_usb, 1);

	spacemit->gpio_hub = devm_gpiod_get(dev, "hub", GPIOD_OUT_HIGH);
	if (IS_ERR_OR_NULL(spacemit->gpio_hub)) {
		dev_err(dev, "can not find hub-gpio\n");
		return -ENODEV;
	}
	dm_gpio_set_value(spacemit->gpio_hub, 1);

	spacemit->gpio_reset= devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR_OR_NULL(spacemit->gpio_reset)) {
		dev_err(dev, "can not find reset-gpio\n");
		return -ENODEV;
	}
	dm_gpio_set_value(spacemit->gpio_reset, 1);

	dev_info(dev, "onboard usb hub driver probe, hub configured\n");

	return 0;
}

static int spacemit_hub_remove(struct udevice *dev)
{
	struct spacemit_hub_priv *spacemit =  dev_get_priv(dev);

	dm_gpio_set_value(spacemit->gpio_usb, 0);
	dm_gpio_set_value(spacemit->gpio_reset, 0);
	dm_gpio_set_value(spacemit->gpio_reset, 0);

	dev_info(dev, "onboard usb hub driver exit, disable hub\n");
	return 0;
}

static int spacemit_hub_bind(struct udevice *dev)
{
	dev_or_flags(dev, DM_FLAG_PROBE_AFTER_BIND);
	return 0;
}

static const struct udevice_id spacemit_hub_ids[] = {
	{.compatible = "spacemit,usb3-hub",},
	{ /* sentinel */ }
};

U_BOOT_DRIVER(spacemit_onboard_hub) = {
	.name	= "spacemit_onboard_hub",
	.id	= UCLASS_MISC,
	.of_match = spacemit_hub_ids,
	.bind = spacemit_hub_bind,
	.probe = spacemit_hub_probe,
	.remove = spacemit_hub_remove,
	.priv_auto	= sizeof(struct spacemit_hub_priv),
};
