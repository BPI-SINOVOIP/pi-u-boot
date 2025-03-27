// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <linux/kernel.h>
#include "../include/spacemit_dsi_common.h"
#include "../include/spacemit_video_tx.h"
#include <linux/delay.h>
#include <command.h>
#include <dm/device.h>
#include <dm/read.h>
#include <regmap.h>
#include <syscon.h>
#include <video.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <backlight.h>

#define PANEL_NUM_MAX	5

int panel_num = 0;
struct lcd_mipi_panel_info *panels[PANEL_NUM_MAX] = {0};
int lcd_id = 0;
int lcd_width = 0;
int lcd_height = 0;
char *lcd_name = NULL;

static bool __maybe_unused lcd_mipi_readid(struct lcd_mipi_tx_data *video_tx_client)
{
	struct spacemit_dsi_rx_buf dbuf;
	uint32_t read_id[3] = {0};
	int i;
	int ret = 0;

	for(i=0;i<1;i++){
		spacemit_mipi_write_cmds(0, video_tx_client->panel_info->set_id_cmds,
			video_tx_client->panel_info->set_id_cmds_num);

		ret = spacemit_mipi_read_cmds(0, &dbuf, video_tx_client->panel_info->read_id_cmds,
				 video_tx_client->panel_info->read_id_cmds_num);
		if (ret) {
			pr_info("%s failed!\n", __func__);
			return false;
		}
		read_id[0] = dbuf.data[0];
		read_id[1] = dbuf.data[1];
		read_id[2] = dbuf.data[2];

	    if((read_id[0] != video_tx_client->panel_info->panel_id0)
			|| (read_id[1] != video_tx_client->panel_info->panel_id1)
			|| (read_id[2] != video_tx_client->panel_info->panel_id2)) {
			pr_info("read panel id: read value = 0x%x, 0x%x, 0x%x\n", read_id[0], read_id[1], read_id[2]);
	    } else {
			pr_info("read panel id OK: read value = 0x%x, 0x%x, 0x%x\n", read_id[0], read_id[1], read_id[2]);
			return true;
	    }
	}
	return false;
}

static int lcd_mipi_reset(struct spacemit_panel_priv *priv)
{
	/* reset lcm */
	if (priv->reset_valid) {
		dm_gpio_set_value(&priv->reset, 1);
		mdelay(10);
		dm_gpio_set_value(&priv->reset, 0);
		mdelay(10);
		dm_gpio_set_value(&priv->reset, 1);
		mdelay(120);
	}

	return 0;
}

static int lcd_mipi_dc_enable(bool power_on, struct spacemit_panel_priv *priv)
{
	if(power_on){
		if (priv->enable_valid)
			dm_gpio_set_value(&priv->enable, 1);

		if (priv->dcp_valid)
			dm_gpio_set_value(&priv->dcp, 1);
		if (priv->dcn_valid)
			dm_gpio_set_value(&priv->dcn, 1);

		if (priv->avee_valid)
			dm_gpio_set_value(&priv->avee, 1);
		if (priv->avdd_valid)
			dm_gpio_set_value(&priv->avdd, 1);
	} else {
		if (priv->enable_valid)
			dm_gpio_set_value(&priv->enable, 0);

		if (priv->avee_valid)
			dm_gpio_set_value(&priv->avee, 0);
		if (priv->avdd_valid)
			dm_gpio_set_value(&priv->avdd, 0);

		if (priv->dcp_valid)
			dm_gpio_set_value(&priv->dcp, 0);
		if (priv->dcn_valid)
			dm_gpio_set_value(&priv->dcn, 0);
	}

	return 0;
}

static uint32_t lcd_mipi_readpower(struct lcd_mipi_tx_data *video_tx_client)
{
	struct spacemit_dsi_rx_buf dbuf;
	uint32_t power = 0;

	spacemit_mipi_write_cmds(0, video_tx_client->panel_info->set_power_cmds,
		video_tx_client->panel_info->set_power_cmds_num);

	spacemit_mipi_read_cmds(0, &dbuf, video_tx_client->panel_info->read_power_cmds,
		video_tx_client->panel_info->read_power_cmds_num);

	power = dbuf.data[0];

	return power;
}

