/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

////////////////////////////////////////////////////////////////////////////////
//! \file vpp_api_tz.c
//!
//! \brief VPP API wrapper for trust zone.
//!
//! Purpose:
//!
//!     Version    Date                     Author
//!     V 1.00,    Nov 25 2013,             Huaiyu Liu
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "vpp_api.h"
#include "tee_ca_vpp.h"
#include "vbuf.h"
#include "vdec_com.h"
//#include "api_avio_dhub.h"
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_CDP_A0)
#include "avpll.h"
#endif

/* CONFIG_VPP_ENABLE_PASSPAR :
 *  pass VPP_VBUF by means of TEEC_PARAMETERS
 *  split VPP_VBUF into subunit big enough to pass by TEEC_PARAMETERS
 *  requires alteast 4 to 5 transaction of PASSPAR to pass VPP_VBUF
 *
 * If not passed as CONFIG_VPP_ENABLE_PASSPAR, then choose
 * CONFIG_VPP_ENABLE_PASSPHY:
 *  Enable to pass PHY address to TA using PASSPHY
 *      otherwise pass VIRT address using PASSSHM
 *  GMC branch uses PASSPHY
 *  CI dev_branch/master support PASSPHY & PASSSHM
 */
#ifdef CONFIG_VPP_ENABLE_PASSPAR
#define VppPassVbufInfo VppPassVbufInfoPar
#else
#ifdef  CONFIG_VPP_ENABLE_PASSPHY
#define VppPassVbufInfo VppPassVbufInfoPhy
#endif //CONFIG_VPP_ENABLE_PASSPHY
#endif //CONFIG_VPP_ENABLE_PASSPAR

//VPP API SHM internal control data structure
typedef struct {
    AMP_SHM_HANDLE            hShm;       /* shm handle */
    UINT32                    *phy_addr;  /* shm physical addr */
    UINT32                    *vir_addr;  /* shm virtul addr */
    MV_OSAL_HANDLE_MUTEX_t     hLock;     /* Lock for shm */
} vpp_shm_t;

/* Static variables */
static INT vpp_created;

/***********************************************************
 * FUNCTION: initialize VPP API trustzone instance
 * PARAMS: none
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EBADCALL - function called previously
 **********************************************************/
INT MV_VPP_Early_Init(VOID)
{
    VppInitialize();

    return (MV_VPP_OK);
}

/***********************************************************
 * FUNCTION: initialize VPP module
 * PARAMS: none
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EBADCALL - function called previously
 **********************************************************/
INT MV_VPPOBJ_Init(VPP_INIT_PARM *vpp_init_parm)
{
#if defined (TZ_3_0) && defined (VPP_ENABLE_INTERNAL_MEM_MGR)
    TEEC_Result result = TEEC_SUCCESS;
    AMP_SHM_HANDLE shmHandle;
    VppInitialize();
    result = VPP_ALLOC(0,SHM_SHARE_SZ,32,&shmHandle);
    if(result !=SUCCESS)
    {
         return MV_VPP_ENOMEM;
    }
    //printf("%s:%d:Memoery alloceted (%d bytes) : %x / %p\n",__func__, __LINE__, SHM_SHARE_SZ, shmHandle, shmHandle);
    vpp_init_parm->uiShmPA = (UINT32)shmHandle;
    vpp_init_parm->uiShmSize=SHM_SHARE_SZ;
#ifdef VPP_ENABLE_FLUSH_CACHE
    vpp_init_parm->iHDMIEnable = 1;
    vpp_init_parm->iVdacEnable = 1;
#endif //VPP_ENABLE_FLUSH_CACHE
#else
    VppInitialize();
#endif
    VppInit(vpp_init_parm);
    return (MV_VPP_OK);
}

/***********************************************
 * FUNCTION: create a VPP object
 * PARAMS: *handle - pointer to object handle
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 ***********************************************/
INT MV_VPPOBJ_Create(INT base_addr, int *handle)
{
    /* Check if VPP has already been created */
    if (vpp_created == 1) {
        return (MV_VPP_OK);
    }

    VppCreate();

    vpp_created = 1;

    return (MV_VPP_OK);
}

