// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <common.h>
#include <display.h>
#include <dm.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/lists.h>

#include <regmap.h>
#include <syscon.h>
#include <video.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>

#include <power-domain-uclass.h>
#include <power-domain.h>
#include <clk.h>
#include <video_bridge.h>
#include <power/pmic.h>
#include <panel.h>

#include "spacemit_dpu.h"

DECLARE_GLOBAL_DATA_PTR;

struct fb_info fbi = {0};

struct spacemit_mode_modeinfo hdmi_1080p_modeinfo = {
	.name = "1920x1080-60",
	.refresh = 60,
	.xres = 1920,
	.yres = 1080,
	.real_xres = 1920,
	.real_yres = 1080,
	.left_margin = 148,
	.right_margin = 88,
	.hsync_len = 44,
	.upper_margin = 36,
	.lower_margin = 4,
	.vsync_len = 5,
	.hsync_invert = 0,
	.vsync_invert = 0,
	.invert_pixclock = 0,
	.pixclock_freq = 148500,
	.pix_fmt_out = OUTFMT_RGB888,
	.width = 0,
	.height = 0,
};

#if 0
static unsigned int hdmi_dpu_read(void __iomem *addr)
{
	unsigned int val = readl(addr + 0xc0440000);
	return val;
}
#endif

static void hdmi_dpu_write(void __iomem *addr, unsigned int val)
{
	writel(val, addr + 0xc0440000);
}

static void hdmi_dpu_init(struct spacemit_mode_modeinfo *hdmi_modeinfo, ulong fbbase)
{
	unsigned int vsync, hsync, vbp, vfp, hbp, hfp, vsp, hsp;
	unsigned int reg_val = 0;

	vsync = hdmi_modeinfo->vsync_len & 0x3ff;
	hsync = hdmi_modeinfo->hsync_len & 0x3ff;
	vbp = hdmi_modeinfo->upper_margin & 0xfff;
	vfp = hdmi_modeinfo->lower_margin & 0xfff;
	hbp = hdmi_modeinfo->left_margin & 0xfff;
	hfp = hdmi_modeinfo->right_margin & 0xfff;
	vsp = hdmi_modeinfo->hsync_invert ? 0 : 1;
	hsp = hdmi_modeinfo->hsync_invert ? 0 : 1;

	pr_debug("%s vsync %d hsync %d \n", __func__, vsync, hsync);
	pr_debug("%s vbp %d vfp %d vsp %d\n", __func__, vbp, vfp, vsp);
	pr_debug("%s hbp %d hfp %d hsp %d\n", __func__, hbp, hfp, hsp);

	hdmi_dpu_write((void __iomem *)0xa1c, 0x2223);
	hdmi_dpu_write((void __iomem *)0x18000, (hdmi_modeinfo->yres << 16) | hdmi_modeinfo->xres);
	hdmi_dpu_write((void __iomem *)0x18018, 0x20);
	hdmi_dpu_write((void __iomem *)0x1807c, 0x100);
	reg_val = (hbp << 16) | hfp;
	hdmi_dpu_write((void __iomem *)0x18080, reg_val);
	reg_val = (vbp << 16) | vfp;
	hdmi_dpu_write((void __iomem *)0x18084, reg_val);
	reg_val = (vsp << 28) | (vsync << 16) | (hsp << 12) | (hsync);
	hdmi_dpu_write((void __iomem *)0x18088, reg_val);
	hdmi_dpu_write((void __iomem *)0x1808c, (hdmi_modeinfo->yres << 16) | hdmi_modeinfo->xres);
	hdmi_dpu_write((void __iomem *)0x18090, hdmi_modeinfo->pix_fmt_out);
	hdmi_dpu_write((void __iomem *)0xd80, 0x202040);
	hdmi_dpu_write((void __iomem *)0xda0, (unsigned int)(fbbase & 0xffffffff));
	hdmi_dpu_write((void __iomem *)0xda4, (unsigned int)(fbbase >> 32));
	hdmi_dpu_write((void __iomem *)0xdb8, hdmi_modeinfo->xres * 4);
	hdmi_dpu_write((void __iomem *)0xdbc, (hdmi_modeinfo->yres << 16) | hdmi_modeinfo->xres);
	hdmi_dpu_write((void __iomem *)0xdc0, 0x0);
	hdmi_dpu_write((void __iomem *)0xdc4, ((hdmi_modeinfo->yres - 1) << 16) | (hdmi_modeinfo->xres - 1));
	hdmi_dpu_write((void __iomem *)0xdf0, 0x4);

	hdmi_dpu_write((void __iomem *)0x4c00, (hdmi_modeinfo->xres << 8) | 0x01);
	hdmi_dpu_write((void __iomem *)0x4c04, hdmi_modeinfo->yres);
	hdmi_dpu_write((void __iomem *)0x4c10, 0xff0000);
	hdmi_dpu_write((void __iomem *)0x4c14, 0xff);
	hdmi_dpu_write((void __iomem *)0x4c38, 0x7);
	hdmi_dpu_write((void __iomem *)0x4c48, 0x0);
	hdmi_dpu_write((void __iomem *)0x4c4c, (hdmi_modeinfo->xres - 1) << 16);
	hdmi_dpu_write((void __iomem *)0x4c50, hdmi_modeinfo->yres - 1);
	hdmi_dpu_write((void __iomem *)0x4c54, 0xff0000);

	hdmi_dpu_write((void __iomem *)0x560, 0x40008);
	hdmi_dpu_write((void __iomem *)0x588, 0x821);
	hdmi_dpu_write((void __iomem *)0x56c, 0x1);
	hdmi_dpu_write((void __iomem *)0x58c, 0x1);

}

