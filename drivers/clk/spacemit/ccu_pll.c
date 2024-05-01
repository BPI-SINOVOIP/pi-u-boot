// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit clock type pll
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
#include "ccu_pll.h"

#define PLL_MIN_FREQ 600000000
#define PLL_MAX_FREQ 3400000000
#define PLL_DELAYTIME 590 //(590*5)us

#define pll_readl(reg)		readl(reg)
#define pll_readl_pll_swcr1(p)	pll_readl(p.base + p.reg_ctrl)
#define pll_readl_pll_swcr2(p)	pll_readl(p.base + p.reg_sel)
#define pll_readl_pll_swcr3(p)	pll_readl(p.base + p.reg_xtc)

#define pll_writel(val, reg)		writel(val, reg)
#define pll_writel_pll_swcr1(val, p)	pll_writel(val, p.base + p.reg_ctrl)
#define pll_writel_pll_swcr2(val, p)	pll_writel(val, p.base + p.reg_sel)
#define pll_writel_pll_swcr3(val, p)	pll_writel(val, p.base + p.reg_xtc)

/* unified pllx_swcr1 for pll1~3 */
union pllx_swcr1 {
	struct {
		unsigned int reg5:8;
		unsigned int reg6:8;
		unsigned int reg7:8;
		unsigned int reg8:8;
	} b;
	unsigned int v;
};

/* unified pllx_swcr2 for pll1~3 */
union pllx_swcr2 {
	struct {
		unsigned int div1_en:1;
		unsigned int div2_en:1;
		unsigned int div3_en:1;
		unsigned int div4_en:1;
		unsigned int div5_en:1;
		unsigned int div6_en:1;
		unsigned int div7_en:1;
		unsigned int div8_en:1;
		unsigned int reserved1:4;
		unsigned int atest_en:1;
		unsigned int cktest_en:1;
		unsigned int dtest_en:1;
		unsigned int rdo:2;
		unsigned int mon_cfg:4;
		unsigned int reserved2:11;
	} b;
	unsigned int v;
};

/* unified pllx_swcr3 for pll1~3 */
union pllx_swcr3{
	struct {
		unsigned int div_frc:24;
		unsigned int div_int:7;
		unsigned int pll_en:1;
	} b;

	unsigned int v;
};

static int ccu_pll_is_enabled(struct clk *clk)
{
	struct ccu_pll *p = clk_to_ccu_pll(clk);
	union pllx_swcr3 swcr3;
	unsigned int enabled;

	swcr3.v = pll_readl_pll_swcr3(p->common);
	enabled = swcr3.b.pll_en;

	return enabled;
}

/* frequency unit Mhz, return pll vco freq */
static ulong __get_vco_freq(struct clk *clk)
{
	unsigned int reg5, reg6, reg7, reg8, size, i;
	unsigned int div_int, div_frc;
	struct ccu_pll_rate_tbl *freq_pll_regs_table;
	struct ccu_pll *p = clk_to_ccu_pll(clk);
	union pllx_swcr1 swcr1;
	union pllx_swcr3 swcr3;

	swcr1.v = pll_readl_pll_swcr1(p->common);
	swcr3.v = pll_readl_pll_swcr3(p->common);

	reg5 = swcr1.b.reg5;
	reg6 = swcr1.b.reg6;
	reg7 = swcr1.b.reg7;
	reg8 = swcr1.b.reg8;

	div_int = swcr3.b.div_int;
	div_frc = swcr3.b.div_frc;

	freq_pll_regs_table = p->pll.rate_tbl;
	size = p->pll.tbl_size;

	for (i = 0; i < size; i++) {
		if ((freq_pll_regs_table[i].reg5 == reg5)
		&& (freq_pll_regs_table[i].reg6 == reg6)
		&& (freq_pll_regs_table[i].reg7 == reg7)
		&& (freq_pll_regs_table[i].reg8 == reg8)
		&& (freq_pll_regs_table[i].div_int == div_int)
		&& (freq_pll_regs_table[i].div_frac == div_frc))
			return freq_pll_regs_table[i].rate;
    }

	pr_info("Unknown rate for clock\n");
	return 0;
}

