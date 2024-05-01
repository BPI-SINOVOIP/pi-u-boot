// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Spacemit K1x Mobile Storage Host Controller
 *
 * Copyright (C) 2023 Spacemit Inc.
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <fdtdec.h>
#include <linux/libfdt.h>
#include <linux/delay.h>
#include <malloc.h>
#include <sdhci.h>
#include <reset-uclass.h>

DECLARE_GLOBAL_DATA_PTR;

/* SDH registers define */
#define SDHC_OP_EXT_REG			0x108
#define OVRRD_CLK_OEN			0x0800
#define FORCE_CLK_ON			0x1000

#define SDHC_LEGACY_CTRL_REG		0x10C
#define GEN_PAD_CLK_ON			0x0040

#define SDHC_MMC_CTRL_REG		0x114
#define MISC_INT_EN			0x0002
#define MISC_INT			0x0004
#define ENHANCE_STROBE_EN		0x0100
#define MMC_HS400			0x0200
#define MMC_HS200			0x0400
#define MMC_CARD_MODE			0x1000

#define SDHC_TX_CFG_REG			0x11C
#define TX_INT_CLK_SEL			0x40000000
#define TX_MUX_SEL			0x80000000

#define SDHC_PHY_CTRL_REG		0x160
#define PHY_FUNC_EN			0x0001
#define PHY_PLL_LOCK			0x0002
#define HOST_LEGACY_MODE		0x80000000

#define SDHC_PHY_FUNC_REG		0x164
#define PHY_TEST_EN			0x0080
#define HS200_USE_RFIFO			0x8000

#define SDHC_PHY_DLLCFG			0x168
#define DLL_PREDLY_NUM			0x04
#define DLL_FULLDLY_RANGE		0x10
#define DLL_VREG_CTRL			0x40
#define DLL_ENABLE			0x80000000
#define DLL_REFRESH_SWEN_SHIFT		0x1C
#define DLL_REFRESH_SW_SHIFT		0x1D

#define SDHC_PHY_DLLCFG1		0x16C
#define DLL_REG2_CTRL			0x0C
#define DLL_REG3_CTRL_MASK		0xFF
#define DLL_REG3_CTRL_SHIFT		0x10
#define DLL_REG2_CTRL_MASK		0xFF
#define DLL_REG2_CTRL_SHIFT		0x08
#define DLL_REG1_CTRL			0x92
#define DLL_REG1_CTRL_MASK		0xFF
#define DLL_REG1_CTRL_SHIFT		0x00

#define SDHC_PHY_DLLSTS			0x170
#define DLL_LOCK_STATE			0x01

#define SDHC_PHY_DLLSTS1		0x174
#define DLL_MASTER_DELAY_MASK		0xFF
#define DLL_MASTER_DELAY_SHIFT		0x10

#define SDHC_PHY_PADCFG_REG		0x178
#define RX_BIAS_CTRL_SHIFT		0x5
#define PHY_DRIVE_SEL_SHIFT		0x0
#define PHY_DRIVE_SEL_MASK		0x7
#define PHY_DRIVE_SEL_DEFAULT		0x4

#define RPM_DELAY			50
#define MAX_74CLK_WAIT_COUNT		74

#define MMC1_IO_V18EN			0x04
#define AKEY_ASFAR			0xBABA
#define AKEY_ASSAR			0xEB10

#define SDHC_RX_CFG_REG			0x118
#define RX_SDCLK_SEL0_MASK		0x03
#define RX_SDCLK_SEL0_SHIFT		0x00
#define RX_SDCLK_SEL0			0x02
#define RX_SDCLK_SEL1_MASK		0x03
#define RX_SDCLK_SEL1_SHIFT		0x02
#define RX_SDCLK_SEL1			0x01

#define SDHC_DLINE_CTRL_REG		0x130
#define DLINE_PU			0x01
#define RX_DLINE_CODE_MASK		0xFF
#define RX_DLINE_CODE_SHIFT		0x10
#define TX_DLINE_CODE_MASK		0xFF
#define TX_DLINE_CODE_SHIFT		0x18

