/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Cash Zhang <cazhan@synaptics.com>
 *         Shaojun Feng <Shaojun.feng@synaptics.com>
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h>
#include <memalign.h>
#include <reset.h>
#include <sdhci.h>
#include <clk.h>

DECLARE_GLOBAL_DATA_PTR;

/* Register Offset of SD Host Controller SOCP self-defined register */
#define SDHCI_P_VENDOR_SPECIFIC_AREA	0xE8
#define SDHCI_P_VENDOR_SPECIFIC_AREA_MASK 0xFFF
#define SDHCI_EMMC_CTRL_OFFSET			0x2C

/* phy */
#define PHY_COMMDL_CNFG_REG_OFFSET 0x031C
#define PHY_SMPLDL_CNFG_REG_OFFSET 0x0320
#define PHY_AT_CTRL_R_REG_OFFSET 0x540
#define TUNE_CLK_STOP_EN (1<<16)
#define PRE_CHANGE_DLY (3<<17)
#define POST_CHANGE_DLY (3<<19)
#define PHY_REG_OFFSET 0x300
#define PHY_CNFG_REG   0x300
#define PHY_CNFG_PHY_PWRGOOD_MASK 0x2

/*PHY PAD GENERAL modes */
#define PAD_SP_8    0x8
#define PAD_SP_9    0x9
#define PAD_SN_8    0x8

typedef struct {
	unsigned int addr;
	unsigned int sp_bit;
	unsigned int sn_bit;
	unsigned int mask;
	unsigned int sp_value;
	unsigned int sn_value;
} PHY_PAD_GENERAL;

struct phy_setting{
	unsigned int addr;
	unsigned int startbit;
	unsigned int mask;
	unsigned int value;
};

struct dwcmshc_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	struct reset_ctl_bulk reset_ctl;
	int num_phy_setting;
	struct phy_setting * phy_setting;
};

/* 1v8 default PHY config */
PHY_PAD_GENERAL pad_general_1v8 =
{
	0x300, 16, 20, 0xF, PAD_SP_8, PAD_SN_8
};

int dwcmsh_PHYconfig(struct sdhci_host *host, struct phy_setting *ps, int num)
{
	int i;
	volatile unsigned int val;
	int tmout = 100, ret = 0;

	for(i = 0; i < num; i++)
	{
		val = sdhci_readl(host, ps[i].addr);
		val &= ~(ps[i].mask << ps[i].startbit);
		val |= (ps[i].value << ps[i].startbit);
		sdhci_writel(host, val, ps[i].addr);
	}

	do {
		val = sdhci_readl(host, PHY_CNFG_REG);

		mdelay(1);
		if(!tmout) {
			printf("EMMC PHY's PowerGood status is not ready !\n");
			ret = -1;
		}
	}while(!(val & PHY_CNFG_PHY_PWRGOOD_MASK) && tmout--);

	return ret;
}

void dwcmsh_PHYreset(struct sdhci_host *host, int rst)
{
	volatile unsigned int val;

	val = sdhci_readl(host, PHY_CNFG_REG);
	val &= ~(0x1 << 0);
	val |= rst;
	sdhci_writel(host, val, PHY_CNFG_REG);
}

static void dwcmshc_sdhci_set_control_reg(struct sdhci_host *host)
{
	u32 reg;

	if(host->mmc->signal_voltage == MMC_SIGNAL_VOLTAGE_180) {
		reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		reg |= SDHCI_CTRL_VDD_180;
		sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);
	}

	if(host->mmc->selected_mode == MMC_HS_52) {
		reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		reg &= ~SDHCI_CTRL_UHS_MASK;
		reg |= HIGH_SPEED_BUS_SPEED;
		sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);
	}
}

const struct sdhci_ops dwcmshc_ops = {
	.set_control_reg = &dwcmshc_sdhci_set_control_reg,
};

static void dwcmshc_sdhci_reset(struct dwcmshc_sdhci_plat *plat,
			      struct sdhci_host *host)
{
	u32 data, val;

