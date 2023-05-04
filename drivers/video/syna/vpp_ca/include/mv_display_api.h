/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef _MV_DISP_INTERFACE_H_
#define _MV_DISP_INTERFACE_H_


#define MV_DISP_SUCESS         (0)
#define MV_DISP_E_RES_CFG     (1)
#define MV_DISP_E_CREATE      (2)
#define MV_DISP_E_CFG         (3)
#define MV_DISP_E_RST         (4)
#define MV_DISP_E_WIN         (5)
#define MV_DISP_E_DISP        (6)
#define MV_DISP_E_NOMEM       (7)
#define MV_DISP_E_FAIL        (8)

/********************************************************************************
 * FUNCTION : Get width, height and display resolution of the system
 * OUTPUT   : resid - resolution ID, refer vpp_api.h
 *            width - width correponding to resolution
 *            height - height correponding to resolution
 *
 * RETURN   : None
 ********************************************************************************/
void MV_DISP_GetDisplayInfo(int *resid, int *width, int *height);

/********************************************************************************
 * FUNCTION : Config the system  with fixed resolution
 * INPUT    : resid - resolution ID, refer vpp_api.h
 *
 *
 * RETURN: MV_DISP_E_SUC - SUCCEED
 *         MV_DISP_E_RES_CFG - Failed to configure resolution
 *         MV_DISP_E_CREATE - Failed to instantiate VPP
 *         MV_DISP_E_RST - Failed to reset VPP
 *         MV_DISP_E_CFG - Failed to configure VPP
 *         MV_DISP_E_WIN - Failed to configure VPP Window
 *         MV_DISP_E_DISP - Failed to configure VPP display
 ********************************************************************************/

int MV_DISP_ConfigDisplay(int win_width, int win_height);
//int MV_DISP_ConfigDisplay(int resid);
/********************************************************************************
 * FUNCTION : Get width, height and display resolution of the system
 * INPUT    : buf - pointer to frame buffer
 *            width - width of the frame
 *            height - height of the frame
 *
 * RETURN  : None
 ********************************************************************************/

void MV_DISP_Display_Frame(void *buf,int width,int height);
/********************************************************************************
 * FUNCTION : Mute the graphic display
 * INPUT    : mute - 1:mute 0:unmute
 *
 * RETURN   : None
 ********************************************************************************/

void MV_DISP_MuteDisplay(int mute);
/********************************************************************************
 * FUNCTION  : Stop the graphic display
 * INPUT     : None
 *
 * RETURN   : None
 ********************************************************************************/

void MV_DISP_StopDisplay(void);
/********************************************************************************
 * FUNCTION : Destroy the graphic display
 * INPUT    : None
 *
 *
 * RETURN   : None
 ********************************************************************************/
void MV_DISP_DestroyDisplay(void);

int MV_DISP_InitDisplay(int resid, int hdmi_bitdepth);
void MV_DISP_VPP_ISR(void);
void MV_DISP_EnableIRQ(void);
void MV_VPP_Display_Frame(void **pBuf,int width,int height);
void MV_VPP_Enable_IRQ(void);
void MV_VPP_Deinit(void);
int MV_VPP_Init(void);

#endif


