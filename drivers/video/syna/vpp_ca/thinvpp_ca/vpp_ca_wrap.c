/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#include "vpp_api.h"
#include "vpp_ca_wrap.h"
#include "vbuf.h"
#include "vdec_com.h"
#include "avpll.h"
#include "tee_ca_vpp.h"

#if (BERLIN_CHIP_VERSION>=BERLIN_VS680)
int MV_VPP_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth)
{
#ifdef CONFIG_VPP_ENABLE_AVPLL
    AVPLL_SetClockFreqForResID(resID);
#endif //CONFIG_VPP_ENABLE_AVPLL

    MV_VPPOBJ_SetCPCBOutputResolution(0, cpcbID, resID, bit_depth);

    return MV_VPPOBJ_SetHdmiVideoFmt(0, OUTPUT_COLOR_FMT_RGB888, bit_depth, 1);
}

int MV_VPP_Reset(void)
{
    MV_VPPOBJ_Reset(0);
    return 0;
}

#else
#include "pllDiag.h"

#define  VPP_SYS_CLK_SEL    17

AVPLL_RES_DESC res_info[] = {
    {RES_525P60,  27027,  1},
    {RES_720P50,  74250,  0},
    {RES_720P60,  74250,  1},
    {RES_1080P60, 148500, 1},
    {RES_1080P50, 148500, 1},
    {}
};

static int get_pixclk(int resID, int *ppixclk, int *pppm1k)
{
    int res_desc_sz, index, rc = -1;
    res_desc_sz = sizeof(res_info)/sizeof(AVPLL_RES_DESC);
    for(index = 0; index < res_desc_sz; index++)
    {
        if(res_info[index].resID == resID){
            *ppixclk = res_info[index].pixelclk;
            *pppm1k = res_info[index].ppm1ken;
            rc = 0;
        }
    }
    return rc;
}

static void config_avpll_videoclk(int resID)
{
    int pixelclk = 0;
    int ppm1k_en = 0;
    int avpll_freq_index = 0;
    float     ovsmp_index = 0;

    if(get_pixclk(resID, &pixelclk, &ppm1k_en) < 0)
        return;

    if(pixelclk<= 25200)
    {
        avpll_freq_index = 0;
        ovsmp_index = 4.0;
    }
    else if((pixelclk == 27000) || (pixelclk == 27027))
    {
        avpll_freq_index = 1;
        ovsmp_index = 4.0;
    }
    else if((pixelclk == 74250) || (pixelclk == 74176))
    {
        avpll_freq_index = 3;
        ovsmp_index = 2.0;
    }
    else if((pixelclk == 148500) || (pixelclk == 148352))
    {
        avpll_freq_index = 5;
        ovsmp_index = 1.0;
    }
    else if((pixelclk == 297000) || (pixelclk == 296703))
    {
        avpll_freq_index = 5;
        ovsmp_index = 2.0;
    }
    else if((pixelclk == 594000) || (pixelclk == 593406))
    {
        avpll_freq_index = 5;
        ovsmp_index = 4.0;
    }
    AVPLL_SetVideoFreq(AVPLL_A,avpll_freq_index, 0, ppm1k_en, ovsmp_index, 1);
    return;
}

int MV_VPP_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth)
{
    int reschng_stage1 = 24, reschng_stage2 = 25, reschng_stageID;

    reschng_stageID = resID | (1<<reschng_stage1);
    MV_VPPOBJ_SetCPCBOutputResolution(0, cpcbID, reschng_stageID, bit_depth);

    config_avpll_videoclk(resID);

    reschng_stageID = resID | (1<<reschng_stage2);
    MV_VPPOBJ_SetCPCBOutputResolution(0, cpcbID, reschng_stageID, bit_depth);

    return MV_VPPOBJ_SetHdmiVideoFmt(0, OUTPUT_COLOR_FMT_RGB888, bit_depth, 1);
}

int MV_VPP_Reset(void)
{
    MV_VPPOBJ_Reset(0);
    diag_clock_change_otherClk(VPP_SYS_CLK_SEL, 1, 1, 1);
    diag_clockFreq(AVPLL_A, 7, 660000000, 5);
    return 0;
}
#endif

int VPP_ISR_Handler(UINT32 msg_id, UINT32 intstat)
{
    VppIsrHandler(msg_id, intstat);
    return 0;
}

void MV_VPP_SetDislayMode(int planeID)
{
    MV_VPPOBJ_SetDisplayMode(0, planeID, DISP_FRAME);
    return;
}


int MV_VPP_SetHdmiTxControl(int enable)
{
    return MV_VPPOBJ_SetHdmiTxControl(0, enable);
}

int MV_VPP_Create(unsigned int base, unsigned int hdmi_addr, unsigned options)
{
    MV_VPPOBJ_Create(0,0);//inputs are unused.
    return 0;
}


int MV_VPP_Config(void)
{
    MV_VPPOBJ_Config(0, NULL, NULL, NULL, NULL);
    return 0;
}

int MV_VPP_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
    MV_VPPOBJ_OpenDispWindow(0, planeID, win, attr);
    return 0;
}

void MV_VPP_DisplayFrame(int planeID, VBUF_INFO *vbufinfo)
{
    MV_VPPOBJ_DisplayFrame(0, planeID,vbufinfo);
    return;
}

void MV_DisplayFrame(int planeID, VBUF_INFO *vbufinfo, int src_fmt,INT32 x, INT32 y, INT32 width, INT32 height, int bitdepth)
{
    MV_VPP_SetDislayMode(planeID);
    build_frames( vbufinfo, src_fmt, bitdepth,  x,  y,  width,  height,  1, 0,0);
    MV_VPPOBJ_DisplayFrame(0, planeID,vbufinfo);
    return;
}

int MV_VPP_Stop(void)
{
    MV_VPPOBJ_Stop(0);
    return 0;
}

int MV_VPP_Destroy(void)
{
    MV_VPPOBJ_Destroy(0);
    return 0;
}