	/* eMMC reset Host */
	reset_assert_bulk(&plat->reset_ctl);
	mdelay(1);
	reset_deassert_bulk(&plat->reset_ctl);

	/* eMMC reset device */
	data = sdhci_readl(host, SDHCI_P_VENDOR_SPECIFIC_AREA);
	data &= SDHCI_P_VENDOR_SPECIFIC_AREA_MASK;
	data += SDHCI_EMMC_CTRL_OFFSET;
	val = sdhci_readl(host, data);
	val |= 0x1;
	val |= (0x1 << 3);
	val &= ~(0x1 << 2);
	sdhci_writel(host, val, data);
	mdelay(1);
	val = sdhci_readl(host, data);
	val |= (0x1 << 2);
	sdhci_writel(host, val, data);
	mdelay(1);
	val = sdhci_readl(host, data);
}

static void dwcmshc_sdhci_slot_init(struct sdhci_host *host)
{
	u32 offset, data;

	offset = sdhci_readl(host, SDHCI_P_VENDOR_SPECIFIC_AREA);
	offset &= SDHCI_P_VENDOR_SPECIFIC_AREA_MASK;
	offset += SDHCI_EMMC_CTRL_OFFSET;
	data = sdhci_readl(host, offset);
	data |= 0x1;
	sdhci_writel(host, data, offset);
}


static void SDHC_PHYDelayLineSetup(struct udevice *dev)
{
	struct sdhci_host *host = dev_get_priv(dev);
	unsigned int value0, value1;

	value0 = sdhci_readl(host, PHY_COMMDL_CNFG_REG_OFFSET);
	value1 = sdhci_readl(host, PHY_SMPLDL_CNFG_REG_OFFSET);

	value0 &= ~(0x1f03);
	sdhci_writel(host, value0, PHY_COMMDL_CNFG_REG_OFFSET);

	value1 &= ~(0x313);
	value1 |= 0xc0c;
	sdhci_writel(host, value1, PHY_SMPLDL_CNFG_REG_OFFSET);

	value0 = sdhci_readl(host, PHY_COMMDL_CNFG_REG_OFFSET);
	value0 |= 0x1000;
	sdhci_writel(host, value0, PHY_COMMDL_CNFG_REG_OFFSET);
	value0 |= 0x7f0000;
	sdhci_writel(host, value0, PHY_COMMDL_CNFG_REG_OFFSET);

	value0 = sdhci_readl(host, PHY_COMMDL_CNFG_REG_OFFSET);
	value0 &= ~0x1000;
	sdhci_writel(host, value0, PHY_COMMDL_CNFG_REG_OFFSET);
}

static void SDHC_PHYTuningSetup(struct udevice *dev)
{
	struct sdhci_host *host = dev_get_priv(dev);
	unsigned int value;

	value = sdhci_readl(host, PHY_AT_CTRL_R_REG_OFFSET);
	value &= ~TUNE_CLK_STOP_EN & ~POST_CHANGE_DLY & ~PRE_CHANGE_DLY;
	value |= (1<<16) | (3<<17) | (3<<19);
	sdhci_writel(host, value, PHY_AT_CTRL_R_REG_OFFSET);
}