/***************************************
 * FUNCTION: VPP profile configuration
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
INT MV_VPPOBJ_Config(INT handle,
                     const INT *pvinport_cfg,
                     const INT *pdv_cfg,
                     const INT *pzorder_cfg,
                     const INT *pvoutport_cfg)
{

    return VppConfig();
}

/*******************************************************************
 * FUNCTION: set CPCB or DV output resolution
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        resID - id of output resolution
 *        bit_depth - HDMI deep color bit depth
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 * Note: this function has to be called before enabling a plane
 *       which belongs to that CPCB or DV.
 *******************************************************************/
INT MV_VPPOBJ_SetCPCBOutputResolution(INT handle,
                                      INT cpcbID,
                                      INT resID,
                                      INT bit_depth)
{
    HRESULT Ret = MV_VPP_OK;

    Ret = VppSetOutRes(cpcbID, resID, bit_depth);

    return (Ret);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT: color_fmt - color format (RGB, YCbCr 444, 422)
 *      : bit_depth - 8/10/12 bit color
 *      : pixel_rept - 1/2/4 repetitions of pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
INT MV_VPPOBJ_SetHdmiVideoFmt(INT handle, INT color_fmt, INT bit_depth, INT pixel_rept)
{
    return VppHdmiSetVidFmt(color_fmt, bit_depth, pixel_rept);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Tx control
 * INPUT: enable - disable/enable the HDMI Tx [0/1]
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
INT MV_VPPOBJ_SetHdmiTxControl(INT handle, INT enable)
{
    return VppSetHdmiTxControl(enable);
}

/******************************************************************************
 * FUNCTION: open a window of a video/graphics plane for display.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
INT MV_VPPOBJ_OpenDispWindow(INT handle,
                             INT planeID,
                             VPP_WIN *win,
                             VPP_WIN_ATTR *attr)
{
    HRESULT Ret = MV_VPP_OK;
    int params[8];

    if (!win)
        return (MV_VPP_EBADPARAM);

    if ((win->width<=0) || (win->height<=0))
        return (MV_VPP_EBADPARAM);

    params[0] = planeID;
    params[1] = win->x;
    params[2] = win->y;
    params[3] = win->width;
    params[4] = win->height;

    /* set video plane background window */
    if (attr) {
        params[5] = attr->bgcolor;
        params[6] = attr->alpha;
        params[7] = attr->globalAlphaFlag;
    } else {
        params[5] = -1;
        params[6] = -1;
        params[7] = -1;
    }

    Ret = VppOpenDispWin(params[0],
                   params[1],
                   params[2],
                   params[3],
                   params[4],
                   params[5],
                   params[6],
                   params[7]);

    return (Ret);
}

/*******************************************************************
 * FUNCTION: set display mode for a plane
 * INPUT: planeID - id of the plane
 *        mode - DISP_STILL_PIC: still picture, DISP_FRAME: frame
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
INT MV_VPPOBJ_SetDisplayMode(INT handle, INT planeID, INT mode)
{
    return VppSetDispMode(planeID, mode);
}

/*******************************************************************
 * FUNCTION: display a frame for a plane
 * INPUT: planeID - id of the plane
 *        *frame - frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
INT MV_VPPOBJ_DisplayFrame(INT handle, INT planeID, VOID *frame)
{
    VBUF_INFO *pVBufInfo,*pElInfo = NULL;
    INT DolbyValid = 0;

#ifndef CONFIG_FASTLOGO
    VDEC_VID_BUF_DESC *pDesc;
    pDesc = (VDEC_VID_BUF_DESC *)frame;
    pVBufInfo = (VBUF_INFO *)pDesc->user_data;
#else
    pVBufInfo = (VBUF_INFO *)frame;
#endif
#if CONFIG_MV_AMP_TEE_ENABLE
    pVBufInfo->m_hDesc = (UINT32)(intptr_t)frame;
#endif
    #ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
    printf("%s:%d: m_is_compressed:%x, m_primaries:%x m_iDisplayOETF:%x m_vitmyuvRangeIn:%x"
            "m_thdr_present_mode:%x, m_dv_status:%x, m_sar_height:%x\n",
             __func__, __LINE__, &(pVBufInfo->m_is_compressed), &(pVBufInfo->m_primaries), &(pVBufInfo->m_iDisplayOETF), &(pVBufInfo->m_vitmyuvRangeIn),
            &(pVBufInfo->m_thdr_present_mode), &(pVBufInfo->m_dv_status), &(pVBufInfo->m_sar_height)
            );
    #endif

    /* pass frame info */
    VppPassVbufInfo((UINT32 *)pVBufInfo, sizeof(VBUF_INFO),
                    (UINT32 *)pElInfo,sizeof(VBUF_INFO),
                    planeID, DolbyValid, DISPLAY_FRAME);

    return (MV_VPP_OK);
}

