// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023, spacemit Corporation.
 */
#include <common.h>
#include <malloc.h>
#include <mapmem.h>
#include <reset.h>
#include <asm/global_data.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/err.h>
#include <dm.h>
#include <dm/pinctrl.h>

#include "pinctrl-spacemit.h"

DECLARE_GLOBAL_DATA_PTR;

static int spacemit_pinctrl_set_state(struct udevice *dev, struct udevice *config)
{
	struct spacemit_pinctrl_priv *priv = dev_get_priv(dev);
	struct spacemit_pinctrl_soc_info *info = priv->info;
	const struct spacemit_regs *regs = info->regs;
	const struct spacemit_pin_conf *pin_conf = info->pinconf;
	int node = dev_of_offset(config);
	const struct fdt_property *prop;
	u32 *pin_data;
	int npins, size, pin_size;
	u32 pin_id, mux_sel, pull_val, drv_strength;
	u8 bank;
	u16 offset;
	u32 fs, od, pull_en, pull, ds, st, rte;
	u64 reg;
	int i, j = 0;

	dev_dbg(dev, "%s: %s\n", __func__, config->name);
	pin_size = SPACEMIT_PIN_SIZE;

	prop = fdt_getprop(gd->fdt_blob, node, "spacemit,pins", &size);
	if (!prop) {
		dev_err(dev, "No spacemit,pins property in node %s\n", config->name);
		return -EINVAL;
	}

	if (!size || size % pin_size) {
		dev_err(dev, "Invalid spacemit,pins property in node %s\n",
			config->name);
		return -EINVAL;
	}

	pin_data = devm_kzalloc(dev, size, 0);
	if (!pin_data)
		return -ENOMEM;

	if (fdtdec_get_int_array(gd->fdt_blob, node, "spacemit,pins",
				 pin_data, size >> 2)) {
		dev_err(dev, "Error reading pin data.\n");
		devm_kfree(dev, pin_data);
		return -EINVAL;
	}

	npins = size / pin_size;

	/*
	 * set pin config to pinctrl reg
	 */
	for (i = 0; i < npins; i++) {
		volatile long mux_config;

		pin_id = pin_data[j++];
		mux_sel = pin_data[j++];
		pull_val = pin_data[j++];
		drv_strength = pin_data[j++];

		dev_dbg(dev, "pin_id 0x%x, mux_sel 0x%x, \
			pull_val 0x%x, drv_strength 0x%x\n",
			pin_id, mux_sel, pull_val, drv_strength);

		bank = PINID_TO_BANK(pin_id);
		offset = PINID_TO_PIN(pin_id);
		reg = (u64)info->base + (u64)regs->cfg;
		reg += bank * regs->reg_len + offset * 4;

		fs = mux_sel << pin_conf->fs_shift;
		od =  OD_DIS << pin_conf->od_shift;
		pull_en = PE_EN << pin_conf->pe_shift;
		pull = pull_val << pin_conf->pull_shift;
		ds = drv_strength << pin_conf->ds_shift;
		st = ST_DIS << pin_conf->st_shift;
		rte = RTE_EN << pin_conf->rte_shift;

		mux_config = (fs | od | pull_en | pull | ds | st | rte);
		writel(mux_config, (void __iomem *)reg);
		dev_dbg(dev, "write: bank %d 0ffset %d val 0x%lx\n",
			bank, offset, mux_config);
	}

	devm_kfree(dev, pin_data);

	return 0;
}

const struct pinctrl_ops spacemit_pinctrl_ops  = {
	.set_state = spacemit_pinctrl_set_state,
};

int spacemit_pinctrl_probe(struct udevice *dev,
		struct spacemit_pinctrl_soc_info *info)
{
	struct spacemit_pinctrl_priv *priv = dev_get_priv(dev);
	fdt_addr_t addr;
	fdt_size_t size;

	if (!info) {
		dev_err(dev, "wrong pinctrl info\n");
		return -EINVAL;
	}

	priv->dev = dev;
	priv->info = info;

	addr = devfdt_get_addr_size_index(dev, 0, &size);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	info->base = map_sysmem(addr, size);
	if (!info->base)
		return -ENOMEM;
	priv->info = info;

	info->rstc = devm_reset_control_get_by_index(dev, 0);
	if (IS_ERR(info->rstc)) {
		dev_warn(dev, "get optional reset failed\n");
		return -EINVAL;
	}
	reset_deassert(info->rstc);

	dev_dbg(dev, "initialized spacemit pinctrl driver\n");

	return 0;
}

int spacemit_pinctrl_remove(struct udevice *dev)
{
	struct spacemit_pinctrl_priv *priv = dev_get_priv(dev);
	struct spacemit_pinctrl_soc_info *info = priv->info;

	reset_assert(info->rstc);
	if (info->base)
		unmap_sysmem(info->base);

	return 0;
}
