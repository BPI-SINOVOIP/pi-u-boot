// SPDX-License-Identifier: GPL-2.0+
/*
 * Charging animation with wave effect
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <video.h>
#include <video_console.h>
#include <video_font.h>
#include <linux/delay.h>
#include <stdlib.h>
#include <dm/uclass.h>
#include <stdint.h>

/*
 * Basic configuration
 */
struct battery_config {
	int width;
	int height;
	int border;
	uint32_t border_color;
	uint32_t fill_color;
	uint32_t fill_color_light;
	uint32_t text_color;
};

/*
 * An 8 columns × 12 rows digit dot matrix.
 * digits_8x12[10][12], each element is 8-bit, corresponding to one row of pixels.
 */
const uint8_t digits_8x12[10][12] = {
	/*
	 * Digit '0'
	 */
	{
		0x3C, // 00111100
		0x7E, // 01111110
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0x7E, // 01111110
		0x3C  // 00111100
	},

	/*
	 * Digit '1'
	 */
	{
		0x18, // 00011000
		0x38, // 00111000
		0x78, // 01111000
		0x18, // 00011000
		0x18, // 00011000
		0x18, // 00011000
		0x18, // 00011000
		0x18, // 00011000
		0x18, // 00011000
		0x18, // 00011000
		0x7E, // 01111110
		0x7E  // 01111110
	},

	/*
	 * Digit '2'
	 */
	{
		0x3C, // 00111100
		0x7E, // 01111110
		0xC3, // 11000011
		0x03, // 00000011
		0x06, // 00000110
		0x0C, // 00001100
		0x18, // 00011000
		0x30, // 00110000
		0x60, // 01100000
		0xC0, // 11000000
		0xFF, // 11111111
		0xFF  // 11111111
	},

	/*
	 * Digit '3'
	 */
	{
		0x7E, // 01111110
		0xFF, // 11111111
		0x03, // 00000011
		0x03, // 00000011
		0x1E, // 00011110
		0x1E, // 00011110
		0x03, // 00000011
		0x03, // 00000011
		0x03, // 00000011
		0x03, // 00000011
		0xFF, // 11111111
		0x7E  // 01111110
	},

	/*
	 * Digit '4'
	 */
	{
		0x06, // 00000110
		0x0E, // 00001110
		0x1E, // 00011110
		0x36, // 00110110
		0x66, // 01100110
		0xC6, // 11000110
		0xFF, // 11111111
		0xFF, // 11111111
		0x06, // 00000110
		0x06, // 00000110
		0x06, // 00000110
		0x06  // 00000110
	},

	/*
	 * Digit '5'
	 */
	{
		0xFF, // 11111111
		0xFF, // 11111111
		0xC0, // 11000000
		0xC0, // 11000000
		0xFC, // 11111100
		0xFE, // 11111110
		0x06, // 00000110
		0x03, // 00000011
		0x03, // 00000011
		0xC3, // 11000011
		0x7E, // 01111110
		0x3C  // 00111100
	},

	/*
	 * Digit '6'
	 */
	{
		0x3E, // 00111110
		0x7F, // 01111111
		0xE3, // 11100011
		0xC0, // 11000000
		0xFC, // 11111100
		0xFE, // 11111110
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0xE3, // 11100011
		0x7E, // 01111110
		0x3C  // 00111100
	},

	/*
	 * Digit '7'
	 */
	{
		0xFF, // 11111111
		0xFF, // 11111111
		0x03, // 00000011
		0x06, // 00000110
		0x0C, // 00001100
		0x18, // 00011000
		0x30, // 00110000
		0x30, // 00110000
		0x60, // 01100000
		0x60, // 01100000
		0x60, // 01100000
		0x60  // 01100000
	},

	/*
	 * Digit '8'
	 */
	{
		0x3C, // 00111100
		0x7E, // 01111110
		0xC3, // 11000011
		0xC3, // 11000011
		0x7E, // 01111110
		0x7E, // 01111110
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0xC3, // 11000011
		0x7E, // 01111110
		0x3C  // 00111100
	},

	/*
	 * Digit '9'
	 */
	{
		0x3C, // 00111100
		0x7E, // 01111110
		0xC3, // 11000011
		0xC3, // 11000011
		0xCF, // 11001111
		0xDF, // 11011111
		0xFB, // 11111011
		0x7F, // 01111111
		0x03, // 00000011
		0x03, // 00000011
		0x7E, // 01111110
		0x3C  // 00111100
	},
};

