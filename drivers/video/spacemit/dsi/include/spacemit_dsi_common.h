// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#ifndef _SPACEMIT_DSI_COMMON_H_
#define _SPACEMIT_DSI_COMMON_H_

#include <linux/types.h>
#include <common.h>
#include <stdio.h>

#define MAX_TX_CMD_COUNT 100
#define MAX_RX_DATA_COUNT 64

enum spacemit_mipi_burst_mode{
	DSI_BURST_MODE_NON_BURST_SYNC_PULSE = 0,
	DSI_BURST_MODE_NON_BURST_SYNC_EVENT = 1,
	DSI_BURST_MODE_BURST = 2,
	DSI_BURST_MODE_MAX
};

enum spacemit_mipi_input_data_mode{
	DSI_INPUT_DATA_RGB_MODE_565 = 0,
	DSI_INPUT_DATA_RGB_MODE_666PACKET = 1,
	DSI_INPUT_DATA_RGB_MODE_666UNPACKET = 2,
	DSI_INPUT_DATA_RGB_MODE_888 = 3,
	DSI_INPUT_DATA_RGB_MODE_MAX
};

enum spacemit_dsi_work_mode {
	SPACEMIT_DSI_MODE_VIDEO,
	SPACEMIT_DSI_MODE_CMD,
	SPACEMIT_DSI_MODE_MAX
};

enum spacemit_dsi_cmd_type {
	SPACEMIT_DSI_DCS_SWRITE = 0x5,
	SPACEMIT_DSI_DCS_SWRITE1 = 0x15,
	SPACEMIT_DSI_DCS_LWRITE = 0x39,
	SPACEMIT_DSI_DCS_READ = 0x6,
	SPACEMIT_DSI_GENERIC_LWRITE = 0x29,
	SPACEMIT_DSI_GENERIC_READ1 = 0x14,
	SPACEMIT_DSI_SET_MAX_PKT_SIZE = 0x37,
};

enum spacemit_dsi_tx_mode {
	SPACEMIT_DSI_HS_MODE = 0,
	SPACEMIT_DSI_LP_MODE = 1,
};

enum spacemit_dsi_rx_data_type {
	SPACEMIT_DSI_ACK_ERR_RESP = 0x2,
	SPACEMIT_DSI_EOTP = 0x8,
	SPACEMIT_DSI_GEN_READ1_RESP = 0x11,
	SPACEMIT_DSI_GEN_READ2_RESP = 0x12,
	SPACEMIT_DSI_GEN_LREAD_RESP = 0x1A,
	SPACEMIT_DSI_DCS_READ1_RESP = 0x21,
	SPACEMIT_DSI_DCS_READ2_RESP = 0x22,
	SPACEMIT_DSI_DCS_LREAD_RESP = 0x1C,
};

enum spacemit_dsi_polarity {
	SPACEMIT_DSI_POLARITY_POS = 0,
	SPACEMIT_DSI_POLARITY_NEG,
	SPACEMIT_DSI_POLARITY_MAX
};

enum spacemit_dsi_te_mode {
	SPACEMIT_DSI_TE_MODE_NO = 0,
	SPACEMIT_DSI_TE_MODE_A,
	SPACEMIT_DSI_TE_MODE_B,
	SPACEMIT_DSI_TE_MODE_C,
	SPACEMIT_DSI_TE_MODE_MAX,

};

struct spacemit_mipi_info {
	unsigned int height;
	unsigned int width;
	unsigned int hfp; /*pixel*/
	unsigned int hbp;
	unsigned int hsync;
	unsigned int vfp; /*line*/
	unsigned int vbp;
	unsigned int vsync;
	unsigned int fps;

	unsigned int work_mode; /*command_mode, video_mode*/
	unsigned int rgb_mode;
	unsigned int lane_number;
	unsigned int phy_freq;
	unsigned int split_enable;
	unsigned int eotp_enable;

	/*for video mode*/
	unsigned int burst_mode;

	/*for cmd mode*/
	unsigned int te_enable;
	unsigned int vsync_pol;
	unsigned int te_pol;
	unsigned int te_mode;

	/*The following fields need not be set by panel*/
	unsigned int real_fps;
};

struct spacemit_dsi_cmd_desc {
	enum spacemit_dsi_cmd_type cmd_type;
	uint8_t  lp;		/*command tx through low power mode or hs mode */
	uint32_t delay;	/* time to delay */
	uint32_t length;	/* cmds length */
	uint8_t data[MAX_TX_CMD_COUNT];
};

struct spacemit_dsi_rx_buf {
	enum spacemit_dsi_rx_data_type data_type;
	uint32_t length; /* cmds length */
	uint8_t data[MAX_RX_DATA_COUNT];
};

/*API for mipi panel*/
int spacemit_mipi_open(int id, struct spacemit_mipi_info *mipi_info, bool ready);
int spacemit_mipi_close(int id);
int spacemit_mipi_write_cmds(int id, struct spacemit_dsi_cmd_desc *cmds, int count);
int spacemit_mipi_read_cmds(int id, struct spacemit_dsi_rx_buf *dbuf,
							struct spacemit_dsi_cmd_desc *cmds, int count);
int spacemit_mipi_ready_for_datatx(int id, struct spacemit_mipi_info *mipi_info);
int spacemit_mipi_close_datatx(int id);

/*API for dsi driver*/
int spacemit_dsi_register_device(void *device);

int spacemit_dsi_probe(void);
int lcd_mipi_probe(void);

int lcd_icnl9911c_init(void);
int lcd_gx09inx101_init(void);

#endif /*_SPACEMIT_DSI_COMMON_H_*/
