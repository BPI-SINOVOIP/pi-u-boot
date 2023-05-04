/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 *
 */

#include "soc.h"
#include "com_type.h"
#include "mv_display_api.h"
#include "OSAL_api.h"
#include "vpp.h"
#include "string.h"
#include "common.h"

#ifndef DEBUG
#define DBG(x...) do {} while (0)
#else
#define DBG(x...) printf(x)
#endif

#define HDMITX_ENABLE_TIMEOUT
#define HDMITX_TIMEOUT_COUNT  500

#define FRAME_RATE 60
#define BG4CT_A0

#define avioDhubSemMap_vpp_vppCPCB0_intr avioDhubSemMap_vpp128b_vpp_inr0
#define avioDhubSemMap_vpp_vppOUT4_intr avioDhubSemMap_vpp128b_vpp_inr6
#define bTST(x, b) (((x) >> (b)) & 1)

#define MEMMAP_AVIO_REG_BASE 0xF7400000
#define CONFIG_MV88DE3010_REGAREA_BASE MEMMAP_AVIO_REG_BASE
#define CONFIG_MV88DE3010_REGAREA_SIZE 0xB8000
#define CONFIG_AVPLL_REGAREA_BASE 0xF7EA0000
#define CONFIG_AVPLL_REGAREA_SIZE 0x10000

static int g_pushframe_done = 0;
static int frm_count = 0;
static int total_frm_count = 0;

VOID MV_VPP_DisplayPatternFrame(INT32 x,
            INT32 y, INT32 w, INT32 h, INT32 stride)
{
    unsigned char *fb_baseAddr=(unsigned char *)(uintptr_t)GaloisMalloc(w*h*4);//ARGB
    memset(fb_baseAddr, 0x0, w*h*4);
    MV_DISP_Display_Frame(fb_baseAddr, w, h);
    g_pushframe_done++;
}

VOID MV_VPP_Display_Frame(void **pBuf,int width,int height)
{
    int size = 1280*720*4;

    flush_dcache_range((uintptr_t)*pBuf, (uintptr_t)(((char *)*pBuf) + size));
    //printf("%s:%d - cnt:%d - %d/%d, buffer:%x\n", __func__, __LINE__, total_frm_count, frm_count, g_pushframe_done, *pBuf);

    MV_DISP_Display_Frame(*pBuf, width, height);
    g_pushframe_done++;

}

VOID VPP_ISR_Handler_irq(VOID *param)
{
    MV_DISP_VPP_ISR();
    total_frm_count++;
    if(g_pushframe_done) {
        frm_count++;
        g_pushframe_done = 0;
    }
}

INT32 MV_VPP_Init(VOID)
{
    {
        int display_primary_HDMI_BitDepth = 2;
        int display_primary_system = 13; //RES_720P60
        int display_primary_width = 1280;
        int display_primary_height = 720;

        //MV_VPP_Init, MV_VPP_Create, MV_VPP_Reset, MV_VPP_Config, MV_VPP_SetCPCBOutputResolution,
        MV_DISP_InitDisplay(display_primary_system, display_primary_HDMI_BitDepth);

        //GFX1: MV_VPP_OpenDispWindow, MV_VPPOBJ_SetRefWindow, MV_VPP_SetDislayMode, create_global_desc_array(SRCFMT_ARGB32)
        MV_DISP_ConfigDisplay(display_primary_width, display_primary_height);

        /*Note: VPP driver enable interrupt as part of PUSHFRAME
         *For TZ: enable AVIO interrupt only after enbling interrupt in VPP driver.
         *Otherwise TZK/bootloader executed from single process env on single core will return error for PUSHFRAME callback.*/
    }

    pr_info("MV_VPP_Init\n");

    return 0;
}


// MPCore GIC interrupt IDs
#define MP_BERLIN_INTR_ID(id)   (id + 32)   // berlin interrupts starts from ID 32

#if defined IRQ_dHubIntrAvio0
#define IRQ_DHUB_INTR_AVIO_0 IRQ_dHubIntrAvio0
#elif defined(IRQ_dHubIntrAvio0_0)
#define IRQ_DHUB_INTR_AVIO_0 IRQ_dHubIntrAvio0_0
#else
#endif

void MV_VPP_Enable_IRQ(void)
{
    irq_install_handler(MP_BERLIN_INTR_ID(IRQ_DHUB_INTR_AVIO_0), VPP_ISR_Handler_irq, NULL);
}

VOID MV_VPP_Deinit(VOID)
{
    irq_free_handler(MP_BERLIN_INTR_ID(IRQ_DHUB_INTR_AVIO_0));
}

int getFrmCount(void)
{
	return frm_count;
}

