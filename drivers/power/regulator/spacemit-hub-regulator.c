// SPDX-License-Identifier: GPL-2.0
/*
 * spacemit-hubpwr-regulator.c -
 * 		Regulator Wrapper for Spacemit k1-x onboard usb hub
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
#include <power/regulator.h>

#define GPIOD_OUT_HIGH (GPIOD_IS_OUT | GPIOD_PULL_UP)
#define GPIOD_OUT_LOW (GPIOD_IS_OUT | GPIOD_PULL_DOWN)
#define GPIO_MAX_COUNT 4

struct spacemit_hub_regulator_plat {
	struct udevice *dev;
	bool is_on;
	struct gpio_desc vbus_gpios[GPIO_MAX_COUNT];
	struct gpio_desc hub_gpios[GPIO_MAX_COUNT];
	u8 vbus_gpio_cnt;
	u8 hub_gpio_cnt;
	bool hub_gpio_active_low;
	bool vbus_gpio_active_low;
	struct udevice *vbus_regulator;
	struct udevice *hub_regulator;
	u32 hub_inter_delay_ms;
	u32 vbus_delay_ms;
	u32 vbus_inter_delay_ms;
};

static int spacemit_hub_enable(struct spacemit_hub_regulator_plat *spacemit, bool on)
{
	unsigned i;
	int ret = 0;
	int active_val = spacemit->hub_gpio_active_low ? 0 : 1;

	if (!spacemit->hub_gpio_cnt && !spacemit->hub_regulator)
		return 0;
	dev_dbg(spacemit->dev, "do hub enable %s\n", on ? "on" : "off");
	if (spacemit->hub_regulator) {
		ret = regulator_set_enable(spacemit->hub_regulator, on);
		if (ret)
			return ret;
	}
	if (on) {
		for (i = 0; i < spacemit->hub_gpio_cnt; i++) {
			ret = dm_gpio_set_value(&spacemit->hub_gpios[i],
					active_val);
			if (ret)
				return ret;
			if (spacemit->hub_inter_delay_ms) {
				mdelay(spacemit->hub_inter_delay_ms);
			}
		}
	} else {
		for (i = spacemit->hub_gpio_cnt; i > 0; --i) {
			ret = dm_gpio_set_value(&spacemit->hub_gpios[i - 1],
					!active_val);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int spacemit_hub_vbus_enable(struct spacemit_hub_regulator_plat *spacemit,
					 bool on)
{
	unsigned i;
	int ret = 0;
	int active_val = spacemit->vbus_gpio_active_low ? 0 : 1;

	if (!spacemit->vbus_gpio_cnt && !spacemit->vbus_regulator)
		return 0;
	dev_dbg(spacemit->dev, "do hub vbus on %s\n", on ? "on" : "off");
	if (spacemit->vbus_regulator) {
		regulator_set_enable(spacemit->vbus_regulator, on);
		if (ret)
			return ret;
	}
	if (on) {
		for (i = 0; i < spacemit->vbus_gpio_cnt; i++) {
			ret = dm_gpio_set_value(&spacemit->vbus_gpios[i],
					active_val);
			if (ret)
				return ret;
			if (spacemit->vbus_inter_delay_ms) {
				mdelay(spacemit->vbus_inter_delay_ms);
			}
		}
	} else {
		for (i = spacemit->vbus_gpio_cnt; i > 0; --i) {
			ret = dm_gpio_set_value(&spacemit->vbus_gpios[i - 1],
					!active_val);
			if (ret)
				return ret;
		}
	}
	return 0;
}

static int spacemit_hub_configure(struct spacemit_hub_regulator_plat *spacemit, bool on)
{
	int ret = 0;
	dev_dbg(spacemit->dev, "do hub configure %s\n", on ? "on" : "off");
	if (on) {
		ret = spacemit_hub_enable(spacemit, true);
		if (ret)
			return ret;
		if (spacemit->vbus_delay_ms && spacemit->vbus_gpio_cnt) {
			mdelay(spacemit->vbus_delay_ms);
		}
		ret = spacemit_hub_vbus_enable(spacemit, true);
		if (ret)
			return ret;
	} else {
		ret = spacemit_hub_vbus_enable(spacemit, false);
		if (ret)
			return ret;
		ret = spacemit_hub_enable(spacemit, false);
		if (ret)
			return ret;
	}
	spacemit->is_on = on;
	return 0;
}

static int spacemit_hub_regulator_of_to_plat(struct udevice *dev)
{
	struct dm_regulator_uclass_plat *uc_pdata;
	struct spacemit_hub_regulator_plat *spacemit;
	int ret;

	spacemit = dev_get_plat(dev);
	uc_pdata = dev_get_uclass_plat(dev);
	if (!uc_pdata)
		return -ENXIO;

	/* Set type to fixed to support boot-on/off */
	uc_pdata->type = REGULATOR_TYPE_FIXED;

	spacemit->hub_inter_delay_ms = dev_read_u32_default(dev, "hub_inter_delay_ms", 0);
	spacemit->vbus_inter_delay_ms = dev_read_u32_default(dev, "vbus_inter_delay_ms", 0);
	spacemit->vbus_delay_ms = dev_read_u32_default(dev, "vbus_delay_ms", 10);

 	spacemit->hub_gpio_active_low = dev_read_bool(dev, "hub_gpio_active_low");
	spacemit->vbus_gpio_active_low = dev_read_bool(dev, "vbus_gpio_active_low");

	ret = gpio_request_list_by_name(dev, "hub-gpios", spacemit->hub_gpios,
			 ARRAY_SIZE(spacemit->hub_gpios),
			 spacemit->hub_gpio_active_low ? GPIOD_OUT_HIGH : GPIOD_OUT_LOW);

	if (ret < 0) {
		dev_err(dev, "failed to retrieve hub-gpios from dts: %d\n", ret);
		return ret;
	}
	spacemit->hub_gpio_cnt = ret;
	ret = device_get_supply_regulator(dev, "hub-supply", &spacemit->hub_regulator);
	if (ret < 0 && ret != -ENOENT) {
		dev_err(dev, "failed to retrieve hub-supply from dts: %d\n", ret);
		return ret;
	}
	dev_dbg(dev, "got %d hub-supply, hubs: %p\n", ret, spacemit->hub_regulator);

	ret = gpio_request_list_by_name(dev, "vbus-gpios", spacemit->vbus_gpios,
			ARRAY_SIZE(spacemit->vbus_gpios),
			spacemit->vbus_gpio_active_low ? GPIOD_OUT_HIGH : GPIOD_OUT_LOW);
	if (ret < 0) {
		dev_err(dev, "failed to retrieve hub-gpios from dts: %d\n", ret);
		return ret;
	}
	spacemit->vbus_gpio_cnt = ret;
	ret = device_get_supply_regulator(dev, "vbus-supply", &spacemit->vbus_regulator);
	if (ret < 0 && ret != -ENOENT) {
		dev_err(dev, "failed to retrieve vbus-supply from dts: %d\n", ret);
		return ret;
	}
	dev_dbg(dev, "got vbus-supply ret %d %p\n", ret, spacemit->vbus_regulator);
	dev_dbg(dev, "found hub gpios: %d vbus gpios: %d\n", spacemit->hub_gpio_cnt,
		spacemit->vbus_gpio_cnt);

	return 0;
}

static int spacemit_hub_regulator_get_enable(struct udevice *dev)
{
	struct spacemit_hub_regulator_plat *spacemit = dev_get_plat(dev);
	return spacemit->is_on;
}

static int spacemit_hub_regulator_set_enable(struct udevice *dev, bool enable)
{
	struct spacemit_hub_regulator_plat *spacemit = dev_get_plat(dev);
	return spacemit_hub_configure(spacemit, enable);
}

static const struct dm_regulator_ops spacemit_hub_regulator_ops = {
	.get_enable	= spacemit_hub_regulator_get_enable,
	.set_enable	= spacemit_hub_regulator_set_enable,
};

static const struct udevice_id spacemit_hub_regulator_ids[] = {
	{.compatible = "spacemit,usb-hub",},
	{ },
};

U_BOOT_DRIVER(spacemit_hub_regulator) = {
	.name = "gpio regulator",
	.id = UCLASS_REGULATOR,
	.ops = &spacemit_hub_regulator_ops,
	.of_match = spacemit_hub_regulator_ids,
	.of_to_plat = spacemit_hub_regulator_of_to_plat,
	.plat_auto	= sizeof(struct spacemit_hub_regulator_plat),
};
