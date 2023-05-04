/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

int MV_VPP_Init(void);
void MV_VPP_Enable_IRQ(void);
void MV_VPP_Deinit(void);
void MV_VPP_DisplayFrame(void **pBuf,int width,int height);
