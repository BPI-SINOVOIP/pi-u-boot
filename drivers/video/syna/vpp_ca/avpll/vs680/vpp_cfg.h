/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#ifndef _VPP_CFG_H_
#define _VPP_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* definition of video resolution type */
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

/* definition of video frame-rate */
enum {
        FRAME_RATE_23P98 = 0,
        FRAME_RATE_24    = 1,
        FRAME_RATE_25    = 2,
        FRAME_RATE_29P97 = 3,
        FRAME_RATE_30    = 4,
        FRAME_RATE_47P96 = 5,
        FRAME_RATE_48    = 6,
        FRAME_RATE_50    = 7,
        FRAME_RATE_59P94 = 8,
        FRAME_RATE_60    = 9,
        FRAME_RATE_100   = 10,
        FRAME_RATE_119P88 = 11,
        FRAME_RATE_120   = 12,
        FRAME_RATE_89P91 = 13,
        FRAME_RATE_90    = 14
};

typedef struct RESOLUTION_INFO_T {
    int active_width;
    int active_height;   /* active height of channel in pixel */
    int width;  /* width of channel in pixel */
    int height; /* height of channel in pixel */
    int hfrontporch; /* front porch of hsync */
    int hsyncwidth; /* hsync width */
    int hbackporch; /* back porch of hsync */
    int vfrontporch; /* front porch of vsync */
    int vsyncwidth; /* vsync width */
    int vbackporch; /* back porch of vsync */
    int type;   /* resolution type: HD or SD */
    int scan;   /* scan mode: progressive or interlaced */
    int frame_rate;   /* frame rate */
    int flag_3d;   /* 1 for 3D, 0 for 2D */
    int freq;   /* pixel frequency */
    int pts_per_cnt_4;   /* time interval in term of PTS for every 4 frames */
}RESOLUTION_INFO;

int video_framerate_num_den(int resID, int * num, int * den);
#ifndef _VPP_CFG_C_

/* VPP module constant tables */
extern const RESOLUTION_INFO m_resinfo_table[MAX_NUM_RESS];

#endif

#ifdef __cplusplus
}
#endif

#endif