static bool lcd_mipi_esd_check(struct video_tx_device *dev)
{
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);
	int power = 0;
	int i;

	if (video_tx_client->panel_info->set_power_cmds_num == 0)
		return true;

	for(i = 0; i < 3; i++) {
		power = lcd_mipi_readpower(video_tx_client);

		if(power == video_tx_client->panel_info->power_value) {
			pr_debug("lcd esd check ok! 0x%x\n", power);
			return true;
		} else {
			pr_info("lcd esd check fail, value (0x%x)\n", power);
		}
	}

	return false;
}

static int lcd_mipi_panel_reset(struct video_tx_device *dev)
{
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);
	int ret = 0;


	spacemit_mipi_close_datatx(0);

	ret = lcd_mipi_reset(video_tx_client->priv);
	if (ret) {
		pr_info("lcd_mipi gpio reset failded!\n");
		return -1;
	}

	if (video_tx_client->panel_info->panel_type == LCD_MIPI) {
		ret = spacemit_mipi_write_cmds(0, video_tx_client->panel_info->init_cmds,
				video_tx_client->panel_info->init_cmds_num);
		if(ret) {
			pr_info("send init cmd fail!\n ");
		}
		ret = spacemit_mipi_write_cmds(0, video_tx_client->panel_info->sleep_out_cmds,
				video_tx_client->panel_info->sleep_out_cmds_num);
		if(ret) {
			pr_info("send sleep out fail!\n ");
		}
	}

	ret = spacemit_mipi_ready_for_datatx(0, video_tx_client->panel_info->mipi_info);
	if (0 != ret) {
		pr_info("lcd_mipi spacemit_mipi_ready_for_datatx fail!\n ");
		spacemit_mipi_close(0);
	}

	return 0;
}

static int lcd_mipi_identify(struct video_tx_device *dev)
{
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);
	struct lcd_mipi_panel_info *panel_info = NULL;
	bool is_panel = false;
	int ret = 0;
	int i;

	ret = lcd_mipi_dc_enable(true, video_tx_client->priv);
	if (ret) {
		pr_info("lcd_mipi gpio dc failded!\n");
	}

	for(i=0; i<panel_num; i++) {
		panel_info = panels[i];
		if(!panel_info)
			continue;

		pr_debug("identify lcd (%s)\n",panel_info->lcd_name);

		video_tx_client->panel_info = panel_info;

		if (panel_info->panel_type == LCD_EDP) {

			is_panel = true;
		} else {

			ret = lcd_mipi_reset(video_tx_client->priv);
			if (ret) {
				pr_info("lcd_mipi gpio reset failded!\n");
				continue;
			}

			ret = spacemit_mipi_open(0, video_tx_client->panel_info->mipi_info, false);
			if(0 != ret) {
				pr_info("%s, lcd_mipi open mipi fai!\n", __func__);
				continue;
			}

			is_panel = lcd_mipi_readid(video_tx_client);

			spacemit_mipi_close(0);
		}

		if (!is_panel) {
			//dev_info(video_tx_client->dev, "lcd_mipi read (%s) chip id failded!\n", video_tx_client->panel_info->lcd_name);
			video_tx_client->panel_info = NULL;
			continue;
		}else{
			lcd_mipi_dc_enable(false, video_tx_client->priv);
			pr_info("Panel is %s\n", video_tx_client->panel_info->lcd_name);
			lcd_id = video_tx_client->panel_info->lcd_id;
			lcd_name = video_tx_client->panel_info->lcd_name;
			lcd_width = video_tx_client->panel_info->spacemit_modeinfo->xres;
			lcd_height = video_tx_client->panel_info->spacemit_modeinfo->yres;
			return 1;
		}
	}
	lcd_mipi_dc_enable(false, video_tx_client->priv);

	return 0;
}

static int lcd_mipi_init(struct video_tx_device *dev)
{
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);
	int ret = 0;

	ret = spacemit_mipi_open(0, video_tx_client->panel_info->mipi_info, false);
	if(0 != ret) {
		pr_info("lcd_mipi open mipi fai!\n ");
		return -1;
	}

	if (video_tx_client->panel_info->panel_type == LCD_MIPI) {

		ret = spacemit_mipi_write_cmds(0, video_tx_client->panel_info->init_cmds,
				video_tx_client->panel_info->init_cmds_num);
	}

	return ret;
}