static int spacemit_panel_init(void)
{
	struct video_tx_device *tx = NULL;
	int modes_num = 0;

	tx = find_video_tx();
	if (!tx) {
		pr_info("probe: failed to find video tx\n");
		return -1;
	}
	fbi.tx = tx;

	modes_num = video_tx_get_modes(fbi.tx, &fbi.mode);
	if (!modes_num) {
		pr_info("can't get videomode num\n");
		return -1;
	}

	video_tx_dpms(fbi.tx, DPMS_ON);

	return 0;
}

#if 0
static unsigned int dsi_dpu_read(void __iomem *addr)
{
	unsigned int val = readl(addr + 0xc0340000);
	pr_debug("## dpu read [0x%lx] = 0x%x\n", (unsigned long)addr + 0xc0340000, val);
	return val;
}
#endif

static void dsi_dpu_write(void __iomem *addr, unsigned int val)
{
	pr_debug("## dpu write [0x%lx] = 0x%x\n", (unsigned long)addr + 0xc0340000, val);
	writel(val, addr + 0xc0340000);
}

static void dsi_dpu_init(struct spacemit_mode_modeinfo *spacemit_mode, ulong fbbase)
{
	unsigned int vsync, hsync, vbp, vfp, hbp, hfp, vsp, hsp;
	unsigned int reg_val = 0;

	vsync = spacemit_mode->vsync_len & 0x3ff;
	hsync = spacemit_mode->hsync_len & 0x3ff;
	vbp = spacemit_mode->upper_margin & 0xfff;
	vfp = spacemit_mode->lower_margin & 0xfff;
	hbp = spacemit_mode->left_margin & 0xfff;
	hfp = spacemit_mode->right_margin & 0xfff;
	vsp = spacemit_mode->hsync_invert ? 0 : 1;
	hsp = spacemit_mode->hsync_invert ? 0 : 1;

	dsi_dpu_write((void __iomem *)0xa1c, 0x2223);
	dsi_dpu_write((void __iomem *)0x18000, (spacemit_mode->yres << 16) | spacemit_mode->xres);
	dsi_dpu_write((void __iomem *)0x18018, 0x20);
	dsi_dpu_write((void __iomem *)0x1807c, 0x100);
	reg_val = (hbp << 16) | hfp;
	dsi_dpu_write((void __iomem *)0x18080, reg_val);
	reg_val = (vbp << 16) | vfp;
	dsi_dpu_write((void __iomem *)0x18084, reg_val);
	reg_val = (vsp << 28) | (vsync << 16) | (hsp << 12) | (hsync);
	dsi_dpu_write((void __iomem *)0x18088, reg_val);
	dsi_dpu_write((void __iomem *)0x1808c, (spacemit_mode->yres << 16) | spacemit_mode->xres);
	dsi_dpu_write((void __iomem *)0x18090, spacemit_mode->pix_fmt_out);
	dsi_dpu_write((void __iomem *)0xd80, 0x202040);
	dsi_dpu_write((void __iomem *)0xda0, (unsigned int)(fbbase & 0xffffffff));
	dsi_dpu_write((void __iomem *)0xda4, (unsigned int)(fbbase >> 32));
	dsi_dpu_write((void __iomem *)0xdb8, spacemit_mode->xres * 4);
	dsi_dpu_write((void __iomem *)0xdbc, (spacemit_mode->yres << 16) | spacemit_mode->xres);
	dsi_dpu_write((void __iomem *)0xdc0, 0x0);
	dsi_dpu_write((void __iomem *)0xdc4, ((spacemit_mode->yres - 1) << 16) | (spacemit_mode->xres - 1));
	dsi_dpu_write((void __iomem *)0xdf0, 0x4);
	/* PP3 will impact display light */
	dsi_dpu_write((void __iomem *)0x4c00, (spacemit_mode->xres << 8) | 0x01);
	dsi_dpu_write((void __iomem *)0x4c04, spacemit_mode->yres);
	dsi_dpu_write((void __iomem *)0x4c10, 0xff0000);
	dsi_dpu_write((void __iomem *)0x4c14, 0xff);
	dsi_dpu_write((void __iomem *)0x4c38, 0x7);
	dsi_dpu_write((void __iomem *)0x4c48, 0x0);
	dsi_dpu_write((void __iomem *)0x4c4c, (spacemit_mode->xres - 1) << 16);
	dsi_dpu_write((void __iomem *)0x4c50, spacemit_mode->yres - 1);
	dsi_dpu_write((void __iomem *)0x4c54, 0xff0000);

	dsi_dpu_write((void __iomem *)0x560, 0x40008);
	dsi_dpu_write((void __iomem *)0x588, 0x821);
	dsi_dpu_write((void __iomem *)0x56c, 0x1);
	dsi_dpu_write((void __iomem *)0x58c, 0x1);
}