/*
 * Function to write a pixel
 */
static void video_write_pixel(struct udevice *dev, int x, int y, u32 color)
{
	struct video_priv *priv = dev_get_uclass_priv(dev);
	void *fb = priv->fb;
	int line_length = priv->line_length;

	/* Boundary check */
	if (x < 0 || x >= priv->xsize || y < 0 || y >= priv->ysize)
		return;

	/* Calculate pixel position */
	unsigned long pos = y * line_length + x * 4; // 4 bytes per pixel

	/* Write 32-bit color value */
	u32 *dst = (u32 *)(fb + pos);
	*dst = color;
}

/*
 * Get the video device
 */
static struct udevice *get_video_device(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device(UCLASS_VIDEO, 0, &dev);
	if (ret) {
		printf("Cannot find video device\n");
		return NULL;
	}
	return dev;
}

/*
 * Get color based on battery level
 */
static u32 get_battery_color(int level)
{
	if (level <= 20) {
		return 0xFFFF0000; // Red (low battery)
	} else if (level <= 60) {
		return 0xFFFFFF00; // Yellow (medium battery)
	} else {
		return 0xFF00FF00; // Green (high battery)
	}
}

/*
 * A simple battery fill effect
 */
static void draw_effect(struct udevice *dev, struct battery_config *cfg, int level)
{
	struct video_priv *priv = dev_get_uclass_priv(dev);
	int x = (priv->xsize - cfg->width) / 2;
	int y = (priv->ysize - cfg->height) / 2;

	/* Calculate fill height */
	int fill_height = (cfg->height - 2 * cfg->border) * level / 100;
	int start_y = y + cfg->height - cfg->border - fill_height;

	/* Get the color corresponding to current battery level */
	u32 color = get_battery_color(level);

	/* Inner boundary of the fill area */
	int inner_left = x + cfg->border;
	int inner_right = x + cfg->width - cfg->border;
	int inner_bottom = y + cfg->height - cfg->border;

	/* Fill a simple rectangle area */
	for (int i = inner_left; i < inner_right; i++) {
		for (int j = start_y; j < inner_bottom; j++) {
			video_write_pixel(dev, i, j, color);
		}
	}
}

/*
 * Draw the battery outline
 */
static void draw_battery_outline(struct udevice *dev, struct battery_config *cfg)
{
	struct video_priv *priv = dev_get_uclass_priv(dev);
	int x = (priv->xsize - cfg->width) / 2;
	int y = (priv->ysize - cfg->height) / 2;
	int i, j;

	/*
	 * Draw the main outline (rectangle)
	 * Draw horizontal borders
	 */
	for (i = 0; i < cfg->width; i++) {
		/* Top border */
		for (j = 0; j < cfg->border; j++) {
			video_write_pixel(dev, x + i, y + j, cfg->border_color);
		}
		/* Bottom border */
		for (j = 0; j < cfg->border; j++) {
			video_write_pixel(dev, x + i, y + cfg->height - 1 - j, cfg->border_color);
		}
	}

	/*
	 * Draw vertical borders
	 */
	for (j = 0; j < cfg->height; j++) {
		/* Left border */
		for (i = 0; i < cfg->border; i++) {
			video_write_pixel(dev, x + i, y + j, cfg->border_color);
		}
		/* Right border */
		for (i = 0; i < cfg->border; i++) {
			video_write_pixel(dev, x + cfg->width - 1 - i, y + j, cfg->border_color);
		}
	}

	/*
	 * Draw the battery cap (rectangle design)
	 */
	int cap_width = cfg->width / 4;
	int cap_height = cfg->height / 8;
	int cap_x = x + (cfg->width - cap_width) / 2;
	int cap_y = y - cap_height;

	/* Draw the rectangle battery cap */
	for (i = 0; i < cap_width; i++) {
		for (j = 0; j < cap_height; j++) {
			video_write_pixel(dev, cap_x + i, cap_y + j, cfg->border_color);
		}
	}
}

