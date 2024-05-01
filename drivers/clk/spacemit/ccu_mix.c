// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit clock type mix(div/mux/gate/factor)
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

#include "ccu_mix.h"

#define TIMEOUT_LIMIT (20000) /* max timeout 10000us */
static int twsi8_reg_val = 0x04;

static int ccu_mix_trigger_fc(struct clk *clk)
{
#ifdef CONFIG_SPL_BUILD
	return 0;
#else
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	unsigned long val = 0;

	int ret = 0, timeout = 50;

	if (common->reg_type == CLK_DIV_TYPE_1REG_FC_V2
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_DIV_V5
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_MUX_V6) {

		timeout = 50;
		val = readl(common->base + common->reg_ctrl);
		val |= common->fc;
		writel(val, common->base + common->reg_ctrl);

		do {
			val = readl(common->base + common->reg_ctrl);
			timeout--;
			if (!(val & (common->fc)))
				break;
		} while (timeout);

		if (timeout == 0) {
			timeout = 5000;
			do {
				val = readl(common->base + common->reg_ctrl);
				timeout--;
				if (!(val & (common->fc)))
					break;
			} while (timeout);
			if (timeout != 0) {
				ret = 0;

			} else {
				ret = -1;
			}
		}
	}

	return ret;
#endif
}

#ifndef CONFIG_SPL_BUILD
static int ccu_mix_disable(struct clk *clk)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	u32 tmp;

	if (!gate)
		return 0;

#ifdef CONFIG_SPL_BUILD
	if (clk->id == CLK_TWSI8_SPL){
#else
	if (clk->id == CLK_TWSI8){
#endif
		twsi8_reg_val &= ~0x7;
		twsi8_reg_val |= 0x4;
		tmp = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
			writel(tmp, common->base + common->reg_sel);
		else
			writel(tmp, common->base + common->reg_ctrl);

		return 0;
	}

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		tmp = readl(common->base + common->reg_sel);
	else
		tmp = readl(common->base + common->reg_ctrl);

	tmp &= ~gate->gate_mask;
	tmp |= gate->val_disable;

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		writel(tmp, common->base + common->reg_sel);
	else
		writel(tmp, common->base + common->reg_ctrl);

	if (gate->flags & SPACEMIT_CLK_GATE_NEED_DELAY) {
		udelay(200);
	}

	return 0;
}

static ulong ccu_mix_round_rate(struct clk *clk, ulong rate)
{
	return rate;
}

static int ccu_mix_set_parent(struct clk *clk, struct clk *parent)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_mux_config *mux = mix->mux;
	int index;
	u32 reg, i;
	int ret;

	if (!parent)
		return -EINVAL;

	for (i = 0; i < common->num_parents; i++) {
		if (!strcmp(parent->dev->name, common->parent_names[i])){
			index = i;
			break;
		}
	}

	if (index < 0) {
		pr_info("Could not fetch index\n");
		return index;
	}

#ifdef CONFIG_SPL_BUILD
	if (clk->id == CLK_TWSI8_SPL){
#else
	if (clk->id == CLK_TWSI8){
#endif
		twsi8_reg_val &= ~GENMASK(mux->width + mux->shift - 1, mux->shift);
		twsi8_reg_val |= (index << mux->shift);
		reg = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
			|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
			writel(reg, common->base + common->reg_sel);
		else
			writel(reg, common->base + common->reg_ctrl);

		return 0;
	}

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	reg &= ~GENMASK(mux->width + mux->shift - 1, mux->shift);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		writel(reg | (index << mux->shift), common->base + common->reg_sel);
	else
		writel(reg | (index << mux->shift), common->base + common->reg_ctrl);

	if (common->reg_type == CLK_DIV_TYPE_1REG_FC_V2
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_MUX_V6) {

		ret = ccu_mix_trigger_fc(clk);
		if(ret)
			pr_info("%s of %s timeout\n", __func__, clk->dev->name);
	}

	return 0;
}
#endif

