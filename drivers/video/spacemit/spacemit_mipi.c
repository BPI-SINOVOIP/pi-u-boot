// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <asm/gpio.h>
#include <asm/io.h>
#include <common.h>
#include <clk.h>
#include <display.h>
#include <dm.h>
#include <regmap.h>
#include <panel.h>
#include <regmap.h>
#include <syscon.h>
#include <power-domain-uclass.h>
#include <power-domain.h>
#include <power/regulator.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mipi_dsi.h>
#include <reset.h>

#include "spacemit_mipi.h"
#include "./dsi/include/spacemit_dsi_common.h"


static int spacemit_mipi_dsi_enable(struct udevice *dev,
		       const struct display_timing *timing)
{
	ofnode node, timing_node;
	int val;
	// struct spacemit_mipi_priv *priv = dev_get_priv(dev);

	/* Set dpi color coding depth 24 bit */
	timing_node = ofnode_find_subnode(dev_ofnode(dev), "display-timings");
	node = ofnode_first_subnode(timing_node);

	val = ofnode_read_u32_default(node, "bits-per-pixel", -1);

	return 0;
}

static int spacemit_mipi_phy_enable(struct udevice *dev)
{
	// struct spacemit_mipi_priv *priv = dev_get_priv(dev);
	return 0;
}


static int mipi_dsi_read_timing(struct udevice *dev,
			struct display_timing *timing)
{
	// int ret;

	// ret = ofnode_decode_display_timing(dev_ofnode(dev), 0, timing);
	// if (ret) {
	// 	pr_debug("%s: Failed to decode display timing (ret=%d)\n",
	// 	      __func__, ret);
	// 	return -EINVAL;
	// }

	return 0;
}

static int mipi_dsi_display_enable(struct udevice *dev, int panel_bpp,
			  const struct display_timing *timing)
{
	int ret;
	// struct spacemit_mipi_priv *priv = dev_get_priv(dev);

	/* Config  and enable mipi dsi according to timing */
	ret = spacemit_mipi_dsi_enable(dev, timing);
	if (ret) {
		pr_debug("%s: spacemit_mipi_dsi_enable() failed (err=%d)\n",
		      __func__, ret);
		return ret;
	}

	/* Config and enable mipi phy */
	ret = spacemit_mipi_phy_enable(dev);
	if (ret) {
		pr_debug("%s: spacemit_mipi_phy_enable() failed (err=%d)\n",
		      __func__, ret);
		return ret;
	}

	/* Enable backlight */
	// ret = panel_enable_backlight(priv->panel);
	// if (ret) {
	// 	pr_debug("%s: panel_enable_backlight() failed (err=%d)\n",
	// 	      __func__, ret);
	// 	return ret;
	// }

	return 0;
}

static int spacemit_mipi_dsi_of_to_plat(struct udevice *dev)
{
	lcd_mipi_probe();
	return 0;
}

static int spacemit_mipi_dsi_probe(struct udevice *dev)
{
	struct spacemit_mipi_priv *priv = dev_get_priv(dev);
	struct power_domain pm_domain;
	unsigned long rate;
	int ret;

	ret = power_domain_get(dev, &pm_domain);
	if (ret) {
		pr_err("power_domain_get mipi dsi failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "pxclk", &priv->pxclk);
	if (ret) {
		pr_err("clk_get_by_name mipi dsi pxclk failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "mclk", &priv->mclk);
	if (ret) {
		pr_err("clk_get_by_name mipi dsi mclk failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "hclk", &priv->hclk);
	if (ret) {
		pr_err("clk_get_by_name mipi dsi hclk failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "escclk", &priv->escclk);
	if (ret) {
		pr_err("clk_get_by_name mipi dsi escclk failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "bitclk", &priv->bitclk);
	if (ret) {
		pr_err("clk_get_by_name mipi dsi bitclk failed: %d", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "dsi_reset", &priv->dsi_reset);
	if (ret) {
		pr_err("reset_get_by_name dsi_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "mclk_reset", &priv->mclk_reset);
	if (ret) {
		pr_err("reset_get_by_name mclk_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "esc_reset", &priv->esc_reset);
	if (ret) {
		pr_err("reset_get_by_name esc_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "lcd_reset", &priv->lcd_reset);
	if (ret) {
		pr_err("reset_get_by_name lcd_reset failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->pxclk);
	if (ret < 0) {
		pr_err("clk_enable mipi dsi pxclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->mclk);
	if (ret < 0) {
		pr_err("clk_enable mipi dsi mclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->hclk);
	if (ret < 0) {
		pr_err("clk_enable mipi dsi hclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->escclk);
	if (ret < 0) {
		pr_err("clk_enable mipi dsi escclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_enable(&priv->bitclk);
	if (ret < 0) {
		pr_err("clk_enable mipi dsi bitclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->pxclk, 88000000);
	if (ret < 0) {
		pr_err("clk_set_rate mipi dsi pxclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->mclk, 307200000);
	if (ret < 0) {
		pr_err("clk_set_rate mipi dsi mclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->escclk, 51200000);
	if (ret < 0) {
		pr_err("clk_set_rate mipi dsi escclk failed: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&priv->bitclk, 614400000);
	if (ret < 0) {
		pr_err("clk_set_rate mipi dsi bitclk failed: %d\n", ret);
		return ret;
	}

	rate = clk_get_rate(&priv->pxclk);
	pr_debug("%s clk_get_rate pxclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->mclk);
	pr_debug("%s clk_get_rate mclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->hclk);
	pr_debug("%s clk_get_rate hclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->escclk);
	pr_debug("%s clk_get_rate escclk rate = %ld\n", __func__, rate);

	rate = clk_get_rate(&priv->bitclk);
	pr_debug("%s clk_get_rate bitclk rate = %ld\n", __func__, rate);

	ret = reset_deassert(&priv->dsi_reset);
	if (ret) {
		pr_err("reset_assert dsi_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->mclk_reset);
	if (ret) {
		pr_err("reset_assert mclk_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->esc_reset);
	if (ret) {
		pr_err("reset_assert esc_reset failed: %d\n", ret);
		return ret;
	}

	ret = reset_deassert(&priv->lcd_reset);
	if (ret) {
		pr_err("reset_assert lcd_reset failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct dm_display_ops spacemit_mipi_dsi_ops = {
	.read_timing = mipi_dsi_read_timing,
	.enable = mipi_dsi_display_enable,
};

static const struct udevice_id spacemit_mipi_dsi_ids[] = {
	{ .compatible = "spacemit,mipi-dsi" },
	{ }
};

U_BOOT_DRIVER(spacemit_mipi_dsi) = {
	.name = "spacemit_mipi_dsi",
	.id = UCLASS_DISPLAY,
	.of_match = spacemit_mipi_dsi_ids,
	.ops = &spacemit_mipi_dsi_ops,
	.of_to_plat = spacemit_mipi_dsi_of_to_plat,
	.probe = spacemit_mipi_dsi_probe,
	.priv_auto	= sizeof(struct spacemit_mipi_priv),
};
