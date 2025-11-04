/*
 * Renesas R-Car Gen3 for USB2.0 PHY driver
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * This is based on the phy-rcar-gen2 driver:
 * Copyright (C) 2014 Renesas Solutions Corp.
 * Copyright (C) 2014 Cogent Embedded, Inc.
 *
 * Ported to u-boot
 * Copyright (C) 2016 GlobalLogic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "common.h"
#include <asm/io.h>


/******* USB2.0 Host registers (original offset is +0x200) *******/
#define USB2_INT_ENABLE		0x000
#define USB2_USBCTR		0x00c
#define USB2_SPD_RSM_TIMSET	0x10c
#define USB2_OC_TIMSET		0x110
#define USB2_COMMCTRL		0x600
#define USB2_OBINTSTA		0x604
#define USB2_OBINTEN		0x608
#define USB2_VBCTRL		0x60c
#define USB2_LINECTRL1		0x610
#define USB2_ADPCTRL		0x630

/* INT_ENABLE */
#define USB2_INT_ENABLE_UCOM_INTEN	BIT(3)
#define USB2_INT_ENABLE_USBH_INTB_EN	BIT(2)
#define USB2_INT_ENABLE_USBH_INTA_EN	BIT(1)
#define USB2_INT_ENABLE_INIT		(USB2_INT_ENABLE_UCOM_INTEN | \
					 USB2_INT_ENABLE_USBH_INTB_EN | \
					 USB2_INT_ENABLE_USBH_INTA_EN)

/* USBCTR */
#define USB2_USBCTR_DIRPD	BIT(2)
#define USB2_USBCTR_PLL_RST	BIT(1)

/* SPD_RSM_TIMSET */
#define USB2_SPD_RSM_TIMSET_INIT	0x014e029b

/* OC_TIMSET */
#define USB2_OC_TIMSET_INIT		0x000209ab

/* COMMCTRL */
#define USB2_COMMCTRL_OTG_PERI		BIT(31)	/* 1 = Peripheral mode */

/* OBINTSTA and OBINTEN */
#define USB2_OBINT_SESSVLDCHG		BIT(12)
#define USB2_OBINT_IDDIGCHG		BIT(11)
#define USB2_OBINT_BITS			(USB2_OBINT_SESSVLDCHG | \
					 USB2_OBINT_IDDIGCHG)

/* VBCTRL */
#define USB2_VBCTRL_DRVVBUSSEL		BIT(8)

/* LINECTRL1 */
#define USB2_LINECTRL1_DPRPD_EN		BIT(19)
#define USB2_LINECTRL1_DP_RPD		BIT(18)
#define USB2_LINECTRL1_DMRPD_EN		BIT(17)
#define USB2_LINECTRL1_DM_RPD		BIT(16)

/* ADPCTRL */
#define USB2_ADPCTRL_OTGSESSVLD		BIT(20)
#define USB2_ADPCTRL_IDDIG		BIT(19)
#define USB2_ADPCTRL_IDPULLUP		BIT(5)	/* 1 = ID sampling is enabled */
#define USB2_ADPCTRL_DRVVBUS		BIT(4)

static void rcar_gen3_set_host_mode(struct rcar_gen3_chan *ch, int host)
{
	void __iomem *usb2_base = ch->usb2.base;
	u32 val = readl(usb2_base + USB2_COMMCTRL);

	dev_vdbg(&ch->phy->dev, "%s: %08x, %d\n", __func__, val, host);
	if (host)
		val &= ~USB2_COMMCTRL_OTG_PERI;
	else
		val |= USB2_COMMCTRL_OTG_PERI;
	writel(val, usb2_base + USB2_COMMCTRL);
}

static void rcar_gen3_set_linectrl(struct rcar_gen3_chan *ch, int dp, int dm)
{
	void __iomem *usb2_base = ch->usb2.base;
	u32 val = readl(usb2_base + USB2_LINECTRL1);

	dev_vdbg(&ch->phy->dev, "%s: %08x, %d, %d\n", __func__, val, dp, dm);
	val &= ~(USB2_LINECTRL1_DP_RPD | USB2_LINECTRL1_DM_RPD);
	if (dp)
		val |= USB2_LINECTRL1_DP_RPD;
	if (dm)
		val |= USB2_LINECTRL1_DM_RPD;
	writel(val, usb2_base + USB2_LINECTRL1);
}

