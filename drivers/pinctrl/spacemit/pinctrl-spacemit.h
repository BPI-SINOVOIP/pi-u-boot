/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2023, spacemit Corporation.
 */
#ifndef __DRIVERS_PINCTRL_SPACEMIT_H
#define __DRIVERS_PINCTRL_SPACEMIT_H

#define PINID_TO_BANK(p)	((p) >> 5)
#define PINID_TO_PIN(p)		((p) % 32)

/*
 * pin config bit field definitions
 *
 * od:	open drain
 * pe:  pull enable
 * st:	schmit trigger
 * rte:	retention signal bus
 *
 * MSB of each field is presence bit for the config.
 */
#define OD_EN		1
#define OD_DIS		0
#define PE_EN		1
#define PE_DIS		0
#define ST_EN		1
#define ST_DIS		0
#define RTE_EN		1
#define RTE_DIS		0

/*
 * Each pin represented in spacemit,pins consists:
 * - u32 PIN_FUNC_ID
 * - u32 pin muxsel
 * - u32 pin pull_up/down
 * - u32 pin driving strength
 */
#define SPACEMIT_PIN_SIZE 16

struct spacemit_regs {
	u16 cfg;
	u16 reg_len;
};

struct spacemit_pin_conf {
	u8  fs_shift;
	u8  od_shift;
	u8  pe_shift;
	u8  pull_shift;
	u8  ds_shift;
	u8  st_shift;
	u8  rte_shift;
};

/**
 * @base: the address to the controller in virtual memory
 * @rstc: reset signal handle.
 */
struct spacemit_pinctrl_soc_info {
	const struct spacemit_regs *regs;
	const struct spacemit_pin_conf *pinconf;
	void __iomem *base;
	struct reset_ctl *rstc;
};

/**
 * @dev: a pointer back to containing device
 * @info: the soc info
 */
struct spacemit_pinctrl_priv {
	struct udevice *dev;
	struct spacemit_pinctrl_soc_info *info;
};

extern const struct pinctrl_ops spacemit_pinctrl_ops;

int spacemit_pinctrl_probe(struct udevice *dev,
		struct spacemit_pinctrl_soc_info *info);
int spacemit_pinctrl_remove(struct udevice *dev);

#endif /* __DRIVERS_PINCTRL_SPACEMIT_H */
