// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_DSI_H_
#define _SPACEMIT_DSI_H_

#include <string.h>
#include "../include/spacemit_dsi_common.h"
#include "spacemit_dphy.h"

enum spacemit_dsi_event_id {
	SPACEMIT_DSI_EVENT_ERROR,
	SPACEMIT_DSI_EVENT_MAX,
};

enum spacemit_dsi_status {
	DSI_STATUS_UNINIT = 0,
	DSI_STATUS_OPENED = 1,
	DSI_STATUS_INIT = 2,
	DSI_STATUS_MAX
};

struct spacemit_dsi_device;

struct spacemit_dsi_driver_ctx {
	int (*dsi_open)(struct spacemit_dsi_device* device_ctx, struct spacemit_mipi_info *mipi_info, bool ready);
	int (*dsi_close)(struct spacemit_dsi_device* device_ctx);
	int (*dsi_write_cmds)(struct spacemit_dsi_device* device_ctx, struct spacemit_dsi_cmd_desc *cmds, int count);
	int (*dsi_read_cmds)(struct spacemit_dsi_device* device_ctx, struct spacemit_dsi_rx_buf *dbuf,
								struct spacemit_dsi_cmd_desc *cmds, int count);
	int (*dsi_ready_for_datatx)(struct spacemit_dsi_device* device_ctx, struct spacemit_mipi_info *mipi_info);
	int (*dsi_close_datatx)(struct spacemit_dsi_device* device_ctx);
};

struct spacemit_dsi_advanced_setting {
	uint32_t lpm_frame_en; /*return to LP mode every frame*/
	uint32_t last_line_turn;
	uint32_t hex_slot_en;
	uint32_t hsa_pkt_en;
	uint32_t hse_pkt_en;
	uint32_t hbp_pkt_en; /*bit:18*/
	uint32_t hfp_pkt_en; /*bit:20*/
	uint32_t hex_pkt_en;
	uint32_t hlp_pkt_en; /*bit:22*/
	uint32_t auto_dly_dis;
	uint32_t timing_check_dis;
	uint32_t hact_wc_en;
	uint32_t auto_wc_dis;
	uint32_t vsync_rst_en;
};

struct spacemit_dsi_device {
	uint32_t id; /*dsi id*/

	unsigned long esc_clk_rate, bit_clk_rate;

	struct spacemit_dsi_driver_ctx *driver_ctx;

	struct spacemit_dphy_ctx dphy_config;
	struct spacemit_dsi_advanced_setting adv_setting;
	int status;
};

#endif /*_SPACEMIT_DSI_H_*/