static int spacemit_display_init(struct udevice *dev, ulong fbbase, ofnode ep_node)
{
	// struct video_uc_plat *uc_plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	// struct spacemit_dpu_priv *priv = dev_get_priv(dev);
	struct display_timing timing;
	int dpu_id, remote_dpu_id;
	struct udevice *disp;
	int ret;
	u32 remote_phandle;
	ofnode remote;
	const char *compat;
	struct display_plat *disp_uc_plat;
	// struct udevice *panel = NULL;

	struct spacemit_mode_modeinfo *spacemit_mode = NULL;


	pr_debug("%s(%s, 0x%lx, %s)\n", __func__,
			  dev_read_name(dev), fbbase, ofnode_get_name(ep_node));

	ret = ofnode_read_u32(ep_node, "remote-endpoint", &remote_phandle);
	if (ret)
		return ret;

	remote = ofnode_get_by_phandle(remote_phandle);
	if (!ofnode_valid(remote))
		return -EINVAL;
	remote_dpu_id = ofnode_read_u32_default(remote, "reg", -1);
	uc_priv->bpix = VIDEO_BPP32;
	pr_debug("remote_dpu_id  %d\n", remote_dpu_id);

	while (ofnode_valid(remote)) {
		remote = ofnode_get_parent(remote);
		if (!ofnode_valid(remote)) {
			pr_debug("%s(%s): no UCLASS_DISPLAY for remote-endpoint\n",
			      __func__, dev_read_name(dev));
			return -EINVAL;
		}
		pr_debug("%s(dev: %s, 0x%lx,remote node: %s)\n", __func__,
		  dev_read_name(dev), fbbase, ofnode_get_name(remote));

		uclass_find_device_by_ofnode(UCLASS_DISPLAY, remote, &disp);
		if (disp)
			break;

	};
	compat = ofnode_get_property(remote, "compatible", NULL);
	if (!compat) {
		pr_info("%s(%s): Failed to find compatible property\n",
		      __func__, dev_read_name(dev));
		return -EINVAL;
	}

	if (strstr(compat, "mipi")) {
		dpu_id = DPU_MODE_MIPI;
	} else if (strstr(compat, "hdmi")) {
		dpu_id = DPU_MODE_HDMI;
	} else {
		pr_info("%s(%s): Failed to find dpu mode for %s\n",
		      __func__, dev_read_name(dev), compat);
		return -EINVAL;
	}

	pr_debug("dpu_id %d,compat = %s\n", dpu_id, compat);

	if(dpu_id == DPU_MODE_HDMI)
	{
		disp_uc_plat = dev_get_uclass_plat(disp);

		pr_info("Found device '%s', disp_uc_priv=%p\n", disp->name, disp_uc_plat);

		disp_uc_plat->source_id = remote_dpu_id;
		disp_uc_plat->src_dev = dev;

		ret = device_probe(disp);
		if (ret) {
			pr_info("%s: device '%s' display won't probe (ret=%d)\n",
			  __func__, dev->name, ret);
			return ret;
		}

		ret = display_enable(disp, 1 << VIDEO_BPP32, &timing);
		if (ret) {
			pr_info("%s: Failed to read timings\n", __func__);
			return ret;
		}

		hdmi_dpu_init(&hdmi_1080p_modeinfo, fbbase);

		uc_priv->xsize = 1920;
		uc_priv->ysize = 1080;

		pr_info("fb=%lx, size=%d %d\n", fbbase, uc_priv->xsize, uc_priv->ysize);

		return 0;
	} else if (dpu_id == DPU_MODE_MIPI) {

		struct video_tx_device *video_tx;

		disp_uc_plat = dev_get_uclass_plat(disp);

		pr_info("Found device '%s', disp_uc_priv=%p\n", disp->name, disp_uc_plat);

		disp_uc_plat->source_id = remote_dpu_id;
		disp_uc_plat->src_dev = dev;

		ret = device_probe(disp);
		if (ret) {
			pr_info("%s: device '%s' display won't probe (ret=%d)\n",
			  __func__, dev->name, ret);
			return ret;
		}

		ret = display_read_timing(disp, &timing);
		if (ret) {
			pr_info("%s: Failed to read timings\n", __func__);
			return ret;
		}

		ret = display_enable(disp, 1 << VIDEO_BPP32, &timing);
		if (ret) {
			pr_info("%s: Failed to display enable\n", __func__);
			return ret;
		}

		ret = spacemit_panel_init();
		if (ret) {
			pr_info("%s: Failed to init panel\n", __func__);
			return ret;
		}

		spacemit_mode = &fbi.mode;
		uc_priv->xsize = spacemit_mode->xres;
		uc_priv->ysize = spacemit_mode->yres;

		dsi_dpu_init(spacemit_mode, fbbase);
		video_tx_esd_check(fbi.tx);
		video_tx = fbi.tx;
		video_tx->driver->bl_enable(video_tx, true);

		pr_info("fb=%lx, size=%d %d\n", fbbase, uc_priv->xsize, uc_priv->ysize);

		return 0;
	}

	return 0;
}

