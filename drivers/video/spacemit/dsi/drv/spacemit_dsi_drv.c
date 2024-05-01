// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <linux/delay.h>
#include <common.h>

#include "../include/spacemit_dsi_common.h"
#include "spacemit_dsi_drv.h"
#include "spacemit_dsi_hw.h"
#include "spacemit_dphy.h"

#define SPACEMIT_DSI_NAME "spacemit_dsi"
#define SPACEMIT_DSI_VERSION	"1.0.0"

#define SPACEMIT_ESC_CLK_DEFAULT 52000000
#define MIPI_CLK_MIN 800000000

#define SPACEMIT_DSI_MAX_TX_FIFO_BYTES	256
#define SPACEMIT_DSI_MAX_RX_FIFO_BYTES	64

#define SPACEMIT_DSI_MAX_CMD_FIFO_BYTES	1024

#define LPM_FRAME_EN_DEFAULT 0
#define LAST_LINE_TURN_DEFAULT 0
#define HEX_SLOT_EN_DEFAULT 0
#define HSA_PKT_EN_DEFAULT_SYNC_PULSE 1
#define HSA_PKT_EN_DEFAULT_OTHER 0
#define HSE_PKT_EN_DEFAULT_SYNC_PULSE 1
#define HSE_PKT_EN_DEFAULT_OTHER 0
#define HBP_PKT_EN_DEFAULT 1
//#define HFP_PKT_EN_DEFAULT 1
#define HFP_PKT_EN_DEFAULT 0
#define HEX_PKT_EN_DEFAULT 0
#define HLP_PKT_EN_DEFAULT 0
#define AUTO_DLY_DIS_DEFAULT 0
#define TIMING_CHECK_DIS_DEFAULT 0
#define HACT_WC_EN_DEFAULT 1
#define AUTO_WC_DIS_DEFAULT 0
#define VSYNC_RST_EN_DEFAULT 1

#define HS_PREP_CONSTANT_DEFAULT 40
#define HS_PREP_UI_DEFAULT 4
#define HS_ZERO_CONSTANT_DEFAULT 145
#define HS_ZERO_UI_DEFAULT 10
#define HS_TRAIL_CONSTANT_DEFAULT 60
#define HS_TRAIL_UI_DEFAULT 4
#define HS_EXIT_CONSTANT_DEFAULT 100
#define HS_EXIT_UI_DEFAULT 0
#define CK_ZERO_CONSTANT_DEFAULT 300
#define CK_ZERO_UI_DEFAULT 0
#define CK_TRAIL_CONSTANT_DEFAULT 60
#define CK_TRAIL_UI_DEFAULT 0
#define REQ_READY_DEFAULT 0x3C
#define WAKEUP_CONSTANT_DEFAULT 1000000
#define WAKEUP_UI_DEFAULT 0
#define LPX_CONSTANT_DEFAULT 60
#define LPX_UI_DEFAULT 0
#define PLL5_VCO_DEF 1540000000
#define CLK_CALCU_DIFF 24
#define PXCLK_PLL5_DIV 3


#define to_dsi_bcnt(timing, bpp)        (((timing) * (bpp)) >> 3)

static unsigned int spacemit_dsi_lane[5] = {0, 0x1, 0x3, 0x7, 0xf};

static unsigned char dsi_bit(unsigned int index, unsigned char *pdata)
{
	unsigned char ret;
	unsigned int cindex, bindex;
	cindex = index / 8;
	bindex = index % 8;

	if (pdata[cindex] & (0x1 << bindex))
		ret = (unsigned char) 0x1;
	else
		ret = (unsigned char) 0x0;

	return ret;
}

