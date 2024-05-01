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


#define PANEL_NUM_MAX	5

int panel_num = 0;
struct lcd_mipi_panel_info *panels[PANEL_NUM_MAX] = {0};
int lcd_id = 0;
int lcd_width = 0;
int lcd_height = 0;
char *lcd_name = NULL;

//extern unsigned int board_id;


static int set_bit_value(int value, int low_bit, int high_bit, int bits_val)
{
	int mask;

	mask = (1 << (high_bit - low_bit + 1)) - 1;
	mask = mask << low_bit;
	value &= ~mask;
	value |= (bits_val << low_bit);

	return value;
}

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
		if (ret)
			return false;
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
	dm_gpio_set_value(&priv->reset, 1);
	mdelay(10);
	dm_gpio_set_value(&priv->reset, 0);
	mdelay(10);
	dm_gpio_set_value(&priv->reset, 1);
	mdelay(120);

	return 0;
}

static int lcd_mipi_dc_enable(bool power_on, struct spacemit_panel_priv *priv)
{
	if(power_on){
		dm_gpio_set_value(&priv->dcp, 1);
		dm_gpio_set_value(&priv->dcn, 1);
	} else {
		dm_gpio_set_value(&priv->dcp, 0);
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
	ret = spacemit_mipi_ready_for_datatx(0, video_tx_client->panel_info->mipi_info);
	if (0 != ret) {
		pr_info("lcd_mipi spacemit_mipi_ready_for_datatx fail!\n ");
		spacemit_mipi_close(0);
	}

	return 0;
}

void dpc_update_clocks(struct lcd_mipi_panel_info *panel_info)
{
	unsigned int value = 0;
	unsigned int freq_sel = 0;
	unsigned int freq_div = 0;
	unsigned int timeout = 50;

	/* bitclk */

	freq_sel = panel_info->bitclk_sel;
	freq_div = panel_info->bitclk_div;
	value = readl((void *)(uintptr_t)0xd4282844);
	value = set_bit_value(value, 20, 21, freq_sel);
	value = set_bit_value(value, 17, 19, freq_div);
	writel(value, (void *)(uintptr_t)0xd4282844);
	value |= BIT(31);
	writel(value, (void *)(uintptr_t)0xd4282844);

	/* wait freq change successful */
	while (true) {
		value = readl((void *)(uintptr_t)0xd4282844);

		if ((value & BIT(31)) == 0)
			break;

		if (timeout == 0) {
			pr_info("failed to change dpu bitclk frequency\n");
			break;
		}

		timeout--;

		udelay(10);
	}

	/* pxclk */
	timeout = 50;
	freq_sel = panel_info->pxclk_sel;
	freq_div = panel_info->pxclk_div;
	value = readl((void *)(uintptr_t)0xd428284c);
	value = set_bit_value(value, 21, 23, freq_sel);
	value = set_bit_value(value, 17, 20, freq_div);
	writel(value, (void *)(uintptr_t)0xd428284c);

	value = readl((void *)(uintptr_t)0xd4282844);
	value |= BIT(30);
	writel(value,(void *)(uintptr_t) 0xd4282844);

	/* wait freq change successful */
	while (true) {
		value = readl((void *)(uintptr_t)0xd4282844);

		if ((value & BIT(30)) == 0)
			break;

		if (timeout == 0) {
			pr_info("failed to change dpu pxclk frequency\n");
			break;
		}

		timeout--;

		udelay(10);
	}

	pr_debug("dpu clk1 = 0x%x\n", readl((void *)(uintptr_t)0xd4282844));
	pr_debug("dpu clk2 = 0x%x\n", readl((void *)(uintptr_t)0xd428284c));
}
static int lcd_mipi_identify(struct video_tx_device *dev)
{
	struct lcd_mipi_tx_data  *video_tx_client =
				video_tx_get_drvdata(dev);
	struct lcd_mipi_panel_info *panel_info = NULL;
	bool is_panel = false;
	int ret = 0;
	int i, num;

	ret = lcd_mipi_dc_enable(true, video_tx_client->priv);
	if (ret) {
		pr_info("lcd_mipi gpio dc failded!\n");
	}

	for(i=0; i<panel_num; i++) {
		panel_info = panels[i];
		if(!panel_info)
			continue;

		dpc_update_clocks(panel_info);

		pr_debug("now check lcd (%s)\n",panel_info->lcd_name);

		video_tx_client->panel_info = panel_info;

		for (num = 0; num < 1; num++) {
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

			if (is_panel)
				break;
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

	ret = spacemit_mipi_write_cmds(0, video_tx_client->panel_info->init_cmds,
			video_tx_client->panel_info->init_cmds_num);

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

	ret = spacemit_mipi_write_cmds(0, video_tx_client->panel_info->sleep_out_cmds,
			video_tx_client->panel_info->sleep_out_cmds_num);

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

	dm_gpio_set_value(&priv->bl, 1);

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

	lcd_icnl9911c_init();
	lcd_gx09inx101_init();


	return 0;
}

static const struct udevice_id spacemit_panel_ids[] = {
        { .compatible = "spacemit,panel" },
        { }
};

static int spacemit_panel_of_to_plat(struct udevice *dev)
{

	struct spacemit_panel_priv *priv = dev_get_priv(dev);
	int ret;

	ret = gpio_request_by_name(dev, "dcp-gpios", 0, &priv->dcp,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_info("%s: Warning: cannot get dcp GPIO: ret=%d\n",
		      __func__, ret);
	}

	ret = gpio_request_by_name(dev, "dcn-gpios", 0, &priv->dcn,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_info("%s: Warning: cannot get dcn GPIO: ret=%d\n",
		      __func__, ret);
	}

	ret = gpio_request_by_name(dev, "bl-gpios", 0, &priv->bl,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_info("%s: Warning: cannot get bl GPIO: ret=%d\n",
		      __func__, ret);
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_info("%s: Warning: cannot get reset GPIO: ret=%d\n",
		      __func__, ret);
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
