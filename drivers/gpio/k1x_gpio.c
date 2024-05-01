// SPDX-License-Identifier: GPL-2.0
/*
 * spacemit k1x gpio driver
 *
 * Copyright (C) 2023 Spacemit
 *
 */

#include <common.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/gpio.h>
#include <linux/bitops.h>
#include <clk.h>
#include "k1x_gpio.h"

#ifdef CONFIG_DM_GPIO
#include <dm/read.h>
#include <dm/device.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <dm/pinctrl.h>

static void __iomem *k1x_gpio_base;
#else
#define K1X_GPIO_BASE   0xd4019000
#endif

#ifndef K1X_MAX_GPIO
#define K1X_MAX_GPIO	128
#endif

#define GPIO_TO_REG(gp)		(gp >> 5)
#define GPIO_TO_BIT(gp)		(1 << (gp & 0x1f))
#define GPIO_VAL(gp, val)	((val >> (gp & 0x1f)) & 0x01)

/**
 * struct k1x_gpio_pctrl_map - gpio and pinctrl mapping
 * @gpio_pin:	start of gpio number in gpio-ranges
 * @pctrl_pin:	start of pinctrl number in gpio-ranges
 * @npins:	total number of pins in gpio-ranges
 * @node:	list node
 */
struct k1x_gpio_pctrl_map {
	u32 gpio_pin;
	u32 pctrl_pin;
	u32 npins;
	struct list_head node;
};

/**
 * struct k1x_gpio_pctrl_map - gpio device instance
 * @pinctrl_dev:pointer to gpio device
 * @gpiomap:	list node having mapping between gpio and pinctrl
 * @base:	I/O register base address of gpio device
 * @name:	gpio device name, ex GPIO0, GPIO1
 * @ngpios:	total number of gpios
 */
struct k1x_gpio_plat {
	struct udevice *pinctrl_dev;
	struct list_head gpiomap;
	void __iomem *base;
};

static inline void *get_gpio_base(int bank)
{
	const unsigned long offset[] = {0, 4, 8, 0x100};
	/* gpio register bank offset */
#ifdef CONFIG_DM_GPIO
	return (struct gpio_reg *)(k1x_gpio_base + offset[bank]);
#else
	return (struct gpio_reg *)(K1X_GPIO_BASE + offset[bank]);
#endif
}

static int _gpio_direction_input(unsigned gpio)
{
	struct gpio_reg *gpio_reg_bank;

	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO %d\n", __func__, gpio);
		return -1;
	}

	gpio_reg_bank = get_gpio_base(GPIO_TO_REG(gpio));
	writel(GPIO_TO_BIT(gpio), &gpio_reg_bank->gcdr);
	return 0;
}

static int _gpio_set_value(unsigned gpio, int value)
{
	struct gpio_reg *gpio_reg_bank;

	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO %d\n", __func__, gpio);
		return -1;
	}

	gpio_reg_bank = get_gpio_base(GPIO_TO_REG(gpio));
	if (value)
		writel(GPIO_TO_BIT(gpio), &gpio_reg_bank->gpsr);
	else
		writel(GPIO_TO_BIT(gpio), &gpio_reg_bank->gpcr);

	return 0;
}

static int _gpio_direction_output(unsigned gpio, int value)
{
	struct gpio_reg *gpio_reg_bank;

	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO %d\n", __func__, gpio);
		return -1;
	}

	gpio_reg_bank = get_gpio_base(GPIO_TO_REG(gpio));
	writel(GPIO_TO_BIT(gpio), &gpio_reg_bank->gsdr);
	_gpio_set_value(gpio, value);
	return 0;
}

static int _gpio_get_value(unsigned gpio)
{
	struct gpio_reg *gpio_reg_bank;
	u32 gpio_val;

	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO %d\n", __func__, gpio);
		return -1;
	}

	gpio_reg_bank = get_gpio_base(GPIO_TO_REG(gpio));
	gpio_val = readl(&gpio_reg_bank->gplr);

	return GPIO_VAL(gpio, gpio_val);
}

#ifdef CONFIG_DM_GPIO
/**
 * k1x_get_gpio_pctrl_mapping() - get associated pinctrl pin from gpio pin
 *
 * @plat: k1x GPIO device
 * @gpio: GPIO pin
 */
static int k1x_get_pctrl_from_gpio(struct k1x_gpio_plat *plat, u32 gpio, u32 *pctrl_pin)
{
	struct k1x_gpio_pctrl_map *range = NULL;
	struct list_head *pos, *tmp;
	int ret = -EINVAL;

	list_for_each_safe(pos, tmp, &plat->gpiomap) {
		range = list_entry(pos, struct k1x_gpio_pctrl_map, node);
		if (gpio >= range->gpio_pin &&
		    gpio < (range->gpio_pin + range->npins)) {
			*pctrl_pin = range->pctrl_pin + (gpio - range->gpio_pin);
			ret = 0;
			break;
		}
	}

	return ret;
}

