// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include "spacemit_dsi_drv.h"
#include "../include/spacemit_dsi_common.h"

#define MAX_DSI_NUM 1

struct spacemit_dsi_device *g_spacemit_dsi_list[MAX_DSI_NUM];

/*API for panel*/
int spacemit_mipi_open(int id, struct spacemit_mipi_info *mipi_info, bool ready)
{
	if(id >= MAX_DSI_NUM) {
		pr_info("%s: Invalid param (%d)\n", __func__, id);
		return -1;
	}

	if(NULL == g_spacemit_dsi_list[id]){
		pr_info("%s: dsi (%d) has not been registed\n", __func__, id);
		return -1;
	}

	return g_spacemit_dsi_list[id]->driver_ctx->dsi_open(g_spacemit_dsi_list[id], mipi_info, ready);
}

int spacemit_mipi_close(int id)
{
	if(id >= MAX_DSI_NUM) {
		pr_info("%s: Invalid param (%d)\n", __func__, id);
		return -1;
	}

	if(NULL == g_spacemit_dsi_list[id]){
		pr_info("%s: dsi (%d) has not been registed\n", __func__, id);
		return -1;
	}

	return g_spacemit_dsi_list[id]->driver_ctx->dsi_close(g_spacemit_dsi_list[id]);
}

int spacemit_mipi_write_cmds(int id, struct spacemit_dsi_cmd_desc *cmds, int count)
{
	if(id >= MAX_DSI_NUM) {
		pr_info("%s: Invalid param (%d)\n", __func__, id);
		return -1;
	}

	if(NULL == g_spacemit_dsi_list[id]){
		pr_info("%s: dsi (%d) has not been registed\n", __func__, id);
		return -1;
	}

	return g_spacemit_dsi_list[id]->driver_ctx->dsi_write_cmds(g_spacemit_dsi_list[id], cmds, count);
}

int spacemit_mipi_read_cmds(int id, struct spacemit_dsi_rx_buf *dbuf,
							struct spacemit_dsi_cmd_desc *cmds, int count)
{
	if(id >= MAX_DSI_NUM) {
		pr_info("%s: Invalid param (%d)\n", __func__, id);
		return -1;
	}

	if(NULL == g_spacemit_dsi_list[id]){
		pr_info("%s: dsi (%d) has not been registed\n", __func__, id);
		return -1;
	}

	return g_spacemit_dsi_list[id]->driver_ctx->dsi_read_cmds(g_spacemit_dsi_list[id], dbuf, cmds, count);
}

int spacemit_mipi_ready_for_datatx(int id, struct spacemit_mipi_info *mipi_info)
{
	if(id >= MAX_DSI_NUM) {
		pr_info("%s: Invalid param (%d)\n", __func__, id);
		return -1;
	}

	if(NULL == g_spacemit_dsi_list[id]){
		pr_info("%s: dsi (%d) has not been registed\n", __func__, id);
		return -1;
	}

	return g_spacemit_dsi_list[id]->driver_ctx->dsi_ready_for_datatx(g_spacemit_dsi_list[id], mipi_info);
}

int spacemit_mipi_close_datatx(int id)
{
	if(id >= MAX_DSI_NUM) {
		pr_info("%s: Invalid param (%d)\n", __func__, id);
		return -1;
	}

	if(NULL == g_spacemit_dsi_list[id]){
		pr_info("%s: dsi (%d) has not been registed\n", __func__, id);
		return -1;
	}

	return g_spacemit_dsi_list[id]->driver_ctx->dsi_close_datatx(g_spacemit_dsi_list[id]);
}

/*API for dsi driver*/
int spacemit_dsi_register_device(void *device)
{
	struct spacemit_dsi_device *dsi_device = (struct spacemit_dsi_device*)device;

	if(NULL == dsi_device){
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	if(dsi_device->id >= MAX_DSI_NUM){
		pr_info("%s: error id(%d)!\n", __func__, dsi_device->id);
		return -1;
	}

	if(dsi_device->driver_ctx == NULL){
		pr_info("%s: error driver_ctx!\n", __func__);
		return -1;
	}

	if(g_spacemit_dsi_list[dsi_device->id] != NULL){
		pr_info("%s: %d id has been registed!\n",  __func__, dsi_device->id);
		return -1;
	}

	g_spacemit_dsi_list[dsi_device->id] = dsi_device;

	return 0;
}
