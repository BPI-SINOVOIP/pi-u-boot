/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023, Spacemit
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <sysreset.h>
#include <asm/io.h>
#include <linux/err.h>
#include <linux/delay.h>

/*wdt*/
#define K1X_WDT_REG (0xd4080000)
#define K1X_WDT_ENABLE (0xd4080000 + 0xb8)
#define K1X_WDT_TIMEOUT (0xd4080000 + 0xbc)
#define K1X_WDT_RESET (0xd4080000 + 0xc8)
#define K1X_WDT_STATUS (0xd4080000 + 0xc0)
#define K1X_WDT_WSAR        (0xd4080000 + 0x00b4)
#define K1X_WDT_WFAR        (0xd4080000 + 0x00b0)

#define WDT_CLEAR_STATUS (0x0)
#define WDT_RESET_ENABLE (0x1)
#define WDT_ENABLE (0x3)
#define WDT_TIMEOUT (0x1)

#define K1X_WDT_START_REG  (0xd4051020)
#define K1X_WDT_START_ENABLE  (BIT(4))

#define K1X_WDT_CLK_RESET_REG (0xd4050200)
#define K1X_WDT_MPU_REG (0xd4051024)
#define K1X_WDT_CLK_RESET_ENABLE (0x3)
#define K1X_WDT_CLK_RESET_FLAG (BIT(2))
#define K1X_WDT_CLK_ENABLE_MPU (BIT(19))

static void spa_wdt_write_access(void)
{
	writel(0xbaba, (void *)K1X_WDT_WFAR);
	writel(0xeb10, (void *)K1X_WDT_WSAR);
}

static void spa_wdt_write(u32 val, void *reg)
{
	spa_wdt_write_access();
	writel(val, reg);
}

static void enable_wdt(void)
{
	u32 reg;

	/*enable wdt clk reset*/
	reg = readl((void *)K1X_WDT_MPU_REG);
	writel(K1X_WDT_CLK_ENABLE_MPU | reg, (void *)K1X_WDT_MPU_REG);

	reg = readl((void *)K1X_WDT_CLK_RESET_REG);
	writel(K1X_WDT_CLK_RESET_ENABLE | reg, (void *)K1X_WDT_CLK_RESET_REG);
	reg = readl((void *)K1X_WDT_CLK_RESET_REG);
	writel((~K1X_WDT_CLK_RESET_FLAG) & reg,(void *)K1X_WDT_CLK_RESET_REG);


	/*set watch dog*/
	spa_wdt_write(WDT_CLEAR_STATUS, (void *)K1X_WDT_STATUS);
	spa_wdt_write(WDT_TIMEOUT, (void *)K1X_WDT_TIMEOUT);
	spa_wdt_write(WDT_ENABLE, (void *)K1X_WDT_ENABLE);
	spa_wdt_write(WDT_RESET_ENABLE, (void *)K1X_WDT_RESET);

	reg = readl((void*)K1X_WDT_START_REG);
	writel(K1X_WDT_START_ENABLE | reg, (void *)K1X_WDT_START_REG);
}

static int spacemit_sysreset_request(struct udevice *dev, enum sysreset_t type)
{
	enable_wdt();
	/*wait for reset*/
	mdelay(5000);

	/*if reset success, it would never return*/
	return -EINPROGRESS;
}

static int spacemit_sysreset_probe(struct udevice *dev)
{
	return 0;
}


static struct sysreset_ops spacemit_sysreset = {
	.request	= spacemit_sysreset_request,
};

U_BOOT_DRIVER(spacemit_sysreset) = {
	.name	= "spacemit_sysreset",
	.id	= UCLASS_SYSRESET,
	.ops	= &spacemit_sysreset,
	.probe  = spacemit_sysreset_probe,
};
