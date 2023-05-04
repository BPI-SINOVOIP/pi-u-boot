// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2015 Google, Inc
 * (C) Copyright 2001-2015
 * DENX Software Engineering -- wd@denx.de
 * Compulab Ltd - http://compulab.co.il/
 * Bernecker & Rainer Industrieelektronik GmbH - http://www.br-automation.com
 */

#include <common.h>
#include <dm.h>
#include <video.h>
#include <video_console.h>
#include <video_font.h>		/* Get font data, width and height */

static int console_normal_set_row(struct udevice *dev, uint row, int clr)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);
	struct video_priv *vid_priv = dev_get_uclass_priv(dev->parent);
	void *line;
	int pixels = VID_TO_PIXEL(vc_priv->xsize_frac);
	int i, j;

	line = vid_priv->fb + (row * VIDEO_FONT_HEIGHT + vc_priv->ystart) * vid_priv->line_length +
	       VID_TO_PIXEL(vc_priv->xstart_frac) * VNBYTES(vid_priv->bpix);

	for (i = 0; i < VIDEO_FONT_HEIGHT; i++) {
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

static int console_normal_move_rows(struct udevice *dev, uint rowdst,
				     uint rowsrc, uint count)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);
	struct video_priv *vid_priv = dev_get_uclass_priv(dev->parent);
	void *dst;
	void *src;

	dst = (vid_priv->fb) +
	      (vc_priv->ystart + rowdst * VIDEO_FONT_HEIGHT) * vid_priv->line_length +
	      (VID_TO_PIXEL(vc_priv->xstart_frac) * VNBYTES(vid_priv->bpix));
	src = (vid_priv->fb) +
	      (vc_priv->ystart + rowsrc * VIDEO_FONT_HEIGHT) * vid_priv->line_length +
	      (VID_TO_PIXEL(vc_priv->xstart_frac) * VNBYTES(vid_priv->bpix));

	int i;
	for (i = 0; i < VIDEO_FONT_HEIGHT * count; i++) {
		memmove(dst, src, VID_TO_PIXEL(vc_priv->xsize_frac) * VNBYTES(vid_priv->bpix));
		dst += vid_priv->line_length;
		src += vid_priv->line_length;
	}

	return 0;
}

static int console_normal_backspace(struct udevice *dev)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);
	struct video_priv *vid_priv = dev_get_uclass_priv(dev->parent);
	void *line;
	int i, j;

	line = vid_priv->fb + (vc_priv->ystart + vc_priv->ycur) * vid_priv->line_length +
	       (VID_TO_PIXEL(vc_priv->xstart_frac + vc_priv->xcur_frac) - VIDEO_FONT_WIDTH) * VNBYTES(vid_priv->bpix);

	for (i = 0; i < VIDEO_FONT_HEIGHT; i++) {
		switch (vid_priv->bpix) {
#ifdef CONFIG_VIDEO_BPP8
		case VIDEO_BPP8: {
			uint8_t *dst = line;

			for (j = 0; j < VIDEO_FONT_WIDTH; j++) {
				*dst++ = vc_priv->colour_bg;
			}
			break;
		}
#endif
#ifdef CONFIG_VIDEO_BPP16
		case VIDEO_BPP16: {
			uint16_t *dst = line;

			for (j = 0; j < VIDEO_FONT_WIDTH; j++) {
				*dst++ = vc_priv->colour_bg;
			}
			break;
		}
#endif
#ifdef CONFIG_VIDEO_BPP32
		case VIDEO_BPP32: {
			uint32_t *dst = line;

			for (j = 0; j < VIDEO_FONT_WIDTH; j++) {
				*dst++ = vc_priv->colour_bg;
			}
			break;
		}
#endif
		default:
			return -ENOSYS;
		}
		line += vid_priv->line_length;
	}
	vc_priv->xcur_frac -= VID_TO_POS(vc_priv->x_charsize);
	if (vc_priv->xcur_frac < 0) {
		vc_priv->xcur_frac = (vc_priv->cols - 1) *
			VID_TO_POS(vc_priv->x_charsize);
		vc_priv->ycur -= vc_priv->y_charsize;
		if (vc_priv->ycur < 0)
			vc_priv->ycur = 0;
	}
	return 0;
}

static int console_normal_putc_xy(struct udevice *dev, uint x_frac, uint y,
				  char ch)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);
	struct udevice *vid = dev->parent;
	struct video_priv *vid_priv = dev_get_uclass_priv(vid);
	int i, row;

	if (x_frac + VID_TO_POS(vc_priv->x_charsize) > vc_priv->xsize_frac)
		return -EAGAIN;

	x_frac += vc_priv->xstart_frac;
	y += vc_priv->ystart;

	void *line = vid_priv->fb + y * vid_priv->line_length +
		VID_TO_PIXEL(x_frac) * VNBYTES(vid_priv->bpix);

	for (row = 0; row < VIDEO_FONT_HEIGHT; row++) {
		unsigned int idx = (u8)ch * VIDEO_FONT_HEIGHT + row;
		uchar bits = video_fontdata[idx];

		switch (vid_priv->bpix) {
#ifdef CONFIG_VIDEO_BPP8
		case VIDEO_BPP8: {
			uint8_t *dst = line;

			for (i = 0; i < VIDEO_FONT_WIDTH; i++) {
				*dst++ = (bits & 0x80) ? vc_priv->colour_fg
					: vc_priv->colour_bg;
				bits <<= 1;
			}
			break;
		}
#endif
#ifdef CONFIG_VIDEO_BPP16
		case VIDEO_BPP16: {
			uint16_t *dst = line;

			for (i = 0; i < VIDEO_FONT_WIDTH; i++) {
				*dst++ = (bits & 0x80) ? vc_priv->colour_fg
					: vc_priv->colour_bg;
				bits <<= 1;
			}
			break;
		}
#endif
#ifdef CONFIG_VIDEO_BPP32
		case VIDEO_BPP32: {
			uint32_t *dst = line;

			for (i = 0; i < VIDEO_FONT_WIDTH; i++) {
				*dst++ = (bits & 0x80) ? vc_priv->colour_fg
					: vc_priv->colour_bg;
				bits <<= 1;
			}
			break;
		}
#endif
		default:
			return -ENOSYS;
		}
		line += vid_priv->line_length;
	}

	return VID_TO_POS(VIDEO_FONT_WIDTH);
}

static int console_normal_probe(struct udevice *dev)
{
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(dev);

	vc_priv->x_charsize = VIDEO_FONT_WIDTH;
	vc_priv->y_charsize = VIDEO_FONT_HEIGHT;

	return 0;
}

static const struct udevice_id console_normal_ids[] = {
	{ .compatible = "console-normal", },
	{ }
};

struct vidconsole_ops console_normal_ops = {
	.putc_xy	= console_normal_putc_xy,
	.move_rows	= console_normal_move_rows,
	.set_row	= console_normal_set_row,
	.backspace	= console_normal_backspace,
};

U_BOOT_DRIVER(vidconsole_normal) = {
	.name	= "vidconsole0",
	.id	= UCLASS_VIDEO_CONSOLE,
	.of_match = console_normal_ids,
	.ops	= &console_normal_ops,
	.probe	= console_normal_probe,
};