static int lcd_mipi_sleep_out(struct video_tx_device *dev)
{
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);
	int ret = 0;

	pr_debug("lcd_mipi_sleep_out enter!\n");

	ret = lcd_mipi_dc_enable(true, video_tx_client->priv);
	if (ret) {
		pr_info("lcd_mipi gpio dc failded!\n");
		return -1;
	}

	ret = lcd_mipi_init(dev);
	if(0 != ret) {
		pr_info("lcd_mipi init fai!\n ");
		return -1;
	}

	// if (video_tx_client->panel_info->panel_type == LCD_MIPI) {

	// 	ret = spacemit_mipi_write_cmds(0, video_tx_client->panel_info->sleep_out_cmds,
	// 			video_tx_client->panel_info->sleep_out_cmds_num);
	// }

	ret = spacemit_mipi_ready_for_datatx(0, video_tx_client->panel_info->mipi_info);
	if(0 != ret) {
		pr_info("lcd_mipi spacemit_mipi_ready_for_datatx fail!\n ");
		spacemit_mipi_close(0);
	}

	return 0;
}

static int lcd_mipi_get_modes(struct video_tx_device *dev,
			  struct spacemit_mode_modeinfo *mode_info)
{
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);

	if (mode_info == NULL)
		return 0;

	memcpy(mode_info, (void *)(video_tx_client->panel_info->spacemit_modeinfo), sizeof(struct spacemit_mode_modeinfo));
	return 1;
}

static int lcd_mipi_dpms(struct video_tx_device *dev, int status)
{
	char *str_dpms;
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);

	if(status == video_tx_client->dpms_status){
		pr_info("lcd has been in (%d) status\n", status);
		return 0;
	}

	switch (status) {
	case DPMS_ON:
		str_dpms = "DRM_MODE_DPMS_ON";
		lcd_mipi_sleep_out(dev);
		break;
	case DPMS_OFF:
		str_dpms = "DRM_MODE_DPMS_OFF";
		break;
	default:
		pr_info("DPMS: unknown status!\n");
		return -EINVAL;
	}

	video_tx_client->dpms_status = status;
	pr_debug("driver->dpms( %s )\n", str_dpms);

	return 0;
}

static int lcd_bl_enable(struct video_tx_device *dev, bool enable)
{
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);
	struct spacemit_panel_priv *priv = video_tx_client->priv;
	int ret;

	if (priv->bl_valid) {
		if (enable) {
			ret = backlight_set_brightness(priv->backlight, BACKLIGHT_DEFAULT);
			pr_debug("%s: set brightness done, ret = %d\n", __func__, ret);
			if (ret)
				return ret;
			ret = backlight_enable(priv->backlight);
			pr_debug("%s: enable backlight done, ret = %d\n", __func__, ret);
			if (ret)
				return ret;
		} else {
			ret = backlight_set_brightness(priv->backlight, BACKLIGHT_OFF);
			pr_debug("%s: shutdown backlight done, ret = %d\n", __func__, ret);
			if (ret)
				return ret;
		}
	}

	return 0;
}

int lcd_mipi_register_panel(struct lcd_mipi_panel_info *panel_info)
{
	if(panel_num >= PANEL_NUM_MAX) {
		pr_info("%s, panel_num is full!\n", __func__);
		return 0;
	}

	panels[panel_num] = panel_info;
	panel_num++;

	pr_debug("fb: panel %s registered in lcd_mipi!\n", panel_info->lcd_name);

	return 0;
}

static struct video_tx_driver lcd_mipi_driver_tx = {
	.get_modes = lcd_mipi_get_modes,
	.dpms = lcd_mipi_dpms,
	.identify = lcd_mipi_identify,
	.esd_check = lcd_mipi_esd_check,
	.panel_reset = lcd_mipi_panel_reset,
	.bl_enable = lcd_bl_enable,
};

struct lcd_mipi_tx_data tx_device_client = {0};
struct video_tx_device tx_device = {0};

static int lcd_mipi_client_init(struct spacemit_panel_priv *priv)
{
	tx_device_client.panel_type = LCD_MIPI;
	tx_device_client.panel_info = NULL;
	tx_device_client.dpms_status = DPMS_OFF;
	tx_device_client.priv = priv;

	tx_device.driver = &lcd_mipi_driver_tx;
	tx_device.panel_type = tx_device_client.panel_type;
	tx_device.private = &tx_device_client;

	video_tx_register_device(&tx_device);

	return 0;
}

