/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifdef CONFIG_FASTLOGO

#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) || defined(CONFIG_VPP_USE_FASTLOGO_TA)
#include "avpll.h"
#include "vdec_com.h"
#endif
#include "vpp_api.h"
#else
#include "disp_ctrl.h"
#endif
#define PRN_DBG 0
//#define dbg_printf(LEVEL, FORMAT, ...) printf(FORMAT, __VA_ARGS__)
#define dbg_printf(LEVEL, FORMAT, ...)

#include "mv_display_api.h"
#if defined(CONFIG_VPP_USE_FASTLOGO_TA)
#include "OSAL_api.h"
#endif //CONFIG_VPP_USE_FASTLOGO_TA
#include "tee_ca_vpp.h"
#include "vpp_ca_wrap.h"
#include "common.h"

#define PRN_DEBUG_LEVEL PRN_DBG

#if defined IRQ_dHubIntrAvio0
#define IRQ_DHUB_INTR_AVIO_0 IRQ_dHubIntrAvio0
#elif defined(IRQ_dHubIntrAvio0_0)
#define IRQ_DHUB_INTR_AVIO_0 IRQ_dHubIntrAvio0_0
#else
#endif

#if defined(CONFIG_VPP_USE_FASTLOGO_TA)
#define WAIT_LOOP_COUNT 240
#else
#define WAIT_LOOP_COUNT 120
#endif //CONFIG_VPP_USE_FASTLOGO_TA
#define DEFAULT_DISPLAY_RES   RES_720P60
#define avioDhubSemMap_vpp_vppCPCB0_intr avioDhubSemMap_vpp128b_vpp_inr0
#define avioDhubSemMap_vpp_vppOUT4_intr avioDhubSemMap_vpp128b_vpp_inr6
#ifndef bTST
#define bTST(x, b) (((x) >> (b)) & 1)
#endif

#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) || defined(CONFIG_VPP_USE_FASTLOGO_TA)
#if (BERLIN_CHIP_VERSION >= BERLIN_BG5CT)
static VPP_WIN_ATTR showlogo_attr = {0x00801080, 0xFFF, 1};
#else
static VPP_WIN_ATTR showlogo_attr = {0x00801080, 0xFF, 1};
#endif
#endif

static volatile int loop_isr_count=0;

void MV_DISP_VPP_ISR(void)
{
#if !defined(CONFIG_VPP_USE_FASTLOGO_TA)
    int instat;
    HDL_semaphore *pSemHandle;
    /* VPP interrupt handling  */
    pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
    instat = semaphore_chk_full(pSemHandle, -1);
    if (bTST(instat, avioDhubSemMap_vpp_vppCPCB0_intr))
    {
       loop_isr_count++;
       semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr, 1);
       semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr);
       VPP_ISR_Handler(VPP_CC_MSG_TYPE_VPP,instat);
    }
    if (bTST(instat, avioDhubSemMap_vpp_vppOUT4_intr))
    {
       semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1);
       semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);
    }
#else
    int instat = 0;
    loop_isr_count++;
    VPP_ISR_Handler(VPP_CC_MSG_TYPE_VPP,instat);
#endif
}


int MV_DISP_InitDisplay(int resid, int hdmi_bitdepth)
{
    int ret = 0;

#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) || defined(CONFIG_VPP_USE_FASTLOGO_TA)
    //AVPLL_InitClock();
    dbg_printf(PRN_DEBUG_LEVEL,"[ConfigDisplay] AVPLL enabled\n");
#if !defined(CONFIG_VPP_USE_FASTLOGO_TA)
    DhubInitialization(0, VPP_DHUB_BASE, VPP_HBO_SRAM_BASE, &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS,DHUB_TYPE_128BIT );
    DhubInitialization(0, AG_DHUB_BASE, AG_HBO_SRAM_BASE, &AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS,DHUB_TYPE_64BIT );
#else
    VPP_INIT_PARM vpp_init_parm;
    ret = MV_VPPOBJ_Init(&vpp_init_parm);
    if(ret != MV_VPP_OK)
    {
        ret= MV_DISP_E_CREATE;
        goto EXIT;
    }
#endif
    ret = MV_VPP_Create(0,0, 1);
    if(ret != MV_VPP_OK)
    {
        ret= MV_DISP_E_CREATE;
        goto EXIT;
    }
    ret = MV_VPP_Reset();
    if(ret != MV_VPP_OK)
    {
        ret= MV_DISP_E_RST;
        goto EXIT;
    }
    ret = MV_VPP_Config();
    if(ret != MV_VPP_OK)
    {
        ret= MV_DISP_E_CFG;
        goto EXIT;
    }
    ret = MV_VPP_SetCPCBOutputResolution(CPCB_1, resid, hdmi_bitdepth);
    if(ret != MV_VPP_OK)
    {
        ret= MV_DISP_E_FAIL;
        goto EXIT;
    }
    ret = MV_VPP_SetHdmiTxControl(1);
    if(ret != MV_VPP_OK)
    {
        ret= MV_DISP_E_FAIL;
        goto EXIT;
    }

EXIT:
    return  ret;
#else
    ret = MV_VPP_Init(resid);
    if(ret != 0)
    {
        dbg_printf(PRN_DEBUG_LEVEL,"MV_VPP_Init failed\n");
        return ret;
    }
    return 0;
#endif
}

