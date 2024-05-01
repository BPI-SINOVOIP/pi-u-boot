// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit clock type ddn
 *
 * Copyright (c) 2023, spacemit Corporation.
 *
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <clk-uclass.h>
#include <dm/device.h>
#include <dm/devres.h>
#include <linux/bitops.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <clk.h>
#include <div64.h>

#include "ccu_ddn.h"
/*
 * It is M/N clock
 *
 * Fout from synthesizer can be given from two equations:
 * numerator/denominator = Fin / (Fout * factor)
 */

static int ccu_ddn_disable(struct clk *clk)
{
	struct ccu_ddn *ddn = clk_to_ccu_ddn(clk);
	struct ccu_common * common = &ddn->common;
	u32 reg;

	if (!ddn->gate)
		return 0;

	reg = readl(common->base + common->reg_sel);

	writel(reg & ~ddn->gate, common->base + common->reg_sel);
	return 0;

}

static int ccu_ddn_enable(struct clk *clk)
{
	struct ccu_ddn *ddn = clk_to_ccu_ddn(clk);
	struct ccu_common * common = &ddn->common;
	u32 reg;

	if (!ddn->gate)
		return 0;

	reg = readl(common->base + common->reg_sel);

	writel(reg | ddn->gate, common->base + common->reg_sel);

	return 0;
}

static ulong clk_ddn_round_rate(struct clk *clk, ulong drate)
{
	struct ccu_ddn *ddn = clk_to_ccu_ddn(clk);
	struct ccu_ddn_config *params = &ddn->ddn;
	unsigned long parent_rate = clk_get_parent_rate(clk);
	unsigned long rate = 0, prev_rate;
	unsigned long result;
	int i;

	for (i = 0; i < params->tbl_size; i++) {
		prev_rate = rate;
		rate = (((parent_rate / 10000) * params->tbl[i].den) /
			(params->tbl[i].num * params->info->factor)) * 10000;
		if (rate > drate)
			break;
	}
	if ((i == 0) || (i == params->tbl_size)) {
		result = rate;
	} else {
		if ((drate - prev_rate) > (rate - drate))
			result = rate;
		else
			result = prev_rate;
	}
	return result;
}

static ulong clk_ddn_get_rate(struct clk *clk)
{
	struct ccu_ddn *ddn = clk_to_ccu_ddn(clk);
	struct ccu_ddn_config *params = &ddn->ddn;
	unsigned long parent_rate = clk_get_parent_rate(clk);
	unsigned int val, num, den;
	unsigned long rate;

	val = readl(ddn->common.base + ddn->common.reg_ctrl);

	/* calculate numerator */
	num = (val >> params->info->num_shift) & params->info->num_mask;

	/* calculate denominator */
	den = (val >> params->info->den_shift) & params->info->den_mask;
	if (!den)
		return 0;
	rate = (((parent_rate / 10000)  * den) /
			(num * params->info->factor)) * 10000;
	return rate;
}

/* Configures new clock rate*/
static ulong clk_ddn_set_rate(struct clk *clk, unsigned long drate)
{
	struct ccu_ddn *ddn = clk_to_ccu_ddn(clk);
	struct ccu_ddn_config *params = &ddn->ddn;
	unsigned long prate = clk_get_parent_rate(clk);
	int i;
	unsigned long val;
	unsigned long prev_rate, rate = 0;

	for (i = 0; i < params->tbl_size; i++) {
		prev_rate = rate;
		rate = (((prate / 10000) * params->tbl[i].den) /
			(params->tbl[i].num * params->info->factor)) * 10000;
		if (rate > drate)
			break;
	}

	if (i > 0)
		i--;

	val = readl(ddn->common.base + ddn->common.reg_ctrl);

	val &= ~(params->info->num_mask << params->info->num_shift);
	val |= (params->tbl[i].num & params->info->num_mask) << params->info->num_shift;

	val &= ~(params->info->den_mask << params->info->den_shift);
	val |= (params->tbl[i].den & params->info->den_mask) << params->info->den_shift;

	writel(val, ddn->common.base + ddn->common.reg_ctrl);

	return 0;
}

const struct clk_ops ccu_ddn_ops = {
	.disable 	= ccu_ddn_disable,
	.enable 	= ccu_ddn_enable,
	.get_rate 	= clk_ddn_get_rate,
	.round_rate 	= clk_ddn_round_rate,
	.set_rate 	= clk_ddn_set_rate,
};

U_BOOT_DRIVER(ccu_clk_ddn) = {
	.name	= CCU_CLK_DDN,
	.id	= UCLASS_CLK,
	.ops	= &ccu_ddn_ops,
};