static int dwcmshc_sdhci_probe(struct udevice *dev)
{
	struct dwcmshc_sdhci_plat *plat = dev_get_platdata(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	int ret = 0;
	int val;
	struct clk clk;
	unsigned long clock;

	dwcmshc_sdhci_reset(plat, host);
	ret = clk_get_by_index(dev, 0, &clk);
	if (ret < 0) {
		dev_err(dev, "failed to get clock\n");
		return ret;
	}

	clock = clk_get_rate(&clk);
	if (IS_ERR_VALUE(clock)) {
		dev_err(dev, "failed to get rate\n");
		return clock;
	}

	host->max_clk = clock;
	host->mmc = &plat->mmc;
	host->mmc->dev = dev;

	host->quirks = SDHCI_QUIRK_NO_1_8_V;
	if (dev_read_bool(dev, "1_8v-signalling"))
		host->mmc->signal_voltage = MMC_SIGNAL_VOLTAGE_180;

	ret = sdhci_setup_cfg(&plat->cfg, host, plat->cfg.f_max, 0);
	if (ret)
		return ret;

	upriv->mmc = host->mmc;
	host->mmc->priv = host;

	dwcmshc_sdhci_slot_init(host);
	ret = sdhci_probe(dev);
	if(ret)
		return ret;

	if(plat->num_phy_setting && plat->phy_setting) {
		SDHC_PHYDelayLineSetup(dev);
		SDHC_PHYTuningSetup(dev);
		val = sdhci_readl(host, pad_general_1v8.addr);
		val &= ~(pad_general_1v8.mask<<pad_general_1v8.sp_bit);
		val |= (pad_general_1v8.sp_value<<pad_general_1v8.sp_bit);
		val &= ~(pad_general_1v8.mask<<pad_general_1v8.sn_bit);
		val |= (pad_general_1v8.sn_value<<pad_general_1v8.sn_bit);
		sdhci_writel(host, val, pad_general_1v8.addr);
		ret = dwcmsh_PHYconfig(host, plat->phy_setting, plat->num_phy_setting);
		dwcmsh_PHYreset(host, 1);
	}

	return ret;
}

static int dwcmshc_sdhci_ofdata_to_platdata(struct udevice *dev)
{
	struct dwcmshc_sdhci_plat *plat = dev_get_platdata(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	struct mmc_config *cfg = &plat->cfg;
	int ret;
	int len, i, j;
	const unsigned int * ps = 0;

	if (reset_get_bulk(dev, &plat->reset_ctl)) {
		printf("%s: can't get reset index for xenon_sdhci\n", __func__);
		return -ENODEV;
	}
	host->name = dev->name;
	host->ioaddr = (void *)dev_read_addr(dev);

	ps = dev_read_prop(dev, "phy-setting", &len);
	if(ps) {
		if(len % sizeof(struct phy_setting)) {
			printf("%s: phy setting is not correct\n", __func__);
			return -EINVAL;
		}
		plat->num_phy_setting = len / sizeof(struct phy_setting);
		plat->phy_setting = malloc(len);
		for(i = 0, j = 0; i < plat->num_phy_setting ; i++) {
			plat->phy_setting[i].addr = fdt32_to_cpu(ps[j++]);
			plat->phy_setting[i].startbit = fdt32_to_cpu(ps[j++]);
			plat->phy_setting[i].mask = fdt32_to_cpu(ps[j++]);
			plat->phy_setting[i].value = fdt32_to_cpu(ps[j++]);
		}
	}

	ret = mmc_of_parse(dev, cfg);
	if (ret)
		return ret;

	host->ops = &dwcmshc_ops;

	return 0;
}

static int dwcmshc_sdhci_bind(struct udevice *dev)
{
	struct dwcmshc_sdhci_plat *plat = dev_get_platdata(dev);
	int ret;

	ret = sdhci_bind(dev, &plat->mmc, &plat->cfg);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id dwcmshc_sdhci_ids[] = {
	{ .compatible = "snps,dwcmshc-sdhci" },
	{ }
};

U_BOOT_DRIVER(dwcmshc_sdhci) = {
	.name	= "dwcmshc-sdhci",
	.id	= UCLASS_MMC,
	.of_match = dwcmshc_sdhci_ids,
	.ofdata_to_platdata = dwcmshc_sdhci_ofdata_to_platdata,
	.ops	= &sdhci_ops,
	.bind	= dwcmshc_sdhci_bind,
	.probe	= dwcmshc_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct sdhci_host),
	.platdata_auto_alloc_size = sizeof(struct dwcmshc_sdhci_plat),
};