static void rcar_gen3_enable_vbus_ctrl(struct rcar_gen3_chan *ch, int vbus)
{
	void __iomem *usb2_base = ch->usb2.base;
	u32 val = readl(usb2_base + USB2_ADPCTRL);

	dev_vdbg(&ch->phy->dev, "%s: %08x, %d\n", __func__, val, vbus);
	if (vbus)
		val |= USB2_ADPCTRL_DRVVBUS;
	else
		val &= ~USB2_ADPCTRL_DRVVBUS;
	writel(val, usb2_base + USB2_ADPCTRL);
}

static void rcar_gen3_init_for_host(struct rcar_gen3_chan *ch)
{
	pr_dbg("++%s\n", __func__);
	rcar_gen3_set_linectrl(ch, 1, 1);
	rcar_gen3_set_host_mode(ch, 1);
	rcar_gen3_enable_vbus_ctrl(ch, 1);
	pr_dbg("--%s\n", __func__);
}

static void rcar_gen3_init_for_peri(struct rcar_gen3_chan *ch)
{
	pr_dbg("++%s\n", __func__);

	rcar_gen3_set_linectrl(ch, 0, 1);
	rcar_gen3_set_host_mode(ch, 0);
	rcar_gen3_enable_vbus_ctrl(ch, 0);

	pr_dbg("--%s\n", __func__);
}

static bool rcar_gen3_check_vbus(struct rcar_gen3_chan *ch)
{
	pr_dbg("+-%s\n", __func__);

	return !!(readl(ch->usb2.base + USB2_ADPCTRL) &
		  USB2_ADPCTRL_OTGSESSVLD);
}

static bool rcar_gen3_check_id(struct rcar_gen3_chan *ch)
{
	pr_dbg("+-%s\n", __func__);
	return !!(readl(ch->usb2.base + USB2_ADPCTRL) & USB2_ADPCTRL_IDDIG);
}

static void rcar_gen3_device_recognition(struct rcar_gen3_chan *ch)
{
	bool is_host = true;

	pr_dbg("++%s\n", __func__);

	/* B-device? */
	if (rcar_gen3_check_id(ch) && rcar_gen3_check_vbus(ch))
		is_host = false;

	/* FIXME: RZ/G2L does NOT have vbus control.
	 * So the checking always return host mode.
	 * We only use peripheral mode. So we just force
	 * set is_host to false */
	is_host = false;
	if (is_host)
		rcar_gen3_init_for_host(ch);
	else
		rcar_gen3_init_for_peri(ch);
	pr_dbg("--%s\n", __func__);
}

static void rcar_gen3_init_otg(struct rcar_gen3_chan *ch)
{
	void __iomem *usb2_base = ch->usb2.base;
	u32 val;

	pr_dbg("++%s\n", __func__);

	val = readl(usb2_base + USB2_VBCTRL);
	writel(val | USB2_VBCTRL_DRVVBUSSEL, usb2_base + USB2_VBCTRL);
	writel(USB2_OBINT_BITS, usb2_base + USB2_OBINTSTA);
	val = readl(usb2_base + USB2_OBINTEN);
	writel(val | USB2_OBINT_BITS, usb2_base + USB2_OBINTEN);
	val = readl(usb2_base + USB2_ADPCTRL);
	writel(val | USB2_ADPCTRL_IDPULLUP, usb2_base + USB2_ADPCTRL);
	val = readl(usb2_base + USB2_LINECTRL1);
	rcar_gen3_set_linectrl(ch, 0, 0);
	writel(val | USB2_LINECTRL1_DPRPD_EN | USB2_LINECTRL1_DMRPD_EN,
	       usb2_base + USB2_LINECTRL1);

	rcar_gen3_device_recognition(ch);
	pr_dbg("--%s\n", __func__);
}

