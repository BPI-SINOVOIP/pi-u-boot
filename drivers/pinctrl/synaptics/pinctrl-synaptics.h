// SPDX-License-Identifier: GPL-2.0+
/*
 * Synaptics pinctrl driver
 *
 * Copyright (C) 2019 Synaptics Corporation
 */

#ifndef __PINCTRL_SYNAPTICS_H
#define __PINCTRL_SYNAPTICS_H

struct synaptics_desc_function {
	const char	*name;
	u8		muxval;
};

struct synaptics_desc_group {
	const char			*name;
	u8				offset;
	u8				bit_width;
	u8				lsb;
	struct synaptics_desc_function	*functions;
};

struct synaptics_pinctrl_desc {
	const struct synaptics_desc_group	*groups;
	unsigned			ngroups;
};

struct synaptics_pinctrl_function {
	const char	*name;
	const char	**groups;
	unsigned	ngroups;
};

#define SYNAPTICS_PINCTRL_GROUP(_name, _offset, _width, _lsb, ...)		\
	{								\
		.name = _name,						\
		.offset = _offset,					\
		.bit_width = _width,					\
		.lsb = _lsb,						\
		.functions = (struct synaptics_desc_function[]){		\
			__VA_ARGS__, { } },				\
	}

#define SYNAPTICS_PINCTRL_FUNCTION(_muxval, _name)		\
	{						\
		.name = _name,				\
		.muxval = _muxval,			\
	}

#define SYNAPTICS_PINCTRL_FUNCTION_UNKNOWN		{}

struct synaptics_pinctrl {
	fdt_addr_t base;
	struct udevice *dev;
	const struct synaptics_pinctrl_desc *desc;
	struct synaptics_pinctrl_function *functions;
	unsigned nfunctions;
};

extern const struct pinctrl_ops synaptics_pinctrl_ops;

extern int synaptics_pinctrl_probe(struct udevice *dev,
				const struct synaptics_pinctrl_desc *desc);

#endif /* __PINCTRL_SYNAPTICS_H */
