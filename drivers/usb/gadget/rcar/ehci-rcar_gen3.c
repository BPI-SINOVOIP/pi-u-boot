/*
 * drivers/usb/host/ehci-rcar_gen3.
 * 	This file is EHCI HCD (Host Controller Driver) for USB.
 *
 * Copyright (C) 2015-2017 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <usb.h>
#include "ehci-rcar.h"

#define BASE_HSUSB	0xE6590000
#define REG_LPSTS	(BASE_HSUSB + 0x0102)	/* 16bit */
#define SUSPM		0x4000
#define SUSPM_SUSPEND	0x0000
#define SUSPM_NORMAL	0x4000
#define REG_UGCTRL2	(BASE_HSUSB + 0x0184)	/* 32bit */
#define USB0SEL		0x00000030
#define USB0SEL_EHCI	0x00000010
#define USB0SEL_HSUSB	0x00000020
#define USB0SEL_OTG	0x00000030

static u32 usb_base_address[] = {
	0xEE080000,	/* USB0 (EHCI) */
	0xEE0A0000,	/* USB1 (EHCI) */
	0xEE0C0000,	/* USB2 (EHCI) */
	0xEE0E0000,	/* USB3 (EHCI) */
};
int usbhs_lowlevel_init(int index, enum usb_init_type init)
{
	u32 base;

	struct ahb_bridge *ahb;
	struct usb_core_reg *ucore;

	base = usb_base_address[index];
	switch (index) {
	case 0:
		clrbits_le32(SMSTPCR7, SMSTPCR703);
		break;
	case 1:
		clrbits_le32(SMSTPCR7, SMSTPCR702);
		break;
	case 2:
		clrbits_le32(SMSTPCR7, SMSTPCR701);
		break;
	case 3:
		clrbits_le32(SMSTPCR7, SMSTPCR700);
		break;
	default:
		return -EINVAL;
	}
	clrbits_le32(SMSTPCR7, SMSTPCR704);

	ahb = (struct ahb_bridge *)(uintptr_t)(base + AHB_OFFSET);
	ucore = (struct usb_core_reg *)(uintptr_t)(base + USB_CORE_OFFSET);

	/* Enable interrupt */
	setbits_le32(&ahb->int_enable, USBH_INTBEN | USBH_INTAEN);
	writel(0x014e029b, &ucore->spd_rsm_timset);
	writel(0x000209ab, &ucore->oc_timset);

	/* Choice USB0SEL */
	clrsetbits_le32(REG_UGCTRL2, USB0SEL, USB0SEL_EHCI);

	/* Clock & Reset */
	clrbits_le32(&ahb->usbctr, PLL_RST);

	/* low power status */
	clrsetbits_le16(REG_LPSTS, SUSPM, SUSPM_NORMAL);

	return 0;
}