static int ccu_pll_enable(struct clk *clk)
{
	unsigned int delaytime = PLL_DELAYTIME;
	unsigned long flags;
	struct ccu_pll *p = clk_to_ccu_pll(clk);
	union pllx_swcr3 swcr3;

	if (ccu_pll_is_enabled(clk))
		return 0;

	spin_lock_irqsave(p->common.lock, flags);
	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.pll_en = 1;
	pll_writel_pll_swcr3(swcr3.v, p->common);
	spin_unlock_irqrestore(p->common.lock, flags);

	/* check lock status */
	udelay(50);

	while ((!(readl(p->pll.lock_base + p->pll.reg_lock) & p->pll.lock_enable_bit))
	       && delaytime) {
		udelay(5);
		delaytime--;
	}
	if (unlikely(!delaytime)) {
		pr_err("ccu_pll_enable enabling didn't get stable within 3000us!!!\n" );
	}

	return 0;
}

static int ccu_pll_disable(struct clk *clk)
{
	struct ccu_pll *p = clk_to_ccu_pll(clk);
	union pllx_swcr3 swcr3;

	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.pll_en = 0;
	pll_writel_pll_swcr3(swcr3.v, p->common);
	return 0;

}

/*
 * pll rate change requires sequence:
 * clock off -> change rate setting -> clock on
 * This function doesn't really change rate, but cache the config
 */
static ulong ccu_pll_set_rate(struct clk *clk, ulong rate)
{
	unsigned int i, reg5 = 0, reg6 = 0, reg7 = 0, reg8 = 0;
	unsigned int div_int, div_frc;
	unsigned long old_rate;
	struct ccu_pll *p = clk_to_ccu_pll(clk);
	struct ccu_pll_config *params = &p->pll;
	union pllx_swcr1 swcr1;
	union pllx_swcr3 swcr3;
	bool found = false;

	if (ccu_pll_is_enabled(clk)) {
		pr_info("%s is enabled, ignore the setrate!\n", clk->dev->name);
		return 0;
	}

	old_rate = __get_vco_freq(clk);
	/* setp 1: calculate fbd frcd kvco and band */
	if (params->rate_tbl) {
		for (i = 0; i < params->tbl_size; i++) {
			if (rate == params->rate_tbl[i].rate) {
				found = true;

				reg5 = params->rate_tbl[i].reg5;
				reg6 = params->rate_tbl[i].reg6;
				reg7 = params->rate_tbl[i].reg7;
				reg8 = params->rate_tbl[i].reg8;
				div_int = params->rate_tbl[i].div_int;
				div_frc = params->rate_tbl[i].div_frac;
				break;
			}
		}

	} else {
		pr_err("don't find freq table for pll\n");
		return -EINVAL;
	}

	/* setp 2: set pll kvco/band and fbd/frcd setting */
	swcr1.v = pll_readl_pll_swcr1(p->common);
	swcr1.b.reg5 = reg5;
	swcr1.b.reg6 = reg6;
	swcr1.b.reg7 = reg7;
	swcr1.b.reg8 = reg8;
	pll_writel_pll_swcr1(swcr1.v, p->common);

	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.div_int = div_int;
	swcr3.b.div_frc = div_frc;
	pll_writel_pll_swcr3(swcr3.v, p->common);

	return 0;
}

static ulong ccu_pll_get_rate(struct clk *clk)
{
	ulong val;
	val = __get_vco_freq(clk);
	return val;
}

static ulong ccu_pll_round_rate(struct clk *clk, ulong rate)
{
	struct ccu_pll *p = clk_to_ccu_pll(clk);
	unsigned long max_rate = 0;
	unsigned int i;
	struct ccu_pll_config *params = &p->pll;

	if (rate > PLL_MAX_FREQ || rate < PLL_MIN_FREQ) {
		pr_err("%lu rate out of range!\n", rate);
		return -EINVAL;
	}

	if (params->rate_tbl) {
		for (i = 0; i < params->tbl_size; i++) {
			if (params->rate_tbl[i].rate <= rate) {
				if (max_rate < params->rate_tbl[i].rate)
					max_rate = params->rate_tbl[i].rate;
			}
		}
	} else {
		pr_info("don't find freq table for pll\n");
	}
	return max_rate;
}

const struct clk_ops ccu_pll_ops = {
	.enable 	= ccu_pll_enable,
	.disable 	= ccu_pll_disable,
	.set_rate 	= ccu_pll_set_rate,
	.get_rate 	= ccu_pll_get_rate,
	.round_rate	= ccu_pll_round_rate,
};

U_BOOT_DRIVER(ccu_clk_pll) = {
	.name	= CCU_CLK_PLL,
	.id	= UCLASS_CLK,
	.ops	= &ccu_pll_ops,
};