#define SDHC_DLINE_CFG_REG		0x134
#define RX_DLINE_REG_MASK		0xFF
#define RX_DLINE_REG_SHIFT		0x00
#define RX_DLINE_GAIN_MASK		0x1
#define RX_DLINE_GAIN_SHIFT		0x8
#define RX_DLINE_GAIN			0x1
#define TX_DLINE_REG_MASK		0xFF
#define TX_DLINE_REG_SHIFT		0x10

#define SDHC_RX_TUNE_DELAY_MIN		0x0
#define SDHC_RX_TUNE_DELAY_MAX		0xFF
#define SDHC_RX_TUNE_DELAY_STEP		0x1


struct spacemit_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	struct reset_ctl_bulk resets;
	struct clk_bulk clks;
};

struct spacemit_sdhci_priv {
	struct sdhci_host host;
	u32 clk_src_freq;
	u32 phy_module;
};

struct spacemit_sdhci_driver_data {
	const struct sdhci_ops *ops;
	u32 flags;
};

/*
 * refer to PMU_SDH0_CLK_RES_CTRL<0x054>, SDH0_CLK_SEL:0x0, SDH0_CLK_DIV:0x1
 * the default clock source is 204800000Hz [409.6MHz(pll1_d6_409p6Mhz)/2]
 *
 * in the start-up phase, use the 200KHz frequency
 */
#define SDHC_DEFAULT_MAX_CLOCK (204800000)
#define SDHC_MIN_CLOCK (200*1000)

static int is_emulator_platform(void)
{
#ifdef CONFIG_K1_X_BOARD_FPGA
	return 1;
#else
	return 0;
#endif
}

static int spacemit_reg[] = {
	0x100, 0x104, 0x108, 0x10c, 0x110, 0x114, 0x118, 0x11c,
	0x120, 0x124, 0x128, 0x12c, 0x130, 0x134, 0x160, 0x164,
	0x168, 0x16c, 0x170, 0x174, 0x178, 0x17c, 0x180, 0x184,
	0x188, 0x18c, 0x190, 0x1f0, 0x1f4, 0xFFF,
};
static u8 cur_com_reg[960]; /* 8 line, 120  character  per line */
static u8 cur_pri_reg[960];

static void __maybe_unused dump_sdh_regs(struct sdhci_host *host, u8 is_emmc)
{
	int val;
	int offset;
	int i;
	int len;
	char *buf;

	buf = (char *)&cur_com_reg[0];
	len = 0;
	i = 0;
	for (offset = 0; offset < 0x70; offset += 4) {
		val = sdhci_readl(host, offset);
		if (i % 4 == 0)
			len += sprintf(buf + len, "\n");
		len += sprintf(buf + len, "\toffset:0x%03x 0x%08x\t", offset, val);
		i++;
	}

	if (i % 4 == 0)
		len += sprintf(buf + len, "\n");
	val = sdhci_readl(host, 0xe0);
	len += sprintf(buf + len, "\toffset:0x%03x 0x%08x\t", 0xe0, val);
	val = sdhci_readl(host, 0xfc);
	len += sprintf(buf + len, "\toffset:0x%03x 0x%08x\t\n", 0xfc, val);

	buf = (char *)&cur_pri_reg[0];
	len = 0;
	i = 0;
	do {
		if (!is_emmc && (spacemit_reg[i] > 0x134))
			break;
		val = sdhci_readl(host, spacemit_reg[i]);
		if (i % 4 == 0)
			len += sprintf(buf + len, "\n");
		len += sprintf(buf + len, "\toffset:0x%03x 0x%08x\t", spacemit_reg[i], val);
		i++;
	} while (spacemit_reg[i] != 0xFFF);
	len += sprintf(buf + len, "\n");

	pr_debug("%s", cur_com_reg);
	pr_debug("%s", cur_pri_reg);
}

