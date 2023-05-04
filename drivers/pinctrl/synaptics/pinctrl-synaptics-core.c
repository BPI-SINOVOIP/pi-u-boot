// SPDX-License-Identifier: GPL-2.0+
/*
 * Synaptics pinctrl driver
 *
 * Copyright (C) 2019 Synaptics Corporation
 */

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <dm/pinctrl.h>
#include <dm/fdtaddr.h>
#include "pinctrl-synaptics.h"

static int synaptics_pinctrl_get_groups_count(struct udevice *dev)
{
	struct synaptics_pinctrl *pctrl = dev_get_priv(dev);

	return pctrl->desc->ngroups;
}

static const char *synaptics_pinctrl_get_group_name(struct udevice *dev,
						 unsigned selector)
{
	struct synaptics_pinctrl *pctrl = dev_get_priv(dev);

	return pctrl->desc->groups[selector].name;
}

static int synaptics_pinmux_get_functions_count(struct udevice *dev)
{
	struct synaptics_pinctrl *pctrl = dev_get_priv(dev);

	return pctrl->nfunctions;
}

static const char *synaptics_pinmux_get_function_name(struct udevice *dev,
						   unsigned selector)
{
	struct synaptics_pinctrl *pctrl = dev_get_priv(dev);

	return pctrl->functions[selector].name;
}

static struct synaptics_desc_function *
synaptics_pinctrl_find_function_by_name(struct synaptics_pinctrl *pctrl,
				     const struct synaptics_desc_group *group,
				     const char *fname)
{
	struct synaptics_desc_function *function = group->functions;

	while (function->name) {
		if (!strcmp(function->name, fname))
			return function;

		function++;
	}

	return NULL;
}

static int synaptics_pinmux_group_set(struct udevice *dev,
				   unsigned group_selector,
				   unsigned func_selector)
{
	struct synaptics_pinctrl *pctrl = dev_get_priv(dev);
	const struct synaptics_desc_group *group_desc = pctrl->desc->groups + group_selector;
	struct synaptics_pinctrl_function *func = pctrl->functions + func_selector;
	struct synaptics_desc_function *function_desc =
		synaptics_pinctrl_find_function_by_name(pctrl, group_desc,
						     func->name);

	u32 mask, val;

	if (!function_desc)
		return -EINVAL;

	debug("%s: group=%d  func=%d\n", __func__, group_selector, func_selector);
	mask = GENMASK(group_desc->lsb + group_desc->bit_width - 1,
		       group_desc->lsb);
	val = function_desc->muxval << group_desc->lsb;

	fdt_addr_t addr = pctrl->base + group_desc->offset;
	debug("%s: base=%016llx  offset=%08x\n", __func__, pctrl->base, group_desc->offset);
	u32 data = readl(addr);
	debug("%s: readl  %016llx  %08x\n", __func__, addr, data);
	debug("%s: mask=%08x, val=%08x\n", __func__, mask, val);
	data &= ~mask;
	data |= val & mask;
	writel(data, addr);
	debug("%s: writel %016llx  %08x\n", __func__, addr, data);

	return 0;
}

const struct pinctrl_ops synaptics_pinctrl_ops = {
	.get_groups_count = synaptics_pinctrl_get_groups_count,
	.get_group_name = synaptics_pinctrl_get_group_name,
	.get_functions_count = synaptics_pinmux_get_functions_count,
	.get_function_name = synaptics_pinmux_get_function_name,
	.pinmux_group_set = synaptics_pinmux_group_set,
	.set_state = pinctrl_generic_set_state,
};

static int synaptics_pinctrl_add_function(struct synaptics_pinctrl *pctrl,
				       const char *name)
{
	struct synaptics_pinctrl_function *function = pctrl->functions;

	while (function->name) {
		if (!strcmp(function->name, name)) {
			function->ngroups++;
			return -EEXIST;
		}
		function++;
	}

	function->name = name;
	function->ngroups = 1;

	pctrl->nfunctions++;

	return 0;
}

static int synaptics_pinctrl_build_state(struct udevice *dev)
{
	struct synaptics_pinctrl *pctrl = dev_get_priv(dev);
	struct synaptics_desc_group const *desc_group;
	struct synaptics_desc_function const *desc_function;
	int i, max_functions = 0;

	pctrl->nfunctions = 0;

	for (i = 0; i < pctrl->desc->ngroups; i++) {
		desc_group = pctrl->desc->groups + i;
		/* compute the maxiumum number of functions a group can have */
		max_functions += 1 << (desc_group->bit_width + 1);
	}

	/* we will reallocate later */
	pctrl->functions = malloc(max_functions * sizeof(*pctrl->functions));
	if (!pctrl->functions)
		return -ENOMEM;
	memset(pctrl->functions, 0, max_functions * sizeof(*pctrl->functions));

	/* register all functions */
	for (i = 0; i < pctrl->desc->ngroups; i++) {
		desc_group = pctrl->desc->groups + i;
		desc_function = desc_group->functions;

		while (desc_function->name) {
			synaptics_pinctrl_add_function(pctrl, desc_function->name);
			desc_function++;
		}
	}

	/* map functions to theirs groups */
	for (i = 0; i < pctrl->desc->ngroups; i++) {
		desc_group = pctrl->desc->groups + i;
		desc_function = desc_group->functions;

		while (desc_function->name) {
			struct synaptics_pinctrl_function
				*function = pctrl->functions;
			const char **groups;
			bool found = false;

			while (function->name) {
				if (!strcmp(desc_function->name, function->name)) {
					found = true;
					break;
				}
				function++;
			}

			if (!found) {
				return -EINVAL;
			}

			if (!function->groups) {
				function->groups =
					malloc(function->ngroups * sizeof(char *));
				if (!function->groups)
					return -ENOMEM;
				memset(function->groups, 0, function->ngroups * sizeof(char *));
			}

			groups = function->groups;
			while (*groups)
				groups++;
			*groups = desc_group->name;

			desc_function++;
		}
	}

	return 0;
}

int synaptics_pinctrl_probe(struct udevice *dev,
			 const struct synaptics_pinctrl_desc *desc)
{
	struct synaptics_pinctrl *pctrl = dev_get_priv(dev);
	int ret;

	pctrl->base = devfdt_get_addr(dev);
	if (pctrl->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	pctrl->dev = dev;
	pctrl->desc = desc;
	debug("%s:%s base=%016llx, desc=%p\n", __func__, dev->name, pctrl->base, pctrl->desc);
	ret = synaptics_pinctrl_build_state(dev);
	if (ret) {
		dev_err(dev, "cannot build driver state: %d\n", ret);
		return ret;
	}

	return 0;
}
