/*
* This is porting layer for Renesas USBHS driver
* Copyright (C) 2016 GlobalLogic

* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/



#include "common.h"
#include <usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "ehci-rcar.h"

struct platform_usbhs {
	void *usbhs;
	irq_handler_t irq;
	struct platform_device pdev;
	void *priv;
};

static struct platform_usbhs phy_dev;
static struct platform_usbhs usbhs_dev;

/*External functions*/

int usb_gadget_register_driver(struct usb_gadget_driver *driver);
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver);
int usbhsg_register_gadget(struct usbhs_priv *priv, struct usb_gadget_driver *driver);
int usbhsg_unregister_gadget(struct usbhs_priv *priv);
struct usb_gadget *usbhsg_get_gadget(struct usbhs_priv *priv);
int rcar_gen3_phy_usb2_probe(struct platform_device *pdev);


struct phy *devm_phy_create(struct device *dev, void *node,
			    const struct phy_ops *ops)
{
	struct phy *ph;

	ph = kzalloc(sizeof(struct phy), GFP_KERNEL);
	if (!ph)
		return NULL;
	ph->ops = ops;
	phy_dev.usbhs = ph;
	return ph;
}


int
devm_request_irq(struct device *dev, unsigned int irq, irq_handler_t handler,
		 unsigned long irqflags, const char *devname, void *dev_id)
{
	struct platform_usbhs *pdev = NULL;
	if (!handler)
		return -EINVAL;

	if (0 == strcmp(devname, RCAR3_PHY_DEVICE)) {
		pdev = &phy_dev;
	}  else if (0 == strcmp(devname, RCAR3_USBHS_DEVICE)) {
		pdev = &usbhs_dev;
	}

	if (!pdev)
		return EINVAL;

	dev_info((struct device *)NULL, "Setting interrupt handler for device %s\n", devname);
	pdev->irq = handler;
	pdev->priv = dev_id;
	return 0;
}

#define USB_DEVICE_PORT 0
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	int ret = -EINVAL;
	struct phy *ph;
	struct usbhs_priv *priv;
	struct usb_gadget *gadget;

	if (!driver    || !driver->bind || !driver->setup) {
		puts("usbhs: bad parameter.\n");
		return -EINVAL;
	}

	ret = rcar_gen3_phy_usb2_probe(&phy_dev.pdev);
	if (ret <0)
		return ret;

	ph = (struct phy *)phy_dev.usbhs;
	if ((ph) && (ph->ops)) {
		ret = ph->ops->init(ph);
		phy_dev.irq(0, phy_dev.priv);
		ret = ph->ops->power_on(ph);
	}else
		ret = -EINVAL;

	if (ret < 0) {
		printf("Phy init failed\n");
		return ret;
	}

	/*Register usbhs driver*/

	ret = usbhs_probe(&usbhs_dev.pdev);
	if (ret<0) {
		printf("Probe failed with error: %d\n", ret);
		return ret;
	}

	priv = platform_get_drvdata(&usbhs_dev.pdev);
	ret = usbhsg_register_gadget(priv, driver);
	if (ret<0) {
		printf("Register gadget failed with error: %d\n", ret);
		return ret;
	}
	gadget = usbhsg_get_gadget(priv);

	ret = driver->bind(gadget);
	if (ret)
		printf("usbhs: driver->bind() returned %d\n", ret);

	return ret;
}

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct usbhs_priv *priv;
	struct usb_gadget *gadget;
	struct phy *ph = (struct phy *)phy_dev.usbhs;

	priv = platform_get_drvdata(&usbhs_dev.pdev);
	gadget = usbhsg_get_gadget(priv);
	usbhsg_unregister_gadget(priv);
	driver->unbind(gadget);
	usbhs_remove (&usbhs_dev.pdev);

	if ((ph) && (ph->ops)) {
		ph->ops->exit(ph);
	}
	return 0;
}

int usb_gadget_handle_interrupts(int index)
{
	phy_dev.irq(0, phy_dev.priv);
	usbhs_dev.irq(0, usbhs_dev.priv);
	return 0;
}

