/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <serial.h>
#include <linux/types.h>
#include <asm/io.h>
#include <ring_buf.h>
#include <Galois_memmap.h>

DECLARE_GLOBAL_DATA_PTR;

struct usbconsole_priv {
	unsigned int rx_base;
	unsigned int rx_size;
	unsigned int tx_base;
	unsigned int tx_size;
	rb_handle get_buf;
	rb_handle put_buf;
};

#ifdef CONFIG_DM_SERIAL
static int usbconsole_putc(struct udevice *dev, const char ch)
{
	struct usbconsole_priv * usbcpriv = dev_get_priv(dev);

	int ret = ring_buffer_push(usbcpriv->put_buf, (uint8_t *)&ch, 1);
	if (ret != 1)
		return -1;

	return 0;
}

static int usbconsole_pending(struct udevice *dev, bool input)
{
	struct usbconsole_priv * usbcpriv = dev_get_priv(dev);

	if (input) {
		return ring_buffer_not_empty(usbcpriv->get_buf);
	}
	return 1;
}

static int usbconsole_getc(struct udevice *dev)
{
	struct usbconsole_priv * usbcpriv = dev_get_priv(dev);
	unsigned char ch;

	int ret = ring_buffer_pop(usbcpriv->get_buf, &ch, 1);
	if (ret == 0)
		return -EAGAIN;

	return (int)ch;
}

static int usbconsole_setbrg(struct udevice *dev, int baudrate)
{
	return 0;
}

#define SYSINIT_SIZE (0xa000)
#define RINGBUF_SIZE (0x800)
int usbconsole_probe(struct udevice *dev)
{
	struct usbconsole_priv * usbcpriv = dev_get_priv(dev);

	usbcpriv->get_buf = ring_buffer_attach(usbcpriv->rx_base, usbcpriv->rx_size);
	usbcpriv->put_buf = ring_buffer_attach(usbcpriv->tx_base, usbcpriv->tx_size);

	return 0;
}

#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
int usbconsole_ofdata_to_platdata(struct udevice *dev)
{
	struct usbconsole_priv * usbcpriv = dev_get_priv(dev);
	unsigned int reg[2];

	dev_read_u32_array(dev, "rxbuf", reg, 2);
	usbcpriv->rx_base = reg[0];
	usbcpriv->rx_size = reg[1];

	dev_read_u32_array(dev, "txbuf", reg, 2);
	usbcpriv->tx_base = reg[0];
	usbcpriv->tx_size = reg[1];

	return 0;
}
#endif

const struct dm_serial_ops usbconsole_ops = {
	.putc = usbconsole_putc,
	.pending = usbconsole_pending,
	.getc = usbconsole_getc,
	.setbrg = usbconsole_setbrg,
};

#if !CONFIG_IS_ENABLED(OF_PLATDATA)
#if CONFIG_IS_ENABLED(OF_CONTROL)
static const struct udevice_id usbconsole_ids[] = {
	{ .compatible = "usbconsole" },
	{}
};
#endif

#if CONFIG_IS_ENABLED(SERIAL_PRESENT)
U_BOOT_DRIVER(usbconsole) = {
	.name	= "usbconsole",
	.id	= UCLASS_SERIAL,
#if CONFIG_IS_ENABLED(OF_CONTROL)
	.ofdata_to_platdata = usbconsole_ofdata_to_platdata,
	.of_match = usbconsole_ids,
#endif
	.priv_auto_alloc_size = sizeof(struct usbconsole_priv),
	.probe = usbconsole_probe,
	.ops	= &usbconsole_ops,
	.flags	= DM_FLAG_PRE_RELOC,
};
#endif
#endif /* !OF_PLATDATA */
#endif /* CONFIG_DM_SERIAL */