static int ccu_mix_enable(struct clk *clk)
{
#ifdef CONFIG_SPL_BUILD
	clk->id = transfer_clk_id_to_spl(clk->id);
#endif
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	u32 tmp;
	u32 val = 0;
	int timeout_power = 1;

	if (!gate)
		return 0;

#ifdef CONFIG_SPL_BUILD
	if (clk->id == CLK_TWSI8_SPL){
#else
	if (clk->id == CLK_TWSI8){
#endif
		twsi8_reg_val &= ~0x7;
		twsi8_reg_val |= 0x3;
		tmp = twsi8_reg_val;
		if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
			writel(tmp, common->base + common->reg_sel);
		else
			writel(tmp, common->base + common->reg_ctrl);

		return 0;
	}

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		tmp = readl(common->base + common->reg_sel);
	else
		tmp = readl(common->base + common->reg_ctrl);

	tmp &= ~gate->gate_mask;
	tmp |= gate->val_enable;

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		writel(tmp, common->base + common->reg_sel);
	else
		writel(tmp, common->base + common->reg_ctrl);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		val = readl(common->base + common->reg_sel);
	else
		val = readl(common->base + common->reg_ctrl);

	while ((val & gate->gate_mask) != gate->val_enable && (timeout_power < TIMEOUT_LIMIT)) {
		udelay(timeout_power);
		if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
			val = readl(common->base + common->reg_sel);
		else
			val = readl(common->base + common->reg_ctrl);
		timeout_power *= 10;
	}

	if (timeout_power > 1) {
		if (val == tmp)
			pr_info("write clk_gate %s timeout occur, read pass after %d us delay\n",
			clk_hw_get_name(&common->clk), timeout_power);
		else
			pr_info("write clk_gate  %s timeout after %d us!\n", clk_hw_get_name(&common->clk), timeout_power);
	}

	if (gate->flags & SPACEMIT_CLK_GATE_NEED_DELAY) {
		udelay(200);
	}

	return 0;
}

static ulong ccu_mix_get_rate(struct clk *clk)
{
#ifdef CONFIG_SPL_BUILD
	clk->id = transfer_clk_id_to_spl(clk->id);
#endif
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_div_config *div = mix->div;
	unsigned long parent_rate = clk_get_parent_rate(clk);
	unsigned long val;
	u32 reg;

#ifdef CONFIG_SPL_BUILD
	if (clk->id == CLK_TWSI8_SPL){
#else
	if (clk->id == CLK_TWSI8){
#endif
		val = parent_rate;
		return val;
	}

	if (!div){
		if (mix->factor)
			val =  parent_rate * mix->factor->mul / mix->factor->div;
		else
		    val =  parent_rate;
		return val;
	}
	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	val = reg >> div->shift;
	val &= (1 << div->width) - 1;

	val = divider_recalc_rate(clk, parent_rate, val, div->table,
				  div->flags, div->width);

	return val;
}

unsigned long ccu_mix_calc_best_rate(struct clk *clk, unsigned long rate,
u32 *mux_val, u32 *div_val, u32 *parent_id)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_div_config *div = mix->div? mix->div: NULL;
	struct ccu_mux_config *mux = mix->mux? mix->mux: NULL;
	struct clk *parent;
	unsigned long parent_rate = 0, best_rate = 0;
	u32 i, j, p_id, div_max;

	if(mux){
		for (i = 0; i < common->num_parents; i++) {
			for(p_id = 0; p_id < common->clk_tbl->num; p_id++){
				if(!common->clk_tbl->clks[p_id])
					continue;
				if(!strcmp(common->clk_tbl->clks[p_id]->dev->name, common->parent_names[i])){
					break;
				}
			}
			clk_get_by_id(p_id, &parent);
			if (!parent)
				continue;
			parent_rate = clk_get_rate(parent);
			if(div)
				div_max = 1 << div->width;
			else
				div_max = 1;
			for(j = 1; j <= div_max; j++){
				if(abs(parent_rate/j - rate) < abs(best_rate - rate)){
					best_rate = DIV_ROUND_UP_ULL(parent_rate, j);
					*mux_val = i;
					*div_val = j - 1;
					*parent_id = p_id;
				}
			}
		}
	}
	else{
		parent_rate = clk_get_parent_rate(clk);
		if(div)
			div_max = 1 << div->width;
		else
			div_max = 1;
		for(j = 1; j <= div_max; j++){
			if(abs(parent_rate/j - rate) < abs(best_rate - rate)){
				best_rate = DIV_ROUND_UP_ULL(parent_rate, j);
				*div_val = j - 1;
			}
		}
	}
    return best_rate;
}

static ulong ccu_mix_set_rate(struct clk *clk, unsigned long rate)
{
#ifdef CONFIG_SPL_BUILD
	clk->id = transfer_clk_id_to_spl(clk->id);
#endif
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_div_config *div_config = mix->div? mix->div: NULL;
	struct ccu_mux_config *mux_config = mix->mux? mix->mux: NULL;
	unsigned long best_rate = 0;
	u32 cur_mux, cur_div, mux_val = 0, div_val = 0, parent_id = 0;
	struct clk *parent;
	unsigned long val;
	u32 reg;
	int ret;

#ifdef CONFIG_SPL_BUILD
	if (clk->id == CLK_TWSI8_SPL)
#else
	if (clk->id == CLK_TWSI8)
#endif
		return 0;

	if(!div_config && !mux_config){
		return 0;
	}

	best_rate = ccu_mix_calc_best_rate(clk, rate, &mux_val, &div_val, &parent_id);
	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	if(mux_config){
		cur_mux = reg >> mux_config->shift;
		cur_mux &= (1 << mux_config->width) - 1;
		if(cur_mux != mux_val){
			clk_get_by_id(parent_id, &parent);
			clk_set_parent(clk, parent);
		}
	}
	if(div_config){
		cur_div = reg >> div_config->shift;
		cur_div &= (1 << div_config->width) - 1;
		if(cur_div == div_val)
			return 0;
	}else{
		return 0;
	}
	val = div_val;
	if (val > BIT(div_config->width) - 1)
		return 0;

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	reg &= ~GENMASK(div_config->width + div_config->shift - 1, div_config->shift);

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		writel(reg | (val << div_config->shift),
	       common->base + common->reg_sel);
	else
		writel(reg | (val << div_config->shift),
	       common->base + common->reg_ctrl);

	if (common->reg_type == CLK_DIV_TYPE_1REG_FC_V2
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4
		|| common->reg_type == CLK_DIV_TYPE_1REG_FC_DIV_V5) {

		ret = ccu_mix_trigger_fc(clk);
		if(ret)
			pr_info("%s of %s timeout\n", __func__, clk->dev->name);
	}
	return 0;

}

unsigned int ccu_mix_get_parent(struct clk *clk)
{
	struct ccu_mix *mix = clk_to_ccu_mix(clk);
	struct ccu_common * common = &mix->common;
	struct ccu_mux_config *mux = mix->mux;
	u32 reg;
	unsigned int parent;

	if(!mux)
		return 0;

#ifdef CONFIG_SPL_BUILD
	if (clk->id == CLK_TWSI8_SPL){
#else
	if (clk->id == CLK_TWSI8){
#endif
		parent = (twsi8_reg_val >> 4) & 0x7;
		return parent;
	}

	if (common->reg_type == CLK_DIV_TYPE_2REG_NOFC_V3
		|| common->reg_type == CLK_DIV_TYPE_2REG_FC_V4)
		reg = readl(common->base + common->reg_sel);
	else
		reg = readl(common->base + common->reg_ctrl);

	parent = reg >> mux->shift;
	parent &= (1 << mux->width) - 1;

	if (mux->table) {
		int num_parents = common->num_parents;
		int i;

		for (i = 0; i < num_parents; i++)
			if (mux->table[i] == parent)
				return i;
	}
	return parent;
}

const struct clk_ops ccu_mix_ops = {
#ifndef CONFIG_SPL_BUILD
	.disable 	= ccu_mix_disable,
	.round_rate 	= ccu_mix_round_rate,
	.set_parent 	= ccu_mix_set_parent,
#endif
	.enable 	= ccu_mix_enable,
	.get_rate	= ccu_mix_get_rate,
	.set_rate 	= ccu_mix_set_rate,
};

U_BOOT_DRIVER(ccu_clk_mix) = {
	.name	= CCU_CLK_MIX,
	.id 	= UCLASS_CLK,
	.ops	= &ccu_mix_ops,
};