int MV_DISP_LogoStart(void *buf,int width,int height,int stride, int win_width, int win_height)
{
#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) || defined(CONFIG_VPP_USE_FASTLOGO_TA)
    VPP_WIN showlogo_win;
    unsigned size;
    VBUF_INFO *p_vbuf;

#if defined(CONFIG_VPP_USE_FASTLOGO_TA)
    p_vbuf = (VBUF_INFO*)(uintptr_t)VPP_TZ_ALLOC(sizeof(VBUF_INFO));
#else
    static VBUF_INFO vbuf;
    p_vbuf = &vbuf;
#endif

    //dbg_printf(PRN_DEBUG_LEVEL,"[showlogo] start: buf=0x0%8x, width=%d, heigth=%d, stride=%d %x %x %x\n", buf, width, height, stride,start[0],start[1]);

    //showlogo_init_irq();

    memset(p_vbuf,0,sizeof(VBUF_INFO));
    p_vbuf->m_pbuf_start    = (UINT32)(uintptr_t)buf; // base address of the frame buffer;
    p_vbuf->m_disp_offset   = 0;
    p_vbuf->m_active_left   = 0;
    p_vbuf->m_active_top    = 0;

    // adjust the following three fields to reflect the actual logo dimensions

    p_vbuf->m_buf_stride = stride;
    p_vbuf->m_active_width = width;
    p_vbuf->m_active_height = height;

    size = p_vbuf->m_buf_stride * p_vbuf->m_active_height;

    showlogo_win.x = 0;
    showlogo_win.y = 0;
    showlogo_win.width  = width;
    showlogo_win.height = height;

    MV_VPP_OpenDispWindow(PLANE_MAIN, &showlogo_win, &showlogo_attr);
#if defined(CONFIG_VPP_USE_FASTLOGO_TA)
    MV_VPPOBJ_SetRefWindow(0, PLANE_MAIN,&showlogo_win);
#endif
    flush_dcache_range((uintptr_t)buf,(uintptr_t)(((char *)buf) + size));
    //this call will enable isr
    MV_DisplayFrame(PLANE_MAIN,p_vbuf,SRCFMT_YUV422,p_vbuf->m_active_left,p_vbuf->m_active_top,width,height,INPUT_BIT_DEPTH_8BIT);
#else
    MV_VPP_DisplayPatternFrame(0, 0, width, height, stride, buf);
#endif
    return 0;
}


int MV_DISP_ConfigDisplay( int win_width, int win_height)
{
#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) ||  defined(CONFIG_VPP_USE_FASTLOGO_TA)
    VPP_WIN showlogo_win;
    int ret = 0;

    showlogo_win.x = 0;
    showlogo_win.y = 0;
    showlogo_win.width  = win_width;
    showlogo_win.height = win_height;

    //graphics
    ret = MV_VPP_OpenDispWindow(PLANE_GFX1, &showlogo_win, &showlogo_attr);
    if(ret != MV_VPP_OK)
    {
       ret= MV_DISP_E_WIN;
       goto EXIT;
    }
#if defined(CONFIG_VPP_USE_FASTLOGO_TA)
    MV_VPPOBJ_SetRefWindow(0, PLANE_GFX1, &showlogo_win);
#endif
    MV_VPP_SetDislayMode(PLANE_GFX1);
    ret = create_global_desc_array(SRCFMT_ARGB32,INPUT_BIT_DEPTH_8BIT,win_width,win_height);

    return MV_DISP_SUCESS;
EXIT:
    return  ret;
#else
    return 0;
#endif
}

void MV_DISP_Display_Frame(void *buf,int width,int height)
{
#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) ||  defined(CONFIG_VPP_USE_FASTLOGO_TA)
    unsigned size;
    VBUF_INFO *vbuf;
    get_vbuf_info(&vbuf);
    vbuf->m_pbuf_start   = (unsigned int)(unsigned long)buf;
    vbuf->m_disp_offset  = 0;
    vbuf->m_active_left  = 0;
    vbuf->m_active_top   = 0;
    vbuf->m_buf_stride = width*4;
    vbuf->m_active_width = width;
    vbuf->m_active_height = height;
    size = vbuf->m_buf_stride * vbuf->m_active_height;
    flush_dcache_range((uintptr_t)buf, (uintptr_t)(((char *)buf) + size));
    MV_VPPOBJ_RecycleFrames(0,PLANE_GFX1);
    MV_VPP_DisplayFrame(PLANE_GFX1,vbuf);
#else
    MV_VPP_DisplayGfxFrame(buf, width, height);
#endif
}

void MV_DISP_MuteDisplay(int mute)
{
#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) ||  defined(CONFIG_VPP_USE_FASTLOGO_TA)
    MV_VPPOBJ_SetPlaneMute(0,PLANE_GFX1,mute);
#endif
}
void MV_DISP_StopDisplay(void)
{
#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) ||  defined(CONFIG_VPP_USE_FASTLOGO_TA)
   loop_isr_count=0;
   while(loop_isr_count<WAIT_LOOP_COUNT);
   MV_DISP_MuteDisplay(1);
#endif
}

void MV_DISP_DestroyDisplay(void)
{
#if (BERLIN_CHIP_VERSION != BERLIN_BG5CT) ||  defined(CONFIG_VPP_USE_FASTLOGO_TA)
    MV_DISP_StopDisplay();
#if !defined(CONFIG_VPP_USE_FASTLOGO_TA)
    MV_VPP_Stop();
#else
    //MV_DISP_DisableIRQ();
#endif
    MV_VPP_Destroy();
#else
    MV_VPP_Deinit();
#endif
    dbg_printf(PRN_DEBUG_LEVEL,"[Display] stopped\n");
    return;
}

#ifdef CONFIG_VPP_ENABLE_AVPLL
void MV_DISP_ProbeClk(struct udevice *dev, int channel_id)
{
    MV_VPP_ProbeClk(dev, channel_id);
}
#endif //CONFIG_VPP_ENABLE_AVPLL