static int spacemit_dpu_probe(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct spacemit_dpu_priv *priv = dev_get_priv(dev);
	struct udevice *udev;
	ofnode port, node;
	int ret;

	priv->regs_dsi = dev_remap_addr_name(dev, "dsi");
	if (!priv->regs_dsi)
		return -EINVAL;

	priv->regs_hdmi = dev_remap_addr_name(dev, "hdmi");
	if (!priv->regs_hdmi)
		return -EINVAL;

	port = dev_read_subnode(dev, "port");
	if (!ofnode_valid(port)) {
		pr_info("%s(%s): 'port' subnode not found\n",
		      __func__, dev_read_name(dev));
		return -EINVAL;
	}

	for (uclass_find_first_device(UCLASS_VIDEO, &udev);
		udev;
		uclass_find_next_device(&udev)) {
		pr_info("%s:video device %s \n", __func__, udev->name);
	}

	for (uclass_find_first_device(UCLASS_DISPLAY, &udev);
		udev;
		uclass_find_next_device(&udev)) {

		pr_info("%s:display device %s\n", __func__, udev->name);
	}

	for (uclass_find_first_device(UCLASS_VIDEO_BRIDGE, &udev);
		udev;
		uclass_find_next_device(&udev)) {

		pr_info("%s:bridge device %s\n", __func__, udev->name);
	}

	for (node = ofnode_first_subnode(port);
		ofnode_valid(node);
		node = dev_read_next_subnode(node)) {

		ret = spacemit_display_init(dev, plat->base, node);
		if (ret)
			pr_debug("Device failed: ret=%d\n", ret);
		if (!ret)
			break;
	}

	video_set_flush_dcache(dev, 1);

	return 0;
}

static int spacemit_dpu_remove(struct udevice *dev)
{
	return 0;
}

struct spacemit_dpu_driverdata dpu_driverdata = {
	.features = DPU_FEATURE_OUTPUT_10BIT,
};

static const struct udevice_id spacemit_dc_ids[] = {
	{ .compatible = "spacemit,dpu",
	  .data = (ulong)&dpu_driverdata },
	{ }
};

static const struct video_ops spacemit_dpu_ops = {
};

int spacemit_dpu_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	pr_debug("%s,%d,plat->size = %d plat->base 0x%lx\n",__func__,__LINE__,plat->size, plat->base);

	plat->size = 4 * (CONFIG_VIDEO_SPACEMIT_MAX_XRES *
			  CONFIG_VIDEO_SPACEMIT_MAX_YRES);

	return 0;
}

U_BOOT_DRIVER(spacemit_dpu) = {
	.name	= "spacemit_dpu",
	.id	= UCLASS_VIDEO,
	.of_match = spacemit_dc_ids,
	.ops	= &spacemit_dpu_ops,
	.bind	= spacemit_dpu_bind,
	.probe	= spacemit_dpu_probe,
    .remove = spacemit_dpu_remove,
	.priv_auto	= sizeof(struct spacemit_dpu_priv),
};
