/*
 * Copyright (c) 2015 Google, Inc
 * (C) Copyright 2001-2015
 * DENX Software Engineering -- wd@denx.de
 * Compulab Ltd - http://compulab.co.il/
 * Bernecker & Rainer Industrieelektronik GmbH - http://www.br-automation.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <video.h>
#include <video_console.h>
#include "fonts/video_font_7x16_png.h"		/* Get font data, width and height */

struct console_priv {
	u8 *font_data;
	int bold;
};

static int console_png_set_row(struct udevice *dev, uint row, int clr)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);
	struct video_priv *vid_priv = dev_get_uclass_priv(dev->parent);
	void *line;
	int pixels = VID_TO_PIXEL(vc_priv->xsize_frac);
	int i, j;

	line = vid_priv->fb + (row * vc_priv->y_charsize + vc_priv->ystart) * vid_priv->line_length +
	       VID_TO_PIXEL(vc_priv->xstart_frac) * VNBYTES(vid_priv->bpix);

	for (i = 0; i < vc_priv->y_charsize; i++) {
		switch (vid_priv->bpix) {
#ifdef CONFIG_VIDEO_BPP8
		case VIDEO_BPP8: {
			uint8_t *dst = line;

			for (j = 0; j < pixels; j++) {
				*dst++ = clr;
			}
			break;
		}
#endif
#ifdef CONFIG_VIDEO_BPP16
		case VIDEO_BPP16: {
			uint16_t *dst = line;

			for (j = 0; j < pixels; j++) {
				*dst++ = clr;
			}
			break;
		}
#endif
#ifdef CONFIG_VIDEO_BPP32
		case VIDEO_BPP32: {
			uint32_t *dst = line;

			for (j = 0; j < pixels; j++) {
				*dst++ = clr;
			}
			break;
		}
#endif
		default:
			return -ENOSYS;
		}
		line += vid_priv->line_length;
	}

	return 0;
}

static int console_png_move_rows(struct udevice *dev, uint rowdst,
				     uint rowsrc, uint count)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);
	struct video_priv *vid_priv = dev_get_uclass_priv(dev->parent);
	void *dst;
	void *src;

	dst = (vid_priv->fb) +
	      (vc_priv->ystart + rowdst * vc_priv->y_charsize) * vid_priv->line_length +
	      (VID_TO_PIXEL(vc_priv->xstart_frac) * VNBYTES(vid_priv->bpix));
	src = (vid_priv->fb) +
	      (vc_priv->ystart + rowsrc * vc_priv->y_charsize) * vid_priv->line_length +
	      (VID_TO_PIXEL(vc_priv->xstart_frac) * VNBYTES(vid_priv->bpix));

	int i;
	for (i = 0; i < vc_priv->y_charsize * count; i++) {
		memmove(dst, src, VID_TO_PIXEL(vc_priv->xsize_frac) * VNBYTES(vid_priv->bpix));
		dst += vid_priv->line_length;
		src += vid_priv->line_length;
	}

	return 0;
}

static int console_png_backspace(struct udevice *dev)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);

	vc_priv->xcur_frac -= VID_TO_POS(vc_priv->x_charsize);
	if (vc_priv->xcur_frac < 0) {
		vc_priv->xcur_frac = (vc_priv->cols - 1) *
			VID_TO_POS(vc_priv->x_charsize);
		vc_priv->ycur -= vc_priv->y_charsize;
		if (vc_priv->ycur < 0)
			vc_priv->ycur = 0;
	}

	video_fill(dev->parent, VID_TO_PIXEL(vc_priv->xstart_frac + vc_priv->xcur_frac), vc_priv->x_charsize,
		(vc_priv->ystart + vc_priv->ycur), vc_priv->y_charsize, vc_priv->colour_bg);

	return 0;
}

static int console_png_putc_xy(struct udevice *dev, uint x_frac, uint y,
				  char ch)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);
	struct udevice *vid = dev->parent;
	struct video_priv *vid_priv = dev_get_uclass_priv(vid);
	struct console_priv *priv = dev_get_priv(dev);
	int i, row;

	if (x_frac + VID_TO_POS(vc_priv->x_charsize) > vc_priv->xsize_frac)
		return -EAGAIN;

	x_frac += vc_priv->xstart_frac;
	y += vc_priv->ystart;

	void *line = vid_priv->fb + y * vid_priv->line_length +
		VID_TO_PIXEL(x_frac) * VNBYTES(vid_priv->bpix);

	int off = ch - 32;
	if (off < 0 || off >= 96)
		return -ENOSYS;

	u8 *font_line = priv->font_data + (off * font.cwidth);
	if (priv->bold)
		font_line += font.cheight * font.width;

	for (row = 0; row < vc_priv->y_charsize; row++) {
		switch (vid_priv->bpix) {
#ifdef CONFIG_VIDEO_BPP8
		case VIDEO_BPP8: {
			uint8_t *dst = line;
			u8 *src = font_line;

			for (i = 0; i < vc_priv->x_charsize; i++) {
				*dst++ = (*src++ & 0x80) ? vc_priv->colour_fg
					: vc_priv->colour_bg;
			}
			break;
		}
#endif
#ifdef CONFIG_VIDEO_BPP16
		case VIDEO_BPP16: {
			uint16_t *dst = line;
			u8 *src = font_line;

			for (i = 0; i < vc_priv->x_charsize; i++) {
				*dst++ = (*src++ & 0x80) ? vc_priv->colour_fg
					: vc_priv->colour_bg;
			}
			break;
		}
#endif
#ifdef CONFIG_VIDEO_BPP32
		case VIDEO_BPP32: {
			uint32_t *dst = line;
			u8 *src = font_line;

			for (i = 0; i < vc_priv->x_charsize; i++) {
				*dst++ = (*src++ & 0x80) ? vc_priv->colour_fg
					: vc_priv->colour_bg;
			}
			break;
		}
#endif
		default:
			return -ENOSYS;
		}
		font_line += font.width;
		line += vid_priv->line_length;
	}

	return VID_TO_POS(vc_priv->x_charsize);
}

static int console_png_probe(struct udevice *dev)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);
	struct console_priv *priv = dev_get_priv(dev);

	u8 *out = (u8 *)malloc(font.width * font.height * 2);
	u8 *in = font.rundata;
	priv->font_data = out;
	priv->bold = 0;

	u8 data;
	while((data = *in++)) {
		memset(out, (data & 0x80) ? 255 : 0, data & 0x7f);
		out += (data & 0x7f);
	}

	vc_priv->x_charsize = font.cwidth;
	vc_priv->y_charsize = font.cheight;

	return 0;
}

static const struct udevice_id console_png_ids[] = {
	{ .compatible = "console-font-png", },
	{ }
};

struct vidconsole_ops console_png_ops = {
	.putc_xy	= console_png_putc_xy,
	.move_rows	= console_png_move_rows,
	.set_row	= console_png_set_row,
	.backspace	= console_png_backspace,
};

U_BOOT_DRIVER(vidconsole_png) = {
	.name	= "vidconsole-png",
	.id	= UCLASS_VIDEO_CONSOLE,
	.of_match = console_png_ids,
	.ops	= &console_png_ops,
	.probe	= console_png_probe,
	.priv_auto_alloc_size	= sizeof(struct console_priv),
};