/***************************************************************
 * FUNCTION: set the reference window for a plane
 * INPUT: planeID - id of the plane
 *        *win - pointer to the reference window struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
INT MV_VPPOBJ_SetRefWindow(INT handle,
        INT planeID,
        VPP_WIN *win)
{
    HRESULT Ret = MV_VPP_OK;

    Ret = VppSetRefWin(planeID,
            win->x,
            win->y,
            win->width,
            win->height);

    return (Ret);
}

/******************************************************************************
 * FUNCTION: change a window of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
INT MV_VPPOBJ_ChangeDispWindow(INT handle,
        INT planeID,
        VPP_WIN *win,
        VPP_WIN_ATTR *attr)
{
    HRESULT Ret = MV_VPP_OK;
    int params[8];

    if((!win)&&(!attr))
        return MV_VPP_EBADPARAM;

    params[0] = planeID;

    if (win) {
        if ((win->width<=2) || (win->height<=2))
            return (MV_VPP_EBADPARAM);

        params[1] = win->x;
        params[2] = win->y;
        params[3] = win->width;
        params[4] = win->height;
    } else {
        params[1] = -1; /* no change */
        params[2] = -1;
        params[3] = -1;
        params[4] = -1;
    }
    if (attr) {
        params[5] = attr->bgcolor;
        params[6] = attr->alpha;
        params[7] = attr->globalAlphaFlag;
    } else {
        params[5] = -1; /* no change */
        params[6] = -1;
        params[7] = -1;
    }

    Ret = VppChangeDispWin(params[0],
            params[1],
            params[2],
            params[3],
            params[4],
            params[5],
            params[6],
            params[7]);

    return (Ret);

}

/***************************************************
 * FUNCTION: mute/un-mute a plane
 * PARAMS:  planeID - plane to mute/un-mute
 *          mute - 1: mute, 0: un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
INT MV_VPPOBJ_SetPlaneMute(INT handle, INT planeID, INT mute)
{
    return VppSetPlaneMute(planeID, mute);
}

/************************************************************************
 * FUNCTION: recycle plane frame descriptors
 * INPUT: planeID - id of the plane
 * RETURN: NULL - no frame descriptor for recycle
 *         other - pointer to frame descriptor being recycled
 ************************************************************************/
uintptr_t MV_VPPOBJ_RecycleFrames(int handle, int planeID)
{
    return (uintptr_t)VppRecycleFrame(planeID);
}

int MV_VPPOBJ_Stop(int handle)
{
    return Vppstop();
}

/***************************************
 * FUNCTION: VPP reset
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
INT MV_VPPOBJ_Reset(INT handle)
{
    return VppReset();
}

/***********************************************
 * FUNCTION: destroy a VPP object
 * PARAMS: handle - VPP object handle
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 ***********************************************/
INT MV_VPPOBJ_Destroy(INT handle)
{
    VppDestroy();

    return (MV_VPP_OK);
}
