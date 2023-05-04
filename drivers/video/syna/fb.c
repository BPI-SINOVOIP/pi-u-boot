/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2017 Marvell Technology Group Ltd.
 * Xiaoming Lu <xmlu@marvell.com>
 *
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <linux/libfdt.h>
#include <video.h>
#include "include/vpp.h"
#include "asm-generic/gpio.h"
#include <iomux.h>

DECLARE_GLOBAL_DATA_PTR;

struct berlin_fb_priv {
	ushort xres;
	ushort yres;
	u_char bpix;
	int init;
};

static int berlin_fb_init(void)
{
#if defined(CONFIG_BERLIN_SOC_BG5CT) \
		&& defined(CONFIG_FASTBOOT)
	struct gpio_desc gpio = {};
	int node;

	node = fdt_node_offset_by_compatible(gd->fdt_blob, 0, "fb,hdmi-5v-gpio");
	if (node < 0){
		printf("can't find fb,hdmi-5v-gpio node\n");
		return -1;
	}

	gpio_request_by_name_nodev(gd->fdt_blob, node, "hdmi-gpio", 0, &gpio,  GPIOD_IS_OUT);
	if (dm_gpio_is_valid(&gpio)) {
		dm_gpio_set_value(&gpio, 1);
	}
#endif
	gd->flags &= ~GD_FLG_DEVINIT;
	debug("%s\n", __func__);
	int ret = MV_VPP_Init();
	if (ret != 0)
	{
		printf("MV_VPP_Init failed\n");
		return -1;
	}

	MV_VPP_Enable_IRQ();
	gd->flags |= GD_FLG_DEVINIT;

	return 0;
}

static int berlin_fb_flush(struct udevice *dev)
{
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct berlin_fb_priv *priv = dev_get_priv(dev);

	if (!priv->init) {
		if (berlin_fb_init())
			return -1;
		priv->init = 1;
	}

	MV_VPP_DisplayFrame(&uc_priv->fb, priv->xres, priv->yres);
	return 0;
}

static int berlin_fb_ofdata_to_platdata(struct udevice *dev)
{
	debug("%s\n", __func__);
	struct berlin_fb_priv *priv = dev_get_priv(dev);
	unsigned int node = dev_of_offset(dev);
	const void *blob = gd->fdt_blob;

	priv->xres = fdtdec_get_int(blob, node, "xres", 1280);
	if (priv->xres == 0) {
		debug("Can't get XRES\n");
		return -ENXIO;
	}

	priv->yres = fdtdec_get_int(blob, node, "yres", 720);
	if (priv->yres == 0) {
		debug("Can't get YRES\n");
		return -ENXIO;
	}

	priv->bpix = fdtdec_get_int(blob, node, "bpix", VIDEO_BPP32);
	if (priv->bpix == 0) {
		debug("Can't get bits per pixel\n");
		return -ENXIO;
	}

	return 0;
}

static int berlin_fb_probe(struct udevice *dev)
{
	debug("%s\n", __func__);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct berlin_fb_priv *priv = dev_get_priv(dev);

	priv->init = 0;

	uc_priv->xsize = priv->xres;
	uc_priv->ysize = priv->yres;
	uc_priv->bpix = priv->bpix;
	uc_priv->colour_format = VIDEO_COLOUR_FORMAT_ABGR;

	/* Enable flushing after LCD writes if requested */
	video_set_flush_dcache(dev, true);

	return 0;
}

static int berlin_fb_bind(struct udevice *dev)
{
	debug("%s\n", __func__);
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);

	/* This is the maximum panel size we expect to see */
	plat->size = 1280 * 720 * 4;

	return 0;
}

static int berlin_fb_remove(struct udevice *dev)
{
	debug("%s\n", __func__);
	MV_VPP_Deinit();
	return 0;
}

static const struct video_ops berlin_fb_ops = {
	.flush = berlin_fb_flush,
};

static const struct udevice_id berlin_fb_ids[] = {
	{ .compatible = "marvell,berlin-fb" },
	{ }
};

U_BOOT_DRIVER(berlin_fb) = {
	.name	= "berlin_fb",
	.id	= UCLASS_VIDEO,
	.of_match = berlin_fb_ids,
	.ops	= &berlin_fb_ops,
	.bind	= berlin_fb_bind,
	.probe	= berlin_fb_probe,
	.remove	= berlin_fb_remove,
	.ofdata_to_platdata	= berlin_fb_ofdata_to_platdata,
	.priv_auto_alloc_size	= sizeof(struct berlin_fb_priv),
};

static int do_vidconsole(cmd_tbl_t *cmdtp, int flag, int argc,
			 char *const argv[])
{
	if (argc == 1)
		iomux_doenv(stdout, "uart@d000,vidconsole");
	else
		iomux_doenv(stdout, argv[1]);
	return 0;
}

U_BOOT_CMD(
	vidconsole, 2,	1,	do_vidconsole,
	"print string on video framebuffer",
	"    <string>"
);
