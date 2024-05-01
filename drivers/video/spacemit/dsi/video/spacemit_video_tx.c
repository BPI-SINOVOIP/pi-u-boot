// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <stddef.h>
#include "../include/spacemit_video_tx.h"


int tx_device_num = 0;
struct video_tx_device *tx_devices[2] = {0};

struct video_tx_device *find_video_tx(void)
{
	struct video_tx_device *tx_device = NULL;
	int is_panel = 0;
	int i;

	for(i=0; i<tx_device_num; i++) {
		tx_device = tx_devices[i];
		if(!tx_device)
			continue;
		if(NULL == tx_device->driver->identify)
			continue;
		is_panel = tx_device->driver->identify(tx_device);
		if(is_panel){
			pr_debug("lcd (port %d) is opened by kernel!\n", tx_device->panel_type);
			return tx_device;
		} else {
			pr_info("lcd_port (%d) is not the corrected video_tx!\n", tx_device->panel_type);
		}
	}

	pr_info("Can not found the corrected panel!\n");
	return NULL;
}

int video_tx_get_modes(struct video_tx_device *video_tx,
		       struct spacemit_mode_modeinfo *modelist)
{
	if (!video_tx->driver->get_modes)
		return -EINVAL;

	return video_tx->driver->get_modes(video_tx, modelist);
}

/**
 * video_tx_dpms - set the power status of a video tx
 *
 * @video_tx: pointer to the video tx device
 * @mode: the power status we want to put the device into
 *	(follows the  DRM_MODE_DPMS definitions)
 *
 * This function will be called by a host in order to change the power
 * mode of a video tx client.
 *
 * It will return 0 on success and negative on error.
 *
 */
int video_tx_dpms(struct video_tx_device *video_tx, int mode)
{
	int ret;

	if (!video_tx->driver->dpms)
		return -EINVAL;

	ret = video_tx->driver->dpms(video_tx, mode);

	return ret;
}

void video_tx_esd_check(struct video_tx_device *video_tx)
{
	bool esd_status = false;
	int ret;

	if (!video_tx->driver->esd_check || !video_tx->driver->panel_reset) {
		pr_info("esd_check() not implemented\n");
		return;
	}

	esd_status = video_tx->driver->esd_check(video_tx);
	if(!esd_status) {
		ret = video_tx->driver->panel_reset(video_tx);
		if(ret) {
			pr_info("panel reset fail!\n");
		}
	}
}


/**
 * video_tx_register_device - register a video tx with the framework
 *
 * @dev: pointer to the video transmitter device structure
 * @driver: pointer to the driver structure that contains all the ops
 *
 * This function will register a video transmitter client with the video_tx
 * framework.
 * It will return a pointer to the newly allocated video_tx_device structure or
 * NULL in case of an error.
 *
 * The video tx driver needs to call this function before using any of the
 * other facilities of the framework.
 *
 */
int video_tx_register_device(struct video_tx_device *tx_device)
{
	if(tx_device_num >= 2) {
		pr_info("%s, video_tx_device is full!\n", __func__);
		return 0;
	}

	tx_devices[tx_device_num] = tx_device;
	tx_device_num++;

	pr_info("fb: video_tx (port %d) register device!\n", tx_device->panel_type);
	return 0;
}

void *video_tx_get_drvdata(struct video_tx_device *tx_device)
{
	return tx_device->private;
}
