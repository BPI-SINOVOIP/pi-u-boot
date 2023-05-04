/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef _VPP_CFG_H_
#define _VPP_CFG_H_

#include "vpp_api.h"

typedef struct RESOLUTION_INFO_T {
	int active_width;
	int active_height;
	int width;
	int height;
	int hfrontporch;
	int hsyncwidth;
	int hbackporch;
	int vfrontporch;
	int vsyncwidth;
	int vbackporch;
	int type;
	int scan;
	int frame_rate;
	int flag_3d;
	int freq;
	int pts_per_cnt_4;
} RESOLUTION_INFO;

#ifndef _VPP_CFG_C_
/* VPP module constant tables */
extern const RESOLUTION_INFO m_resinfo_table[MAX_NUM_RESS];

#endif

#endif