/*
 * Modified digit display function, adding a scaling feature
 */
static void draw_digit(struct udevice *dev, int x, int y, int digit, u32 color, int scale)
{
	/* Using 8x12 dot matrix */
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 12; j++) {
			if (digits_8x12[digit][j] & (0x80 >> i)) {
				/*
				 * Draw a scaled-up block of pixels
				 */
				for (int sx = 0; sx < scale; sx++) {
					for (int sy = 0; sy < scale; sy++) {
						video_write_pixel(dev,
							x + i * scale + sx,
							y + j * scale + sy,
							color);
					}
				}
			}
		}
	}
}

/*
 * Modified text display function
 */
static void draw_battery_text(struct udevice *dev, struct battery_config *cfg, int level)
{
	struct video_priv *priv = dev_get_uclass_priv(dev);

	/* Increase scale factor */
	int scale = 2;

	/* Calculate text position */
	int center_x = (priv->xsize - cfg->width) / 2 + cfg->width / 2;
	int center_y = (priv->ysize - cfg->height) / 2 + cfg->height / 2 - 10;

	/* Adjust position considering the scaled size */
	int digit_width = 8 * scale;   // 8 columns
	int digit_height = 12 * scale; // 12 rows

	/* Compute digits */
	int hundreds = level / 100;
	int tens = (level % 100) / 10;
	int ones = level % 10;

	/* Adjust the starting x position based on how many digits we have */
	int start_x;
	if (hundreds > 0) {
		/* Three digits, move further left by one digit width */
		start_x = center_x - digit_width * 2;
	} else if (tens > 0) {
		/* Two digits */
		start_x = center_x - digit_width - digit_width / 2;
	} else {
		/* Single digit */
		start_x = center_x - digit_width / 2;
	}

	/* Draw hundreds digit */
	if (hundreds > 0) {
		draw_digit(dev,
			   start_x,
			   center_y - digit_height / 2,
			   hundreds, 0xFFFFFFFF, scale);
		start_x += digit_width;
	}

	/* Draw tens digit */
	if (hundreds > 0 || tens > 0) {
		draw_digit(dev,
			   start_x,
			   center_y - digit_height / 2,
			   tens, 0xFFFFFFFF, scale);
		start_x += digit_width;
	}

	/* Draw ones digit */
	draw_digit(dev,
		   start_x,
		   center_y - digit_height / 2,
		   ones, 0xFFFFFFFF, scale);

	video_sync(dev, false);
}

/*
 * Display battery icon with specified charge level
 */
void battery_level_display(int target_level)
{
	struct udevice *dev = get_video_device();
	if (!dev) {
		printf("Error: Failed to get video device in battery_level_display\n");
		return;
	}

	struct video_priv *priv = dev_get_uclass_priv(dev);
	if (!priv) {
		printf("Error: Failed to get video private data\n");
		return;
	}

	/* Adjust battery size */
	int battery_width = priv->xsize / 6;
	int battery_height = battery_width * 2;

	struct battery_config cfg = {
		.width = battery_width,
		.height = battery_height,
		.border = battery_width / 25,
		.border_color = 0xFFFFFFFF,
		.fill_color = 0xFF00FF00,
		.fill_color_light = 0xFF80FF80,
		.text_color = 0xFFFFFFFF,
	};

	/* Ensure the level is within valid range */
	int level = target_level;
	if (level < 0) level = 0;
	if (level > 100) level = 100;

	/* Clear the screen and draw the battery outline */
	video_clear(dev);
	draw_battery_outline(dev, &cfg);

	/* Draw the fill based on battery level */
	draw_effect(dev, &cfg, level);

	/* Draw the numeric battery level */
	draw_battery_text(dev, &cfg, level);

	/* Refresh the display */
	video_sync(dev, false);
}

/*
 * Clear the display
 */
void clear_battery_display(void)
{
	struct udevice *dev = get_video_device();
	if (!dev) {
		printf("Error: Failed to get video device in clear_battery_display\n");
		return;
	}

	/* Clear the screen */
	video_clear(dev);

	/* Refresh the display */
	video_sync(dev, false);
}