// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Cheng Lu <Cheng.Lu@synaptics.com>
 */

#ifndef __USB_PHY_SYNA_H
#define __USB_PHY_SYNA_H

struct phy_syna_usb_priv {
	struct udevice *dev;
	const struct phy_ops *ops;
	fdt_addr_t base;
	struct reset_ctl_bulk	rsts;
};

extern const struct phy_ops syna_usb_phy_ops;
extern int syna_usb_phy_probe(struct udevice *dev, struct phy_ops *desc);
#endif /* __USB_PHY_SYNA_H */