static int rcar_gen3_phy_usb2_init(struct phy *p)
{
	struct rcar_gen3_chan *channel = phy_get_drvdata(p);
	void __iomem *usb2_base = channel->usb2.base;

	pr_dbg("++%s\n", __func__);

	/* Initialize USB2 part */
	writel(USB2_INT_ENABLE_INIT, usb2_base + USB2_INT_ENABLE);
	writel(USB2_SPD_RSM_TIMSET_INIT, usb2_base + USB2_SPD_RSM_TIMSET);
	writel(USB2_OC_TIMSET_INIT, usb2_base + USB2_OC_TIMSET);

	/* Initialize otg part */
	if (channel->has_otg)
		rcar_gen3_init_otg(channel);

	pr_dbg("--%s\n", __func__);

	return 0;
}

static int rcar_gen3_phy_usb2_exit(struct phy *p)
{
	struct rcar_gen3_chan *channel = phy_get_drvdata(p);

	pr_dbg("+-%s\n", __func__);

	writel(0, channel->usb2.base + USB2_INT_ENABLE);

	return 0;
}

static int rcar_gen3_phy_usb2_power_on(struct phy *p)
{
	struct rcar_gen3_chan *channel = phy_get_drvdata(p);
	void __iomem *usb2_base = channel->usb2.base;
	u32 val;

	pr_dbg("++%s\n", __func__);

	val = readl(usb2_base + USB2_USBCTR);
	val |= USB2_USBCTR_PLL_RST;
	writel(val, usb2_base + USB2_USBCTR);
	val &= ~USB2_USBCTR_PLL_RST;
	writel(val, usb2_base + USB2_USBCTR);

	pr_dbg("--%s\n", __func__);

	return 0;
}

static struct phy_ops rcar_gen3_phy_usb2_ops = {
	.init		= rcar_gen3_phy_usb2_init,
	.exit		= rcar_gen3_phy_usb2_exit,
	.power_on	= rcar_gen3_phy_usb2_power_on,
	.owner		= THIS_MODULE,
};

static irqreturn_t rcar_gen3_phy_usb2_irq(int irq, void *_ch)
{
	struct rcar_gen3_chan *ch = _ch;
	void __iomem *usb2_base = ch->usb2.base;
	u32 status = readl(usb2_base + USB2_OBINTSTA);
	irqreturn_t ret = IRQ_NONE;

	if (status & USB2_OBINT_BITS) {
		dev_vdbg(&ch->phy->dev, "%s: %08x\n", __func__, status);
		writel(USB2_OBINT_BITS, usb2_base + USB2_OBINTSTA);
		rcar_gen3_device_recognition(ch);
		ret = IRQ_HANDLED;
	}

	return ret;
}

int rcar_gen3_phy_usb2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rcar_gen3_chan *channel;
	int irq = 0;

	pr_dbg("++ %s(%s)\n", __func__, pdev->name);

	channel = kzalloc(sizeof(*channel), GFP_KERNEL);
	if (!channel)
		return -ENOMEM;

	channel->usb2.base = (void *)PHY_BASE;
	channel->has_otg = true;
	irq = devm_request_irq(dev, irq, rcar_gen3_phy_usb2_irq,
			       0, RCAR3_PHY_DEVICE, channel);
	if (irq < 0)
		dev_err(dev, "No irq handler (%d)\n", irq);
	channel->has_otg = true;

	/* devm_phy_create() will call pm_runtime_enable(dev); */

	channel->phy = devm_phy_create(dev, NULL, &rcar_gen3_phy_usb2_ops);
	if (IS_ERR(channel->phy)) {
		dev_err(dev, "Failed to create USB2 PHY\n");
		return PTR_ERR(channel->phy);
	}

	phy_set_drvdata(channel->phy, channel);
	pdev->dev_platform_data = (void *)channel;

	pr_dbg("--%s\n", __func__);
	return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Renesas R-Car Gen3 USB 2.0 PHY");
MODULE_AUTHOR("Yoshihiro Shimoda <yoshihiro.shimoda.uh@renesas.com>");

