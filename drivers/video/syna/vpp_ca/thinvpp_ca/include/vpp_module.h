/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef _VPP_MODULE_H_
#define _VPP_MODULE_H_

/* definition of video frame-rate */
enum {
	TYPE_SD = 0,
	TYPE_HD = 1,
	TYPE_UHD = 2,
};

/* definition of video scan mode */
enum {
	SCAN_PROGRESSIVE = 0,
	SCAN_INTERLACED  = 1,
};

enum {
	FRAME_RATE_23P98 = 0,
	FRAME_RATE_24	 = 1,
	FRAME_RATE_25	 = 2,
	FRAME_RATE_29P97 = 3,
	FRAME_RATE_30	 = 4,
	FRAME_RATE_47P96 = 5,
	FRAME_RATE_48	 = 6,
	FRAME_RATE_50	 = 7,
	FRAME_RATE_59P94 = 8,
	FRAME_RATE_60	 = 9,
	FRAME_RATE_100	 = 10,
	FRAME_RATE_119P88 = 11,
	FRAME_RATE_120	 = 12,
};
#endif

