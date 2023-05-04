/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef _VPP_CA_WRAP_H_
#define _VPP_CA_WRAP_H_

#include "vpp_ca_wrap.h"
#include "vdec_com.h"
#include "com_type.h"
#include "tee_ca_vpp.h"

#include <common.h>
#include <linux/errno.h>
#include <clk.h>


typedef struct _AVPLL_RES_DESC_
{
    int resID;
    int pixelclk;
    int ppm1ken; //0 for 50/59.94Hz SD & 59.94/29.97/23.98 HD resolutions
}AVPLL_RES_DESC;

int VPP_ISR_Handler(UINT32 msg_id, UINT32 intstat);
void MV_VPP_SetDislayMode(int planeID);
int MV_VPP_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth);
int MV_VPP_SetHdmiTxControl(int enable);
int MV_VPP_Create(unsigned int base, unsigned int hdmi_addr, unsigned options);
int MV_VPP_Reset(void);
int MV_VPP_Config(void);
int MV_VPP_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);
void MV_VPP_DisplayFrame(int planeID, VBUF_INFO *vbufinfo);
void MV_DisplayFrame(int planeID, VBUF_INFO *vbufinfo, int src_fmt,INT32 x, INT32 y, INT32 width, INT32 height, int bitdepth);
int MV_VPP_Stop(void);
int MV_VPP_Destroy(void);
void MV_VPP_ProbeClk(struct udevice *dev, int channel_id);

HRESULT create_global_desc_array(int src_fmt, int bitdepth,int width, int height);
HRESULT get_vbuf_info(VBUF_INFO **pVBInfo);
void build_frames( VBUF_INFO *vbufinfo, INT32 srcfmt,
        INT32 bit_depth, INT32 x, INT32 y, INT32 width, INT32 height, INT32 progressive, INT32 pattern_type,bool IsPatt);
#endif