static void spacemit_mmc_phy_init(struct sdhci_host *host)
{
	struct udevice *dev = host->mmc->dev;
	struct spacemit_sdhci_priv *priv = dev_get_priv(dev);
	unsigned int value = 0;

	if (priv->phy_module) {
		/* mmc card mode */
		value = sdhci_readl(host, SDHC_MMC_CTRL_REG);
		value |= MMC_CARD_MODE;
		sdhci_writel(host, value, SDHC_MMC_CTRL_REG);

		if (is_emulator_platform()) {
			/* mmc phy bypass */
			value = sdhci_readl(host, SDHC_TX_CFG_REG);
			value |= TX_INT_CLK_SEL;
			sdhci_writel (host, value, SDHC_TX_CFG_REG);

			value = sdhci_readl(host, SDHC_PHY_CTRL_REG);
			value |= HOST_LEGACY_MODE;
			sdhci_writel (host, value, SDHC_PHY_CTRL_REG);

			value = sdhci_readl(host, SDHC_PHY_FUNC_REG);
			value |= PHY_TEST_EN;
			sdhci_writel (host, value, SDHC_PHY_FUNC_REG);
			pr_debug("%s: mmc phy bypass.\n", host->name);
		} else {
			/* use phy func mode */
			value = sdhci_readl(host, SDHC_PHY_CTRL_REG);
			value |= (PHY_FUNC_EN | PHY_PLL_LOCK);
			sdhci_writel(host, value, SDHC_PHY_CTRL_REG);

			value = sdhci_readl(host, SDHC_PHY_PADCFG_REG);
			value |= (1 << RX_BIAS_CTRL_SHIFT);
			sdhci_writel(host, value, SDHC_PHY_PADCFG_REG);
			pr_debug("%s: use mmc phy func.\n", host->name);
		}
	} else {
		pr_debug("%s: not support phy module.\n", host->name);
		value = sdhci_readl (host, SDHC_TX_CFG_REG);
		value |= TX_INT_CLK_SEL;
		sdhci_writel (host, value, SDHC_TX_CFG_REG);
	}

	value = sdhci_readl(host, SDHC_MMC_CTRL_REG);
	value &= ~ENHANCE_STROBE_EN;
	sdhci_writel(host, value, SDHC_MMC_CTRL_REG);
}

#define MAX_WAIT_COUNT 100
int spacemit_set_sdh_74_clk(struct udevice *dev)
{
	struct spacemit_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	u32 tmp = 0;
	int count = 0;

	tmp = sdhci_readl(host, SDHC_MMC_CTRL_REG);
	tmp |= MISC_INT | MISC_INT_EN;
	sdhci_writel(host, tmp, SDHC_MMC_CTRL_REG);

	tmp = sdhci_readl(host, SDHC_LEGACY_CTRL_REG);
	tmp |= GEN_PAD_CLK_ON;
	sdhci_writel(host, tmp, SDHC_LEGACY_CTRL_REG);

	while (count < MAX_WAIT_COUNT) {
		if (sdhci_readl(host, SDHC_MMC_CTRL_REG) & MISC_INT)
			break;

		udelay(10);
		count++;
	}
	if (count >= MAX_WAIT_COUNT){
		pr_err("%s: 74 clk wait timeout(%d)\n", host->name, count);
	}
	return 0;
}

