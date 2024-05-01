// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_VIDEO_TX_H_
#define _SPACEMIT_VIDEO_TX_H_

#include <linux/types.h>
#include <linux/errno.h>
#include <stdio.h>
#include <common.h>
#include <asm-generic/gpio.h>


#define INVALID_GPIO	0x0FFFFFFF
#define LCD_DUMMY		0xFFFF
#define DEFAULT_ID		0x1901

enum {
	DPMS_OFF = 0,
	DPMS_ON = 1,
};

enum panel_type {
	LCD_MIPI = 0,
	LCD_HDMI = 1,
	LCD_NULL
};

#define OUTFMT_RGB121212	0
#define OUTFMT_RGB101010	1
#define OUTFMT_RGB888		2
#define OUTFMT_RGB666		12
#define OUTFMT_RGB565		13

enum pix_fmt {
	PIXFMT_RGB565 = 0,
	PIXFMT_RGB1555,
	PIXFMT_RGB888PACK,
	PIXFMT_RGB888UNPACK,
	PIXFMT_RGBA888,
	PIXFMT_YUV422_PACK,
	PIXFMT_YUV422P,
	PIXFMT_YUV420P,
	PIXFMT_RGB888A = 0xB,
	PIXFMT_YUV420SP = 0xC,

	PIXFMT_PSEUDOCOLOR = 0x200,
};

enum vdma_fmt {
	DMA_FMT_RGB = 0x0,
	DMA_FMT_YUV422P = 0x4,
	DMA_FMT_YUV420P = 0x6,	/*3 planes*/
	DMA_FMT_YUV420SP = 0x7,	/*2 planes*/
};

struct spacemit_mode_modeinfo {
	const char *name;
	unsigned int refresh;
	unsigned int xres;
	unsigned int yres;
	unsigned int real_xres;
	unsigned int real_yres;
	unsigned int left_margin;
	unsigned int right_margin;
	unsigned int upper_margin;
	unsigned int lower_margin;
	unsigned int hsync_len;
	unsigned int vsync_len;
	unsigned int hsync_invert;
	unsigned int vsync_invert;
	unsigned int invert_pixclock;
	unsigned int pixclock_freq;
	int pix_fmt_out;
	uint32_t height; /* screen height in mm */
	uint32_t width; /* screen width in mm */
};

struct lcd_mipi_panel_info {
	char *lcd_name;
	unsigned int lcd_id;
	unsigned int panel_id0;
	unsigned int panel_id1;
	unsigned int panel_id2;
	unsigned int power_value;
	enum panel_type panel_type;
	uint32_t width_mm;
	uint32_t height_mm;
	uint32_t dft_pwm_bl;
	unsigned int set_power_cmds_num;
	unsigned int read_power_cmds_num;
	unsigned int set_id_cmds_num;
	unsigned int set_backlight_value_cmds_num;
	unsigned int set_backlight_pwm_freq_cmds_num;
	unsigned int read_id_cmds_num;
	unsigned int init_cmds_num;
	unsigned int sleep_out_cmds_num;
	unsigned int sleep_in_cmds_num;
	struct drm_mode_modeinfo *drm_modeinfo;
	struct spacemit_mode_modeinfo *spacemit_modeinfo;
	struct spacemit_mipi_info *mipi_info;
	struct spacemit_dsi_cmd_desc *set_power_cmds;
	struct spacemit_dsi_cmd_desc *set_backlight_value_cmds;
	struct spacemit_dsi_cmd_desc *set_backlight_pwm_freq_cmds;
	struct spacemit_dsi_cmd_desc *read_power_cmds;
	struct spacemit_dsi_cmd_desc *set_id_cmds;
	struct spacemit_dsi_cmd_desc *read_id_cmds;
	struct spacemit_dsi_cmd_desc *init_cmds;
	struct spacemit_dsi_cmd_desc *sleep_out_cmds;
	struct spacemit_dsi_cmd_desc *sleep_in_cmds;
	void (*set_backlight_value)(int, int);
	unsigned int bitclk_sel;
	unsigned int bitclk_div;
	unsigned int pxclk_sel;
	unsigned int pxclk_div;
};

struct video_tx_device {
	const struct video_tx_driver *driver;
	enum panel_type panel_type;
	void *private;
};

struct video_tx_driver {
	/* client driver ops */
	/* Retrieve a list of modes supported by the display */
	int (*get_modes)(struct video_tx_device *, struct spacemit_mode_modeinfo *);
	/* Set the DPMS status of the display */
	int (*dpms)(struct video_tx_device *, int);
	int (*identify)(struct video_tx_device *);
	bool (*esd_check)(struct video_tx_device *);
	int (*panel_reset)(struct video_tx_device *);
	int (*bl_enable)(struct video_tx_device *, bool enable);
};

struct lcd_mipi_tx_data {
	int dpms_status;
	enum panel_type panel_type;
	struct lcd_mipi_panel_info *panel_info;
	struct spacemit_panel_priv *priv;
};

struct spacemit_panel_priv {
	struct gpio_desc ldo_1v2_gpio;
	unsigned int ldo_1v8;
	unsigned int ldo_2v8;
	unsigned int ldo_1v2;
	unsigned int bl_pwm;

	struct gpio_desc dcp;
	struct gpio_desc dcn;
	struct gpio_desc bl;
	struct gpio_desc enable;
	struct gpio_desc reset;

};
extern int lcd_id;
extern int lcd_width;
extern int lcd_height;
extern char *lcd_name;

/* Host functions */
struct video_tx_device *find_video_tx(void);
int video_tx_get_modes(struct video_tx_device *video_tx,
		       struct spacemit_mode_modeinfo *modelist);
int video_tx_dpms(struct video_tx_device *video_tx, int mode);
void video_tx_esd_check(struct video_tx_device *video_tx);

/* Client functions */
int video_tx_register_device(struct video_tx_device *tx_device);
void *video_tx_get_drvdata(struct video_tx_device *tx_device);

int lcd_mipi_register_panel(struct lcd_mipi_panel_info *panel_info);

#endif /* _SPACEMIT_VIDEO_TX_H_ */