static unsigned char calculate_ecc(unsigned char *pdata)
{
	unsigned char ret;
	unsigned char p[8];

	p[7] = (unsigned char) 0x0;
	p[6] = (unsigned char) 0x0;

	p[5] = (unsigned char) (
	(
		dsi_bit(10, pdata) ^
		dsi_bit(11, pdata) ^
		dsi_bit(12, pdata) ^
		dsi_bit(13, pdata) ^
		dsi_bit(14, pdata) ^
		dsi_bit(15, pdata) ^
		dsi_bit(16, pdata) ^
		dsi_bit(17, pdata) ^
		dsi_bit(18, pdata) ^
		dsi_bit(19, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(22, pdata) ^
		dsi_bit(23, pdata)
		)
	);
	p[4] = (unsigned char) (
		dsi_bit(4, pdata) ^
		dsi_bit(5, pdata) ^
		dsi_bit(6, pdata) ^
		dsi_bit(7, pdata) ^
		dsi_bit(8, pdata) ^
		dsi_bit(9, pdata) ^
		dsi_bit(16, pdata) ^
		dsi_bit(17, pdata) ^
		dsi_bit(18, pdata) ^
		dsi_bit(19, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(22, pdata) ^
		dsi_bit(23, pdata)
	);
	p[3] = (unsigned char) (
	(
		dsi_bit(1, pdata) ^
		dsi_bit(2, pdata) ^
		dsi_bit(3, pdata) ^
		dsi_bit(7, pdata) ^
		dsi_bit(8, pdata) ^
		dsi_bit(9, pdata) ^
		dsi_bit(13, pdata) ^
		dsi_bit(14, pdata) ^
		dsi_bit(15, pdata) ^
		dsi_bit(19, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(23, pdata)
		)
	);
	p[2] = (unsigned char) (
	(
		dsi_bit(0, pdata) ^
		dsi_bit(2, pdata) ^
		dsi_bit(3, pdata) ^
		dsi_bit(5, pdata) ^
		dsi_bit(6, pdata) ^
		dsi_bit(9, pdata) ^
		dsi_bit(11, pdata) ^
		dsi_bit(12, pdata) ^
		dsi_bit(15, pdata) ^
		dsi_bit(18, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(22, pdata)
		)
	);
	p[1] = (unsigned char) (
		(
		dsi_bit(0, pdata) ^
		dsi_bit(1, pdata) ^
		dsi_bit(3, pdata) ^
		dsi_bit(4, pdata) ^
		dsi_bit(6, pdata) ^
		dsi_bit(8, pdata) ^
		dsi_bit(10, pdata) ^
		dsi_bit(12, pdata) ^
		dsi_bit(14, pdata) ^
		dsi_bit(17, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(22, pdata) ^
		dsi_bit(23, pdata)
		)
	);
	p[0] = (unsigned char) (
		(
		dsi_bit(0, pdata) ^
		dsi_bit(1, pdata) ^
		dsi_bit(2, pdata) ^
		dsi_bit(4, pdata) ^
		dsi_bit(5, pdata) ^
		dsi_bit(7, pdata) ^
		dsi_bit(10, pdata) ^
		dsi_bit(11, pdata) ^
		dsi_bit(13, pdata) ^
		dsi_bit(16, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(22, pdata) ^
		dsi_bit(23, pdata)
		)
	);
	ret = (unsigned char)(
		p[0] |
		(p[1] << 0x1) |
		(p[2] << 0x2) |
		(p[3] << 0x3) |
		(p[4] << 0x4) |
		(p[5] << 0x5)
	);
	return   ret;
}

static unsigned short gs_crc16_generation_code = 0x8408;
static unsigned short calculate_crc16(unsigned char *pdata, unsigned
		short count)
{
	unsigned short byte_counter;
	unsigned char bit_counter;
	unsigned char data;
	unsigned short crc16_result = 0xFFFF;
	if (count > 0) {
		for (byte_counter = 0; byte_counter < count;
			byte_counter++) {
			data = *(pdata + byte_counter);
			for (bit_counter = 0; bit_counter < 8; bit_counter++) {
				if (((crc16_result & 0x0001) ^ ((0x0001 *
					data) & 0x0001)) > 0)
					crc16_result = ((crc16_result >> 1)
					& 0x7FFF) ^ gs_crc16_generation_code;
				else
					crc16_result = (crc16_result >> 1)
					& 0x7FFF;
				data = (data >> 1) & 0x7F;
			}
		}
	}
	return crc16_result;
}

static void dsi_get_advanced_setting(struct spacemit_dsi_device *dev, struct spacemit_mipi_info *mipi_info)
{
	struct spacemit_dsi_advanced_setting *adv_setting = &dev->adv_setting;

	adv_setting->lpm_frame_en = LPM_FRAME_EN_DEFAULT;
	adv_setting->last_line_turn = LAST_LINE_TURN_DEFAULT;
	adv_setting->hex_slot_en = HEX_SLOT_EN_DEFAULT;

	if(mipi_info->burst_mode == DSI_BURST_MODE_NON_BURST_SYNC_PULSE)
		adv_setting->hsa_pkt_en = HSA_PKT_EN_DEFAULT_SYNC_PULSE;
	else
		adv_setting->hsa_pkt_en = HSA_PKT_EN_DEFAULT_OTHER;

	if(mipi_info->burst_mode == DSI_BURST_MODE_NON_BURST_SYNC_PULSE)
		adv_setting->hse_pkt_en = HSE_PKT_EN_DEFAULT_SYNC_PULSE;
	else
		adv_setting->hse_pkt_en = HSE_PKT_EN_DEFAULT_OTHER;

	adv_setting->hbp_pkt_en = HBP_PKT_EN_DEFAULT;
	adv_setting->hfp_pkt_en = HFP_PKT_EN_DEFAULT;
	adv_setting->hex_pkt_en = HEX_PKT_EN_DEFAULT;
	adv_setting->hlp_pkt_en = HLP_PKT_EN_DEFAULT;
	adv_setting->auto_dly_dis = AUTO_DLY_DIS_DEFAULT;
	adv_setting->timing_check_dis = TIMING_CHECK_DIS_DEFAULT;
	adv_setting->hact_wc_en = HACT_WC_EN_DEFAULT;
	adv_setting->auto_wc_dis = AUTO_WC_DIS_DEFAULT;
	adv_setting->vsync_rst_en = VSYNC_RST_EN_DEFAULT;
}

static void dsi_get_dphy_setting(struct spacemit_dsi_device *dev)
{
	struct spacemit_dphy_timing *dphy_timing;

	if(NULL == dev){
		pr_info("%s: Invalid param\n",__func__);
		return;
	}

	dphy_timing = &dev->dphy_config.dphy_timing;

	dphy_timing->hs_prep_constant = HS_PREP_CONSTANT_DEFAULT;
	dphy_timing->hs_prep_ui = HS_PREP_UI_DEFAULT;
	dphy_timing->hs_zero_constant = HS_ZERO_CONSTANT_DEFAULT;
	dphy_timing->hs_zero_ui = HS_ZERO_UI_DEFAULT;
	dphy_timing->hs_trail_constant = HS_TRAIL_CONSTANT_DEFAULT;
	dphy_timing->hs_trail_ui = HS_TRAIL_UI_DEFAULT;
	dphy_timing->hs_exit_constant = HS_EXIT_CONSTANT_DEFAULT;
	dphy_timing->hs_exit_ui = HS_EXIT_UI_DEFAULT;
	dphy_timing->ck_zero_constant = CK_ZERO_CONSTANT_DEFAULT;
	dphy_timing->ck_zero_ui = CK_ZERO_UI_DEFAULT;
	dphy_timing->ck_trail_constant = CK_TRAIL_CONSTANT_DEFAULT;
	dphy_timing->ck_zero_ui = CK_TRAIL_UI_DEFAULT;
	dphy_timing->req_ready = REQ_READY_DEFAULT;
	dphy_timing->wakeup_constant = WAKEUP_CONSTANT_DEFAULT;
	dphy_timing->wakeup_ui = WAKEUP_UI_DEFAULT;
	dphy_timing->lpx_constant = LPX_CONSTANT_DEFAULT;
	dphy_timing->lpx_ui = LPX_UI_DEFAULT;
}

static void dsi_reset(void)
{
	uint32_t reg;

	reg = CFG_SOFT_RST | CFG_SOFT_RST_REG | CFG_CLR_PHY_FIFO | CFG_RST_TXLP |
		CFG_RST_CPU | CFG_RST_CPN | CFG_RST_VPN | CFG_DSI_PHY_RST;

	/* software reset DSI module */
	dsi_write(DSI_CTRL_0, reg);
	/* Note: there need some delay after set CFG_SOFT_RST */
	udelay(100);
	dsi_write(DSI_CTRL_0, 0);
}

static void dsi_enable_video_mode(bool enable)
{
	if(enable)
		dsi_set_bits(DSI_CTRL_0, CFG_VPN_TX_EN | CFG_VPN_SLV | CFG_VPN_EN);
	else
		dsi_clear_bits(DSI_CTRL_0, CFG_VPN_TX_EN | CFG_VPN_EN);
}

static void dsi_enable_cmd_mode(bool enable)
{
	if(enable)
		dsi_set_bits(DSI_CTRL_0, CFG_CPN_EN);
	else
		dsi_clear_bits(DSI_CTRL_0, CFG_CPN_EN);
}

static void dsi_enable_eotp(bool enable)
{
	if(enable)
		dsi_set_bits(DSI_CTRL_1, CFG_EOTP_EN);
	else
		dsi_clear_bits(DSI_CTRL_1, CFG_EOTP_EN);
}

static void dsi_enable_lptx_lanes(uint32_t lane_num)
{
	dsi_write_bits(DSI_CPU_CMD_1,
		CFG_TXLP_LPDT_MASK, lane_num << CFG_TXLP_LPDT_SHIFT);
}

static void dsi_enable_split_mode(bool splite_mode)
{
	if(splite_mode){
		dsi_set_bits(DSI_LCD_BDG_CTRL0, CFG_SPLIT_EN);
	} else {
		dsi_clear_bits(DSI_LCD_BDG_CTRL0, CFG_SPLIT_EN);
	}
}

static int dsi_write_cmd(uint8_t *parameter, uint8_t count, bool lp)
{
	uint32_t send_data = 0, reg, timeout, tmp, i;
	bool turnaround;
	uint32_t len;

	/* write all packet bytes to packet data buffer */
	for (i = 0; i < count; i++) {
		send_data |= parameter[i] << ((i % 4) * 8);
		if (0 ==((i + 1) % 4)) {
			dsi_write(DSI_CPU_WDAT, send_data);
			reg = CFG_CPU_DAT_REQ |	CFG_CPU_DAT_RW |((i - 3) << CFG_CPU_DAT_ADDR_SHIFT);
			dsi_write(DSI_CPU_CMD_3, reg);
			/* wait write operation done */
			timeout = 1000;
			do {
				timeout--;
				tmp = dsi_read(DSI_CPU_CMD_3);
			} while ((tmp & CFG_CPU_DAT_REQ) && timeout);
			if (timeout == 0)
				pr_info("DSI write data to the packet data buffer not done.\n");
			send_data = 0;
		}
	}

	/* handle last none 4Byte align data */
	if (0 != i % 4) {
		dsi_write(DSI_CPU_WDAT, send_data);
		reg = CFG_CPU_DAT_REQ | CFG_CPU_DAT_RW |((4 * (i / 4)) << CFG_CPU_DAT_ADDR_SHIFT);
		dsi_write(DSI_CPU_CMD_3, reg);
		/* wait write operation done */
		timeout = 1000;
		do {
			timeout--;
			tmp = dsi_read(DSI_CPU_CMD_3);
		} while ((tmp & CFG_CPU_DAT_REQ) && timeout);
		if (timeout == 0)
			pr_info("DSI write data to the packet data buffer not done.\n");
		send_data = 0;
	}

	if (parameter[0] == SPACEMIT_DSI_DCS_READ ||
		parameter[0] == SPACEMIT_DSI_GENERIC_READ1)
		turnaround = true;
	else
		turnaround = false;

	len = count;
#if 0
	/* The packet length should contain  CRC_bytes_length in Aquilac_DSI version */
	if ((parameter[0] == SPACEMIT_DSI_DCS_LWRITE ||
		parameter[0] == SPACEMIT_DSI_GENERIC_LWRITE) && !lp)
		len = count - 6;
#endif
	reg = CFG_CPU_CMD_REQ |
		((count == 4) ? CFG_CPU_SP : 0) |
		(turnaround ? CFG_CPU_TURN : 0) |
		(lp ? CFG_CPU_TXLP : 0) |
		(len << CFG_CPU_WC_SHIFT);

	/* send out the packet */
	dsi_write(DSI_CPU_CMD_0, reg);
	/* wait packet be sent out */
	timeout = 1000;
	do {
		timeout--;
		tmp = dsi_read(DSI_CPU_CMD_0);
		udelay(20);
	} while ((tmp & CFG_CPU_CMD_REQ) && timeout);
	if (0 == timeout) {
		pr_info("%s: DSI send out packet maybe failed.\n", __func__);
		return -1;
	}

	return 0;
}

static void dsi_config_video_mode(struct spacemit_dsi_device *dsi_ctx, struct spacemit_mipi_info *mipi_info)
{
	uint32_t hsync_b, hbp_b, hact_b, hex_b, hfp_b, httl_b;
	uint32_t hsync, hbp, hact, httl, v_total;
	uint32_t hsa_wc, hbp_wc, hact_wc, hex_wc, hfp_wc, hlp_wc;
	uint32_t bpp, hss_bcnt = 4, hse_bct = 4, lgp_over_head = 6, reg;
	uint32_t slot_cnt0, slot_cnt1;
	uint32_t dsi_ex_pixel_cnt = 0;
	uint32_t dsi_hex_en = 0;
	uint32_t width, lane_number;
	struct spacemit_dsi_advanced_setting *adv_setting = &dsi_ctx->adv_setting;

	switch(mipi_info->rgb_mode){
	case DSI_INPUT_DATA_RGB_MODE_565:
		bpp = 16;
		break;
	case DSI_INPUT_DATA_RGB_MODE_666PACKET:
		bpp = 18;
		break;
	case DSI_INPUT_DATA_RGB_MODE_666UNPACKET:
		bpp = 18;
		break;
	case DSI_INPUT_DATA_RGB_MODE_888:
		bpp = 24;
		break;
	default:
		bpp = 24;
	}

	v_total = mipi_info->height + mipi_info->vfp + mipi_info->vbp + mipi_info->vsync;

	if(mipi_info->split_enable) {
		if(( 0 != (mipi_info->width & 0x1)) || (0 != (mipi_info->lane_number & 0x1))){
			pr_info("%s: warning:Invalid split config(lane = %d, width = %d)\n",
				__func__, mipi_info->lane_number, mipi_info->width);
		}
		width = mipi_info->width >> 1;
		lane_number = mipi_info->lane_number >> 1;
	} else {
		width = mipi_info->width;
		lane_number = mipi_info->lane_number;
	}

	hact_b = to_dsi_bcnt(width, bpp);
	hfp_b = to_dsi_bcnt(mipi_info->hfp, bpp);
	hbp_b = to_dsi_bcnt(mipi_info->hbp, bpp);
	hsync_b = to_dsi_bcnt(mipi_info->hsync, bpp);
	hex_b = to_dsi_bcnt(dsi_ex_pixel_cnt, bpp);
	httl_b = hact_b + hsync_b + hfp_b + hbp_b + hex_b;
	slot_cnt0 = (httl_b - hact_b) / lane_number + 3;
	slot_cnt1 = slot_cnt0;

	hact = hact_b / lane_number;
	hbp = hbp_b / lane_number;
	hsync = hsync_b / lane_number;
	httl = (hact_b + hfp_b + hbp_b + hsync_b) / lane_number;

	/* word count in the unit of byte */
	hsa_wc = (mipi_info->burst_mode == DSI_BURST_MODE_NON_BURST_SYNC_PULSE) ?
		(hsync_b - hss_bcnt - lgp_over_head) : 0;

	/* Hse is with backporch */
	hbp_wc = (mipi_info->burst_mode == DSI_BURST_MODE_NON_BURST_SYNC_PULSE) ?
		(hbp_b - hse_bct - lgp_over_head)
		: (hsync_b + hbp_b - hss_bcnt - lgp_over_head);

	hfp_wc = ((mipi_info->burst_mode == DSI_BURST_MODE_BURST) && (dsi_hex_en == 0)) ?
		(hfp_b + hex_b - lgp_over_head - lgp_over_head) :
		(hfp_b - lgp_over_head - lgp_over_head);

	hact_wc =  (width * bpp) >> 3;

	/* disable Hex currently */
	hex_wc = 0;

	/*  There is no hlp with active data segment.  */
	hlp_wc = (mipi_info->burst_mode == DSI_BURST_MODE_NON_BURST_SYNC_PULSE) ?
		(httl_b - hsync_b - hse_bct - lgp_over_head) :
		(httl_b - hss_bcnt - lgp_over_head);

	/* FIXME - need to double check the (*3) is bytes_per_pixel
	* from input data or output to panel
	*/

	/*Jessica: need be calculated by really case*/
	dsi_write(DSI_VPN_CTRL_0, (0x50<<16) | 0xc08);

	/* SET UP LCD1 TIMING REGISTERS FOR DSI BUS */
	dsi_write(DSI_VPN_TIMING_0, (hact << 16) | httl);
	dsi_write(DSI_VPN_TIMING_1, (hsync << 16) | hbp);
	dsi_write(DSI_VPN_TIMING_2, ((mipi_info->height)<<16) | (v_total));
	dsi_write(DSI_VPN_TIMING_3, ((mipi_info->vsync) << 16) | (mipi_info->vbp));

	/* SET UP LCD1 WORD COUNT REGISTERS FOR DSI BUS */
	dsi_write(DSI_VPN_WC_0, (hbp_wc << 16) | hsa_wc);
	dsi_write(DSI_VPN_WC_1, (hfp_wc << 16) | hact_wc);
	dsi_write(DSI_VPN_WC_2, (hex_wc << 16) | hlp_wc);

	dsi_write(DSI_VPN_SLOT_CNT_0, (slot_cnt0 << 16) | slot_cnt0);
	dsi_write(DSI_VPN_SLOT_CNT_1, (slot_cnt1 << 16) | slot_cnt1);

	/* Configure LCD control register 1 FOR DSI BUS */
	reg = adv_setting->vsync_rst_en << CFG_VPN_VSYNC_RST_EN_SHIFT |
			adv_setting->auto_wc_dis << CFG_VPN_AUTO_WC_DIS_SHIFT |
			adv_setting->hact_wc_en << CFG_VPN_HACT_WC_EN_SHIFT |
			adv_setting->timing_check_dis << CFG_VPN_TIMING_CHECK_DIS_SHIFT |
			adv_setting->auto_dly_dis << CFG_VPN_AUTO_DLY_DIS_SHIFT |
			adv_setting->hlp_pkt_en << CFG_VPN_HLP_PKT_EN_SHIFT |
			adv_setting->hex_pkt_en << CFG_VPN_HEX_PKT_EN_SHIFT |
			adv_setting->hfp_pkt_en << CFG_VPN_HFP_PKT_EN_SHIFT |
			adv_setting->hbp_pkt_en << CFG_VPN_HBP_PKT_EN_SHIFT |
			adv_setting->hse_pkt_en << CFG_VPN_HSE_PKT_EN_SHIFT |
			adv_setting->hsa_pkt_en << CFG_VPN_HSA_PKT_EN_SHIFT |
			adv_setting->hex_slot_en<< CFG_VPN_HEX_SLOT_EN_SHIFT |
			adv_setting->last_line_turn << CFG_VPN_LAST_LINE_TURN_SHIFT |
			adv_setting->lpm_frame_en << CFG_VPN_LPM_FRAME_EN_SHIFT |
			mipi_info->burst_mode << CFG_VPN_BURST_MODE_SHIFT |
			mipi_info->rgb_mode << CFG_VPN_RGB_TYPE_SHIFT;
	dsi_write(DSI_VPN_CTRL_1,reg);

	dsi_write_bits(DSI_LCD_BDG_CTRL0, CFG_VPN_FIFO_AFULL_CNT_MASK,
		0 << CFG_VPN_FIFO_AFULL_CNT_SHIT);
	dsi_set_bits(DSI_LCD_BDG_CTRL0, CFG_VPN_FIFO_AFULL_BYPASS);
	dsi_set_bits(DSI_LCD_BDG_CTRL0, CFG_PIXEL_SWAP);

	dsi_enable_cmd_mode(false);
	dsi_enable_video_mode(true);
}

static void dsi_config_cmd_mode(struct spacemit_dsi_device *dsi_ctx, struct spacemit_mipi_info *mipi_info)
{
	int reg;
	int rgb_mode, bpp;

	switch(mipi_info -> rgb_mode){
	case DSI_INPUT_DATA_RGB_MODE_565:
		bpp = 16;
		rgb_mode = 2;
		break;
	case DSI_INPUT_DATA_RGB_MODE_666UNPACKET:
		bpp = 18;
		rgb_mode = 1;
		break;
	case DSI_INPUT_DATA_RGB_MODE_888:
		bpp = 24;
		rgb_mode = 0;
		break;
	default:
		pr_info("%s: unsupported rgb format!\n", __func__);
		bpp = 24;
		rgb_mode = 0;
	}

	reg = mipi_info->te_enable << CFG_CPN_TE_EN_SHIFT |
			rgb_mode << CFG_CPN_RGB_TYPE_SHIFT |
			1 << CFG_CPN_BURST_MODE_SHIFT |
			0 << CFG_CPN_DMA_DIS_SHIFT |
			0 << CFG_CPN_ADDR0_EN_SHIFT;
	dsi_write(DSI_CPN_CMD, reg);

	reg = mipi_info->width * bpp / 8 << CFG_CPN_PKT_CNT_SHIFT |
		SPACEMIT_DSI_MAX_CMD_FIFO_BYTES << CFG_CPN_FIFO_FULL_LEVEL_SHIFT;
	dsi_write(DSI_CPN_CTRL_1,reg);

	dsi_write_bits(DSI_LCD_BDG_CTRL0, CFG_CPN_TE_EDGE_MASK,
		mipi_info->te_pol << CFG_CPN_TE_EDGE_SHIFT);
	dsi_write_bits(DSI_LCD_BDG_CTRL0, CFG_CPN_VSYNC_EDGE_MASK,
		mipi_info->vsync_pol << CFG_CPN_VSYNC_EDGE_SHIFT);
	dsi_write_bits(DSI_LCD_BDG_CTRL0, CFG_CPN_TE_MODE_MASK,
		mipi_info->te_mode << CFG_CPN_TE_MODE_SHIFT);

	reg = 0x80 << CFG_CPN_TE_DLY_CNT_SHIFT |
			0 << CFG_CPN_TE_LINE_CNT_SHIFT;
	dsi_write(DSI_LCD_BDG_CTRL1, reg);

	dsi_enable_video_mode(false);
	dsi_enable_cmd_mode(true);
}

static int dsi_write_cmd_array(struct spacemit_dsi_device *dsi_ctx,
									struct spacemit_dsi_cmd_desc *cmds,int count)
{
	struct spacemit_dsi_cmd_desc cmd_line;
	uint8_t type, parameter[SPACEMIT_DSI_MAX_TX_FIFO_BYTES], len;
	uint32_t crc, loop;
	int ret = 0;

	if(NULL == dsi_ctx) {
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	for (loop = 0; loop < count; loop++) {
		cmd_line = cmds[loop];
		type = cmd_line.cmd_type;
		len = cmd_line.length;
		memset(parameter, 0x00, len + 6);
		parameter[0] = type & 0xff;
		switch (type) {
		case SPACEMIT_DSI_DCS_SWRITE:
		case SPACEMIT_DSI_DCS_SWRITE1:
		case SPACEMIT_DSI_DCS_READ:
		case SPACEMIT_DSI_GENERIC_READ1:
		case SPACEMIT_DSI_SET_MAX_PKT_SIZE:
			memcpy(&parameter[1], cmd_line.data, len);
			len = 4;
			break;
		case SPACEMIT_DSI_GENERIC_LWRITE:
		case SPACEMIT_DSI_DCS_LWRITE:
			parameter[1] = len & 0xff;
			parameter[2] = 0;
			memcpy(&parameter[4], cmd_line.data, len);
			crc = calculate_crc16(&parameter[4], len);
			parameter[len + 4] = crc & 0xff;
			parameter[len + 5] = (crc >> 8) & 0xff;
			len += 6;
			break;
		default:
			pr_info("%s: data type not supported 0x%8x\n",__func__, type);
			break;
		}

		parameter[3] = calculate_ecc(parameter);

		/* send dsi commands */
		ret = dsi_write_cmd(parameter, len, cmd_line.lp);
		if(ret)
			return -1;

		if (0 != cmd_line.delay)
			mdelay(cmd_line.delay);
	}
	return 0;
}

static int dsi_read_cmd_array(struct spacemit_dsi_device *dsi_ctx, struct spacemit_dsi_rx_buf *dbuf,
					struct spacemit_dsi_cmd_desc *cmds, int count)
{
	uint8_t parameter[SPACEMIT_DSI_MAX_RX_FIFO_BYTES];
	uint32_t i, rx_reg, timeout, tmp, packet,
	    data_pointer, byte_count;

	if(NULL == dsi_ctx) {
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	memset(dbuf, 0x0, sizeof(struct spacemit_dsi_rx_buf));
	dsi_write_cmd_array(dsi_ctx, cmds, count);

	timeout = 1000;
	do {
		timeout--;
		tmp = dsi_read(DSI_IRQ_ST);
	} while (((tmp & IRQ_RX_PKT) == 0) && timeout);
	if (0 == timeout) {
		pr_info("%s: dsi didn't receive packet, irq status 0x%x\n", __func__, tmp);
		return -1;
	}

	if (tmp & IRQ_RX_TRG3)
		pr_info("%s: not defined package is received\n", __func__);
	if (tmp & IRQ_RX_TRG2)
		pr_info("%s: ACK package is received\n", __func__);
	if (tmp & IRQ_RX_TRG1)
		pr_info("%s: TE trigger is received\n", __func__);
	if (tmp & IRQ_RX_ERR) {
		tmp = dsi_read(DSI_RX_PKT_HDR_0);
		pr_info("%s: error: ACK with error report (0x%x)\n", __func__, tmp);
	}

	packet = dsi_read(DSI_RX_PKT_ST_0);

	data_pointer = (packet & CFG_RX_PKT0_PTR_MASK) >> CFG_RX_PKT0_PTR_SHIFT;
	tmp = dsi_read(DSI_RX_PKT_CTRL_1);
	byte_count = tmp & CFG_RX_PKT_BCNT_MASK;

	memset(parameter, 0x00, byte_count);
	for (i = data_pointer; i < data_pointer + byte_count; i++) {
		rx_reg = dsi_read(DSI_RX_PKT_CTRL);
		rx_reg &= ~CFG_RX_PKT_RD_PTR_MASK;
		rx_reg |= CFG_RX_PKT_RD_REQ | (i << CFG_RX_PKT_RD_PTR_SHIFT);
		dsi_write(DSI_RX_PKT_CTRL, rx_reg);
		count = 10000;
		do {
			count--;
			rx_reg = dsi_read(DSI_RX_PKT_CTRL);
		} while (rx_reg & CFG_RX_PKT_RD_REQ && count);
		if ( 0 == count)
			pr_info("%s: error: read Rx packet FIFO error\n", __func__);
		parameter[i - data_pointer] = rx_reg & 0xff;
	}
	switch (parameter[0]) {
	case SPACEMIT_DSI_ACK_ERR_RESP:
		pr_info("%s: error: Acknowledge with error report\n", __func__);
		break;
	case SPACEMIT_DSI_EOTP:
		pr_info("%s: error: End of Transmission packet\n", __func__);
		break;
	case SPACEMIT_DSI_GEN_READ1_RESP:
	case SPACEMIT_DSI_DCS_READ1_RESP:
		dbuf->data_type = parameter[0];
		dbuf->length = 1;
		memcpy(dbuf->data, &parameter[1], dbuf->length);
		break;
	case SPACEMIT_DSI_GEN_READ2_RESP:
	case SPACEMIT_DSI_DCS_READ2_RESP:
		dbuf->data_type = parameter[0];
		dbuf->length = 2;
		memcpy(dbuf->data, &parameter[1], dbuf->length);
		break;
	case SPACEMIT_DSI_GEN_LREAD_RESP:
	case SPACEMIT_DSI_DCS_LREAD_RESP:
		dbuf->data_type = parameter[0];
		dbuf->length = (parameter[2] << 8) | parameter[1];
		memcpy(dbuf->data, &parameter[4], dbuf->length);
		break;
	}
	return 0;
}

static void dsi_open_dphy(struct spacemit_dsi_device* device_ctx,
								struct spacemit_mipi_info *mipi_info, bool ready)
{
	struct spacemit_dphy_ctx *dphy_config = NULL;

	dsi_get_dphy_setting(device_ctx);
	dphy_config = &device_ctx->dphy_config;
	dphy_config->phy_freq = device_ctx->bit_clk_rate / 1000;
	dphy_config->esc_clk = device_ctx->esc_clk_rate / 1000;
	if(mipi_info->split_enable)
		dphy_config->lane_num = mipi_info->lane_number >> 1;
	else
		dphy_config->lane_num = mipi_info->lane_number;
	dphy_config->status = DPHY_STATUS_UNINIT;

	if(ready){
		dphy_config->status = DPHY_STATUS_INIT;
		return;
	}

	spacemit_dphy_init(dphy_config);
}

static void dsi_close_dphy(struct spacemit_dsi_device* device_ctx)
{
	spacemit_dphy_uninit(&device_ctx->dphy_config);
}

int spacemit_dsi_open(struct spacemit_dsi_device* device_ctx, struct spacemit_mipi_info *mipi_info, bool ready)
{
	int lane_number;

	if((NULL == device_ctx) || (NULL == mipi_info)){
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	if(mipi_info->split_enable)
		lane_number = mipi_info->lane_number >> 1;
	else
		lane_number = mipi_info->lane_number;

	dsi_get_advanced_setting(device_ctx, mipi_info);

	if(!ready)
		dsi_reset();

	dsi_open_dphy(device_ctx, mipi_info, ready);
	if(!ready) {
		dsi_enable_split_mode(mipi_info->split_enable);
		dsi_enable_lptx_lanes(spacemit_dsi_lane[lane_number]);
		dsi_enable_eotp(mipi_info->eotp_enable);
	}

	device_ctx->status = DSI_STATUS_OPENED;
	return 0;
}

int spacemit_dsi_close(struct spacemit_dsi_device* device_ctx)
{
	if(NULL == device_ctx){
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	dsi_close_dphy(device_ctx);

	device_ctx->status = DSI_STATUS_UNINIT;
	return 0;
}

int spacemit_dsi_ready_for_datatx(struct spacemit_dsi_device* device_ctx, struct spacemit_mipi_info *mipi_info)
{
	if((NULL == device_ctx) || (NULL == mipi_info)){
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	if(mipi_info->work_mode == SPACEMIT_DSI_MODE_CMD){
	    dsi_config_cmd_mode(device_ctx, mipi_info);
	} else {
    	dsi_config_video_mode(device_ctx, mipi_info);
	}

    return 0;
}

int spacemit_dsi_close_datatx(struct spacemit_dsi_device* device_ctx)
{
	if(NULL == device_ctx){
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	dsi_enable_cmd_mode(false);
	dsi_enable_video_mode(false);

    return 0;
}

int spacemit_dsi_write_cmds(struct spacemit_dsi_device* device_ctx,
									struct spacemit_dsi_cmd_desc *cmds, int count)
{
	if((NULL == device_ctx) || (NULL == cmds)){
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	return dsi_write_cmd_array(device_ctx, cmds, count);
}

int spacemit_dsi_read_cmds(struct spacemit_dsi_device* device_ctx, struct spacemit_dsi_rx_buf *dbuf,
								struct spacemit_dsi_cmd_desc *cmds, int count)
{
	if((NULL == device_ctx) || (NULL == cmds)){
		pr_info("%s: Invalid param\n", __func__);
		return -1;
	}

	return dsi_read_cmd_array(device_ctx, dbuf, cmds, count);
}

struct spacemit_dsi_driver_ctx dsi_driver_ctx = {
	.dsi_open = spacemit_dsi_open,
	.dsi_close = spacemit_dsi_close,
	.dsi_write_cmds = spacemit_dsi_write_cmds,
	.dsi_read_cmds = spacemit_dsi_read_cmds,
	.dsi_ready_for_datatx = spacemit_dsi_ready_for_datatx,
	.dsi_close_datatx = spacemit_dsi_close_datatx,
};

struct spacemit_dsi_device spacemit_dsi_dev = {0};

int spacemit_dsi_probe(void)
{
	int ret;

	spacemit_dsi_dev.driver_ctx = &dsi_driver_ctx;
	spacemit_dsi_dev.status = DSI_STATUS_UNINIT;
	spacemit_dsi_dev.id = 0;

	spacemit_dsi_dev.esc_clk_rate = SPACEMIT_ESC_CLK_DEFAULT;

	ret = spacemit_dsi_register_device(&spacemit_dsi_dev);
	if(ret != 0){
		pr_info("%s: register dsi (%d) device fail!\n", __func__, spacemit_dsi_dev.id);
		return -1;
	}

	spacemit_dsi_dev.status = DSI_STATUS_INIT;

	return 0;
}