void spacemit_sdhci_set_control_reg(struct sdhci_host *host)
{
	struct mmc *mmc = (struct mmc *)host->mmc;
	u32 reg;

	if (mmc->signal_voltage == MMC_SIGNAL_VOLTAGE_180) {
		reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		reg |= SDHCI_CTRL_VDD_180;
		sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);
	}

	if (mmc->selected_mode > SD_HS &&
	    mmc->selected_mode <= MMC_HS_400_ES)
		sdhci_set_uhs_timing(host);

	/* according to the SDHC_TX_CFG_REG(0x11c<bit>),
	 * set TX_INT_CLK_SEL to gurantee the hold time
	 * at default speed mode or HS/SDR12/SDR25/SDR50 mode.
	 */
	reg = sdhci_readl(host, SDHC_TX_CFG_REG);
	if ((mmc->selected_mode == MMC_LEGACY) ||
	    (mmc->selected_mode == MMC_HS) ||
	    (mmc->selected_mode == SD_HS) ||
	    (mmc->selected_mode == UHS_SDR12) ||
	    (mmc->selected_mode == UHS_SDR25) ||
	    (mmc->selected_mode == UHS_SDR50)) {
		reg |= TX_INT_CLK_SEL;
	} else {
		reg &= ~TX_INT_CLK_SEL;
	}
	sdhci_writel(host, reg, SDHC_TX_CFG_REG);

	if ((mmc->selected_mode == MMC_HS_200) ||
	    (mmc->selected_mode == MMC_HS_400) ||
	    (mmc->selected_mode == MMC_HS_400_ES)) {
		reg = sdhci_readw(host, SDHC_MMC_CTRL_REG);
		reg |= (mmc->selected_mode == MMC_HS_200) ? MMC_HS200 : MMC_HS400;
		sdhci_writew(host, reg, SDHC_MMC_CTRL_REG);
	} else {
		reg = sdhci_readw(host, SDHC_MMC_CTRL_REG);
		reg &= ~(MMC_HS200 | MMC_HS400 | ENHANCE_STROBE_EN);
		sdhci_writew(host, reg, SDHC_MMC_CTRL_REG);
	}
}

#if CONFIG_IS_ENABLED(MMC_HS400_ES_SUPPORT)
static int spacemit_sdhci_phy_dll_init(struct sdhci_host *host)
{
	u32 reg;
	int i;

	/* config dll_reg1 & dll_reg2 */
	reg = sdhci_readl(host, SDHC_PHY_DLLCFG);
	reg |= (DLL_PREDLY_NUM | DLL_FULLDLY_RANGE | DLL_VREG_CTRL);
	sdhci_writel(host, reg, SDHC_PHY_DLLCFG);

	reg = sdhci_readl(host, SDHC_PHY_DLLCFG1);
	reg |= (DLL_REG1_CTRL & DLL_REG1_CTRL_MASK);
	sdhci_writel(host, reg, SDHC_PHY_DLLCFG1);

	/* dll enable */
	reg = sdhci_readl(host, SDHC_PHY_DLLCFG);
	reg |= DLL_ENABLE;
	sdhci_writel(host, reg, SDHC_PHY_DLLCFG);

	/* wait dll lock */
	i = 0;
	while (i++ < 100) {
		if (sdhci_readl(host, SDHC_PHY_DLLSTS) & DLL_LOCK_STATE)
			break;
		udelay(10);
	}
	if (i == 100) {
		pr_err("%s: phy dll lock timeout\n", host->name);
		return 1;
	}

	return 0;
}

static int spacemit_sdhci_hs400_enhanced_strobe(struct sdhci_host *host)
{
	u32 reg;

	reg = sdhci_readl(host, SDHC_MMC_CTRL_REG);
	reg |= ENHANCE_STROBE_EN;
	sdhci_writel(host, reg, SDHC_MMC_CTRL_REG);

	return spacemit_sdhci_phy_dll_init(host);
}
#endif

