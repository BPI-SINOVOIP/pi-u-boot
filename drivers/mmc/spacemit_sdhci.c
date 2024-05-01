// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <malloc.h>
#include <sdhci.h>
#include <reset-uclass.h>
#include <asm/global_data.h>

#define SPACEMIT_SDHC_MIN_FREQ    (400000)
#define SDHCI_CTRL_ADMA2_LEN_MODE BIT(10)
#define SDHCI_CTRL_CMD23_ENABLE BIT(11)
#define SDHCI_CTRL_HOST_VERSION_4_ENABLE BIT(12)
#define SDHCI_CTRL_ADDRESSING BIT(13)
#define SDHCI_CTRL_ASYNC_INT_ENABLE BIT(14)

#define SDHCI_CLOCK_PLL_EN BIT(3)

DECLARE_GLOBAL_DATA_PTR;

struct spacemit_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	struct reset_ctl *reset;
};

static const struct sdhci_ops spacemit_ops = {
	.set_control_reg = &sdhci_set_control_reg,
};

static void sdhci_do_enable_v4_mode(struct udevice *dev)
{
	struct sdhci_host *host = dev_get_priv(dev);
	u16 ctrl2;
	ctrl2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	ctrl2 |= SDHCI_CTRL_ADMA2_LEN_MODE
			| SDHCI_CTRL_CMD23_ENABLE
			| SDHCI_CTRL_HOST_VERSION_4_ENABLE
			| SDHCI_CTRL_ADDRESSING
			| SDHCI_CTRL_ASYNC_INT_ENABLE;

	sdhci_writew(host, ctrl2, SDHCI_HOST_CONTROL2);
}

static struct dm_mmc_ops spacemit_mmc_ops;

static int spacemit_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct spacemit_sdhci_plat *plat = dev_get_plat(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	u32 max_clk;
	int ret;

	struct clk clk;
	ret = clk_get_by_index(dev, 0, &clk);
	if (ret)
		return ret;

	ret = clk_enable(&clk);
	if (ret)
		return ret;

	plat->reset = devm_reset_control_get_by_index(dev, 0);
	if (IS_ERR(plat->reset)) {
		pr_err("get optional reset failed\n");
		return -EINVAL;
	}

	ret = reset_deassert(plat->reset);
	if (ret < 0) {
		pr_err("MMC1 deassert failed: %d", ret);
		return ret;
	}

	max_clk = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
					"max-frequency", 50000000);

	host->name = dev->name;
	host->ioaddr = dev_read_addr_ptr(dev);
	host->ops = &spacemit_ops;
	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD;
	host->bus_width	= fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
					 "bus-width", 4);

	ret = mmc_of_parse(dev, &plat->cfg);
	if (ret)
		return ret;

	host->max_clk = max_clk;
	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	spacemit_mmc_ops = sdhci_ops;
	spacemit_mmc_ops.reinit = sdhci_probe;

	ret = sdhci_setup_cfg(&plat->cfg, host, max_clk, SPACEMIT_SDHC_MIN_FREQ);
	if (ret)
		return ret;

	host->mmc->priv = host;
	upriv->mmc = host->mmc;

	/* clk_free(&clk); */
	
	ret = sdhci_probe(dev);
	if (ret)
		return ret;

	/*
	enable v4 should execute after sdhci_probe, because sdhci reset would
	clear the SDHCI_HOST_CONTROL2 register.
	*/
	sdhci_do_enable_v4_mode(dev);
	return 0;
}

static int spacemit_sdhci_bind(struct udevice *dev)
{
	struct spacemit_sdhci_plat *plat = dev_get_plat(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id spacemit_sdhci_ids[] = {
	{ .compatible = "spacemit,k1-pro-sdhci" },
	{ }
};

U_BOOT_DRIVER(spacemit_sdhci_drv) = {
	.name		= "spacemit_sdhci",
	.id		= UCLASS_MMC,
	.of_match	= spacemit_sdhci_ids,
	.ops		= &spacemit_mmc_ops,
	.bind		= spacemit_sdhci_bind,
	.probe		= spacemit_sdhci_probe,
	.priv_auto	= sizeof(struct sdhci_host),
	.plat_auto	= sizeof(struct spacemit_sdhci_plat),
};