/**
 * k1x_get_gpio_pctrl_mapping() - get mapping between gpio and pinctrl
 *
 * Read dt node "gpio-ranges" to get gpio and pinctrl mapping and store
 * in private data structure to use it later while enabling gpio.
 *
 * @dev: pointer to GPIO device
 * Return: 0 on success and -ENOMEM on failure
 */
static int k1x_get_gpio_pctrl_mapping(struct udevice *dev)
{
	struct k1x_gpio_plat *plat = dev_get_plat(dev);
	struct k1x_gpio_pctrl_map *range = NULL;
	struct ofnode_phandle_args args;
	int index = 0, ret;

	for (;; index++) {
		ret = dev_read_phandle_with_args(dev, "gpio-ranges",
						 NULL, 3, index, &args);
		if (ret)
			break;

		range = (struct k1x_gpio_pctrl_map *)devm_kzalloc(dev, sizeof(*range), GFP_KERNEL);
		if (!range)
			return -ENOMEM;

		range->gpio_pin = args.args[0];
		range->pctrl_pin = args.args[1];
		range->npins = args.args[2];
		list_add_tail(&range->node, &plat->gpiomap);
	}

	return 0;
}

static int gpio_k1x_probe(struct udevice *dev)
{
	struct k1x_gpio_plat *plat = dev_get_plat(dev);
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct clk gpio_clk;
	int ret = 0;

	plat->base = dev_remap_addr_index(dev, 0);
	if (!plat->base) {
		debug("%s: Failed to get base address\n", __func__);
		return -EINVAL;
	}
	k1x_gpio_base = plat->base;

	uc_priv->gpio_count = dev_read_u32_default(dev, "gpio-count", 0);
	ret = uclass_get_device_by_phandle(UCLASS_PINCTRL, dev, "gpio-ranges",
				     &plat->pinctrl_dev);
	if (ret == 0) {
		INIT_LIST_HEAD(&plat->gpiomap);
		ret = k1x_get_gpio_pctrl_mapping(dev);
		if (ret < 0) {
			dev_err(dev, "%s: Failed to get gpio to pctrl map ret(%d)\n",
				__func__, ret);
			return ret;
		}
	} else {
		dev_info(dev, "%s: has no gpio-ranges\n", __func__);
	}

	ret = clk_get_by_index(dev, 0, &gpio_clk);
	if (ret) 
		return ret;
	clk_enable(&gpio_clk);

	return 0;
}

static const struct udevice_id gpio_k1x_ids[] = {
	{ .compatible = "spacemit,k1x-gpio" },
	{ }
};

static int k1x_gpio_request(struct udevice *dev, unsigned gpio,
			     const char *label)
{
	struct k1x_gpio_plat *plat = dev_get_plat(dev);
	u32 pctrl;
	int ret = 0;

	/* nothing to do if there is no corresponding pinctrl device */
	if (!plat->pinctrl_dev)
		return 0;

	ret = k1x_get_pctrl_from_gpio(plat, gpio, &pctrl);
	if (ret < 0)
		return 0;

	return pinctrl_request(plat->pinctrl_dev, pctrl, 0);
}

static int k1x_gpio_get_value(struct udevice *dev, unsigned int gpio)
{
	return _gpio_get_value(gpio);
}

static int k1x_gpio_set_value(struct udevice *dev, unsigned int gpio,
				   int value)
{
	return _gpio_set_value(gpio, value);
}

static int k1x_gpio_direction_input(struct udevice *dev, unsigned int gpio)
{
	return _gpio_direction_input(gpio);
}

static int k1x_gpio_direction_output(struct udevice *dev, unsigned int gpio,
					  int value)
{
	return _gpio_direction_output(gpio, value);
}

static const struct dm_gpio_ops gpio_k1x_ops = {
	.request		= k1x_gpio_request,
	.direction_input	= k1x_gpio_direction_input,
	.direction_output	= k1x_gpio_direction_output,
	.get_value		= k1x_gpio_get_value,
	.set_value		= k1x_gpio_set_value,
};
U_BOOT_DRIVER(gpio_k1x) = {
	.name	= "gpio_k1x",
	.id	= UCLASS_GPIO,
	.ops	= &gpio_k1x_ops,
	.of_match = gpio_k1x_ids,
	.probe	= gpio_k1x_probe,
	.plat_auto	= sizeof(struct k1x_gpio_plat),
};

#else

int gpio_request(unsigned gpio, const char *label)
{
	if (gpio >= K1X_MAX_GPIO) {
		printf("%s: Invalid GPIO requested %d\n", __func__, gpio);
		return -1;
	}
	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	return _gpio_direction_input(gpio);
}

int gpio_direction_output(unsigned gpio, int value)
{
	return _gpio_direction_output(gpio, value);
}

int gpio_get_value(unsigned gpio)
{
	return _gpio_get_value(gpio);
}

int gpio_set_value(unsigned gpio, int value)
{
	return _gpio_set_value(gpio, value);
}
#endif