int lcd_mipi_probe(void)
{
	int ret;
	struct udevice *dev = NULL;
	struct spacemit_panel_priv *priv = NULL;

	ret = uclass_get_device_by_driver(UCLASS_NOP,
		DM_DRIVER_GET(spacemit_panel), &dev);
	if (ret) {
		pr_info("spacemit_panel probe failed %d\n", ret);
		return ret;
	}

	priv = dev_get_priv(dev);

	ret = lcd_mipi_client_init(priv);
	if (ret) {
		pr_info("lcd_mipi client init failed\n");
		return ret;
	}

	ret = spacemit_dsi_probe();
	if (ret < 0) {
		pr_info("spacemit_dsi_probe failed\n");
		return ret;
	}

	if (strcmp("lt8911ext_edp_1080p", priv->panel_name) == 0) {
		tx_device_client.panel_type = LCD_EDP;
		tx_device.panel_type = tx_device_client.panel_type;
		lcd_lt8911ext_edp_1080p_init();
	} else if(strcmp("icnl9951r", priv->panel_name) == 0) {
		tx_device_client.panel_type = LCD_MIPI;
		tx_device.panel_type = tx_device_client.panel_type;
		lcd_icnl9951r_init();
	} else if(strcmp("jd9365dah3", priv->panel_name) == 0) {
		tx_device_client.panel_type = LCD_MIPI;
		tx_device.panel_type = tx_device_client.panel_type;
		lcd_jd9365dah3_init();
	} else if(strcmp("ft8201sinx101", priv->panel_name) == 0) {
		tx_device_client.panel_type = LCD_MIPI;
		tx_device.panel_type = tx_device_client.panel_type;
		lcd_ft8201sinx101_init();
	} else {
		// lcd_icnl9911c_init();
		lcd_gx09inx101_init();
	}

	return 0;
}

static const struct udevice_id spacemit_panel_ids[] = {
        { .compatible = "spacemit,panel" },
        { }
};

static int spacemit_panel_of_to_plat(struct udevice *dev)
{

	struct spacemit_panel_priv *priv = dev_get_priv(dev);
	const char *name;
	int ret;

	memset(priv, 0, sizeof(*priv));

	name = dev_read_string(dev, "force-attached");
	if (name) {
		strcpy(priv->panel_name, name);
		pr_info("spacemit_panel_of_to_plat panel %s \n", priv->panel_name);
	}

	ret = gpio_request_by_name(dev, "dcp-gpios", 0, &priv->dcp,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get dcp GPIO: ret=%d\n",
		      __func__, ret);
		priv->dcp_valid = false;
	} else {
		priv->dcp_valid = true;
	}

	ret = gpio_request_by_name(dev, "dcn-gpios", 0, &priv->dcn,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get dcn GPIO: ret=%d\n",
		      __func__, ret);
		priv->dcn_valid = false;
	} else {
		priv->dcn_valid = true;
	}

	ret = gpio_request_by_name(dev, "avee-gpios", 0, &priv->avee,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get avee GPIO: ret=%d\n",
		      __func__, ret);
		priv->avee_valid = false;
	} else {
		priv->avee_valid = true;
	}

	ret = gpio_request_by_name(dev, "avdd-gpios", 0, &priv->avdd,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get avdd GPIO: ret=%d\n",
		      __func__, ret);
		priv->avdd_valid = false;
	} else {
		priv->avdd_valid = true;
	}


	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &priv->backlight);
	if (ret) {
		pr_debug("%s: Warning: cannot get backlight pwm: ret = %d\n",
			__func__, ret);
		priv->bl_valid = false;
	} else {
		pr_debug("apply for pwm successfully\n");
		priv->bl_valid = true;
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0, &priv->enable,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get enable GPIO: ret=%d\n",
		      __func__, ret);
		priv->enable_valid = false;
	} else {
		priv->enable_valid = true;
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_debug("%s: Warning: cannot get reset GPIO: ret=%d\n",
		      __func__, ret);
		priv->reset_valid = false;
	} else {
		priv->reset_valid = true;
	}

	return 0;
}

U_BOOT_DRIVER(spacemit_panel) = {
        .name   = "spacemit-panel",
        .id     = UCLASS_NOP,
        .of_to_plat     = spacemit_panel_of_to_plat,
        .of_match = spacemit_panel_ids,
        .priv_auto      = sizeof(struct spacemit_panel_priv),
};
