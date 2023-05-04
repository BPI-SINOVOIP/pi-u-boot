// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for SUNPLUS PHYs
 *
 * Based code from drivers/net/phy/sunplus.c of Linux kernel
 * Copyright (C) 2020 Synaptics Incorporated
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <common.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <phy.h>
#include <log.h>
#include <dm/ofnode.h>


#define SUNPLUS_PAGE_SELECT	31
#define LINKUP_TIMEOUT 5000 /* 5s for link up timeout */

static int sunplus_write_page(struct phy_device *phydev, int page)
{
	return phy_write(phydev, MDIO_DEVAD_NONE, SUNPLUS_PAGE_SELECT, (page & 0x1f) << 8);
}

static int sunplus_config(struct phy_device *phydev)
{
	int count;
	phydev->speed = SPEED_100;
	phydev->duplex = DUPLEX_HALF;
	phydev->autoneg = AUTONEG_DISABLE;

	sunplus_write_page(phydev, 0x6);
	/* CP current optimization for PLL */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x17, 0x0545);

	sunplus_write_page(phydev, 0x1);
	/* disable aps */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x12, 0x4824);

	sunplus_write_page(phydev, 0x2);
	/* 10Base-T filter selector */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x18, 0x0000);

	sunplus_write_page(phydev, 0x6);
	/* PHYAFE TX optimization and invert ADC clock */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x19, 0x004c);
	/* Tx_level optimization */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x15, 0x3030);
	/* PD enable control */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1c, 0x8880);

	sunplus_write_page(phydev, 0x8);
	/* disable a_TX_LEVEL_Auto calibration */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x0844);

	sunplus_write_page(phydev, 0x0);
	/* speed selectoin=100 Mbps, duplex = half */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0, 0x2000);

	genphy_config_aneg(phydev);

	count = LINKUP_TIMEOUT/100;
	while (count-- && !(phy_read(phydev, MDIO_DEVAD_NONE, 0x1) & 0x4))
		mdelay(100);

	if (!count) {
		printf("WARING: phy link is down\n");
	}

	return 0;
}

static struct phy_driver spitsi_driver = {
	.name = "SUNPLUS SPI2TSI",
	.uid = 0x00441400,
	.mask = 0xffffff,
	.features = PHY_BASIC_FEATURES,
	.config = &sunplus_config,
	.startup = &genphy_startup,
	.shutdown = &genphy_shutdown,
};

int phy_sunplus_init(void)
{
	phy_register(&spitsi_driver);
	return 0;
}