static int spacemit_sdhci_probe(struct udevice *dev)
{
	struct spacemit_sdhci_driver_data *drv_data =
			(struct spacemit_sdhci_driver_data *)dev_get_driver_data(dev);
	struct spacemit_sdhci_plat *plat = dev_get_plat(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct spacemit_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	struct dm_mmc_ops *mmc_driver_ops = (struct dm_mmc_ops *)dev->driver->ops;
	struct clk clk;
	int ret = 0;

	host->mmc = &plat->mmc;
	host->mmc->priv = host;
	host->mmc->dev = dev;
	upriv->mmc = host->mmc;

	ret = clk_get_bulk(dev, &plat->clks);
	if (ret) {
		pr_err("Can't get clk: %d\n", ret);
		return ret;
	}

	ret = clk_enable_bulk(&plat->clks);
	if (ret) {
		pr_err("Failed to enable clk: %d\n", ret);
		return ret;
	}

	ret = reset_get_bulk(dev, &plat->resets);
	if (ret) {
		pr_err("Can't get reset: %d\n", ret);
		return ret;
	}

	ret = reset_deassert_bulk(&plat->resets);
	if (ret) {
		pr_err("Failed to reset: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret) {
		pr_err("Can't get io clk: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(&clk, priv->clk_src_freq);
	if (ret) {
		pr_err("Failed to set io clk: %d\n", ret);
		return ret;
	}

	/* Set quirks */
#if defined(CONFIG_SPL_BUILD)
	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD;
#else
	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD | SDHCI_QUIRK_32BIT_DMA_ADDR;
#endif
	host->host_caps = MMC_MODE_HS | MMC_MODE_HS_52MHz;
	host->max_clk = priv->clk_src_freq;

	plat->cfg.f_max = priv->clk_src_freq;
	plat->cfg.f_min = SDHC_MIN_CLOCK;
	host->ops = drv_data->ops;

	mmc_driver_ops->deferred_probe = spacemit_set_sdh_74_clk;

	ret = sdhci_setup_cfg(&plat->cfg, host, priv->clk_src_freq, SDHC_MIN_CLOCK);
	if (ret)
		return ret;

	ret = sdhci_probe(dev);
	if (ret)
		return ret;

	/* emmc phy bypass if need */
	spacemit_mmc_phy_init(host);

	pr_info("%s: probe done.\n", host->name);
	return ret;
}

static int spacemit_sdhci_of_to_plat(struct udevice *dev)
{
	struct spacemit_sdhci_plat *plat = dev_get_plat(dev);
	struct spacemit_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(dev);
	int ret = 0;

	host->name = dev->name;
	host->ioaddr = (void *)devfdt_get_addr(dev);
	priv->phy_module = fdtdec_get_uint(blob, node, "sdh-phy-module", 0);

	priv->clk_src_freq = fdtdec_get_uint(blob, node, "clk-src-freq", SDHC_DEFAULT_MAX_CLOCK);
	ret = mmc_of_parse(dev, &plat->cfg);

	return ret;
}

static int spacemit_sdhci_bind(struct udevice *dev)
{
	struct spacemit_sdhci_plat *plat = dev_get_plat(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

const struct sdhci_ops spacemit_sdhci_ops = {
	.set_control_reg = spacemit_sdhci_set_control_reg,
#if CONFIG_IS_ENABLED(MMC_HS400_ES_SUPPORT)
	.set_enhanced_strobe = spacemit_sdhci_hs400_enhanced_strobe,
#endif
};

const struct spacemit_sdhci_driver_data spacemit_sdhci_drv_data = {
	.ops = &spacemit_sdhci_ops,
};

static const struct udevice_id spacemit_sdhci_ids[] = {
	{ .compatible = "spacemit,k1-x-sdhci",
	  .data = (ulong)&spacemit_sdhci_drv_data,
	},
	{ }
};

U_BOOT_DRIVER(spacemit_sdhci_drv) = {
	.name		= "spacemit_sdhci",
	.id		= UCLASS_MMC,
	.of_match	= spacemit_sdhci_ids,
	.of_to_plat = spacemit_sdhci_of_to_plat,
	.ops		= &sdhci_ops,
	.bind		= spacemit_sdhci_bind,
	.probe		= spacemit_sdhci_probe,
	.priv_auto = sizeof(struct spacemit_sdhci_priv),
	.plat_auto = sizeof(struct spacemit_sdhci_plat),
};
