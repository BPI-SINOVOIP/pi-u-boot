/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#define _VPP_CFG_C_

#include "OSAL_api.h"
#include "vpp_api.h"
#include "vpp_cfg.h"

/////////////////////////////////////////////////////////////
// VPP module constant tables
/////////////////////////////////////////////////////////////

/* VPP TG standard timing format information table */
const RESOLUTION_INFO m_resinfo_table[MAX_NUM_RESS] = {
                     /* { active_width, active_height, width, height, hfrontporch, hsyncwidth, hbackporch, vfrontporch, vsyncwidth, vbackporch, type, scan, frame_rate, flag_3d, freq, pts_per_cnt_4 } */
    /*  0: RES_NTSC_M              */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_59P94,    0,  27000,  6006 },
    /*  1: RES_NTSC_J              */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_59P94,    0,  27000,  6006 },
    /*  2: RES_PAL_M               */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_59P94,    0,  27000,  6006 },
    /*  3: RES_PAL_BGH             */ {     720,          576,         864,    625,    12,          63,         69,         2,           3,          19,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_50,       0,  27000,  7200 },
    /*  4: RES_525I60              */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_60,       0,  27027,  6000 },
    /*  5: RES_525I5994            */ {     720,          480,         858,    525,    19,          62,         57,         4,           3,          15,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_59P94,    0,  27000,  6006 },
    /*  6: RES_625I50              */ {     720,          576,         864,    625,    12,          63,         69,         2,           3,          19,   TYPE_SD,   SCAN_INTERLACED, FRAME_RATE_50,       0,  27000,  7200 },
    /*  7: RES_525P60              */ {     720,          480,         858,    525,    16,          62,         60,         9,           6,          30,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0,  27027,  6000 },
    /*  8: RES_525P5994            */ {     720,          480,         858,    525,    16,          62,         60,         9,           6,          30,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0,  27000,  6006 },
    /*  9: RES_625P50              */ {     720,          576,         864,    625,    12,          64,         68,         5,           5,          39,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0,  27000,  7200 },
    /* 10: RES_720P30              */ {    1280,          720,         3300,   750,    1760,        40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_30,      0,  74250, 12000 },
    /* 11: RES_720P2997            */ {    1280,          720,         3300,   750,    1760,        40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_29P97,   0,  74176, 12012 },
    /* 12: RES_720P25              */ {    1280,          720,         3960,   750,    2420,        40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_25,      0,  74250, 14400 },
    /* 13: RES_720P60              */ {    1280,          720,         1650,   750,    110,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0,  74250,  6000 },
    /* 14: RES_720P5994            */ {    1280,          720,         1650,   750,    110,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0,  74176,  6006 },
    /* 15: RES_720P50              */ {    1280,          720,         1980,   750,    440,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0,  74250,  7200 },
    /* 16: RES_1080I60             */ {    1920,         1080,         2200,   1125,   88,          44,         148,        2,           5,          15,   TYPE_HD,   SCAN_INTERLACED,  FRAME_RATE_60,      0, 148500,  6000 },
    /* 17: RES_1080I5994           */ {    1920,         1080,         2200,   1125,   88,          44,         148,        2,           5,          15,   TYPE_HD,   SCAN_INTERLACED,  FRAME_RATE_59P94,   0, 148352,  6006 },
    /* 18: RES_1080I50             */ {    1920,         1080,         2640,   1125,   528,         44,         148,        2,           5,          15,   TYPE_HD,   SCAN_INTERLACED,  FRAME_RATE_50,      0, 148500,  7200 },
    /* 19: RES_1080P30             */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 74250,  12000 },
    /* 20: RES_1080P2997           */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_29P97,   0, 74176,  12012 },
    /* 21: RES_1080P25             */ {    1920,         1080,         2640,   1125,   528,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_25,      0, 74250,  14400 },
    /* 22: RES_1080P24             */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_24,      0, 74250,  15000 },
    /* 23: RES_1080P2398           */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_23P98,   0, 74176,  15015 },
    /* 24: RES_1080P60             */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 148500,  6000 },
    /* 25: RES_1080P5994           */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 148352,  6006 },
    /* 26: RES_1080P50             */ {    1920,         1080,         2640,   1125,   528,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0, 148500,  7200 },
    /* 27: RES_LVDS_1080P48        */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      0, 148500,  7500 },
    /* 28: RES_LVDS_1080P50        */ {    1920,         1080,         2200,   1350,   32,          44,         204,        229,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0, 148500,  7200 },
    /* 29: RES_LVDS_1080P60        */ {    1920,         1080,         2200,   1125,   32,          44,         204,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 148500,  6000 },
    /* 30: RES_LVDS_2160P12        */ {    1920,         1080,         2200,   1406,   32,          44,         204,        235,         5,          86,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      0, 148470,  7500 },
    /* 31: RES_VGA_480P60          */ {    640,           480,         800,    525,    16,          96,         48,         10,          2,          33,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 25200,   6000 },
    /* 32: RES_VGA_480P5994        */ {    640,           480,         800,    525,    16,          96,         48,         10,          2,          33,   TYPE_SD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 25175,   6006 },
    /* 33: RES_720P50_3D           */ {    1280,          720,         1980,   750,    440,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     1, 148500,  7200 },
    /* 34: RES_720P60_3D           */ {    1280,          720,         1650,   750,    110,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },
    /* 35: RES_720P5994_3D         */ {    1280,          720,         1650,   750,    110,         40,         220,        5,           5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_119P88,  1, 148352,  6006 },
    /* 36: RES_1080P24_3D          */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      1, 148500, 15000 },
    /* 37: RES_1080P2398_3D        */ {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_47P96,   1, 148352, 15015 },
    /* 38: RES_1080P30_3D          */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      1, 148500, 12000 },
    /* 39: RES_1080P2997_3D        */ {    1920,         1080,         2200,   1125,   88,          44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   1, 148352, 12012 },
    /* 40: RES_1080P25_3D          */ {    1920,         1080,         2640,   1125,   528,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      1, 148500, 14400 },
    /* 41: RES_1080I60_FP          */ {    1920,         2228,         2200,   2250,    88,         44,         148,        2,           5,          15,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 148500,  12000},
    /* 42: RES_1080I5994_FP        */ {    1920,         2228,         2200,   2250,    88,         44,         148,        2,           5,          15,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 148352,  12000},
    /* 43: RES_1080I50_FP          */ {    1920,         2228,         2640,   2250,   528,         44,         148,        2,           5,          15,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_25,      0, 148500,  14400},
    /* 44: RES_LVDS_1920X540P60_3D */ {    1920,          540,         1980,    625,     8,         44,          8,        16,           5,          64,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },
    /* 45: RES_LVDS_1920X540P30_3D */ {    1920,          540,         1980,    625,     8,         44,          8,        16,           5,          64,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      1, 74250,  12000 },
    /* 46: RES_LVDS_1920X540P24_3D */ {    1920,          540,         2475,    625,   407,         44,        104,        16,           5,          64,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      1, 74250,  15000 },
    /* 47: RES_LVDS_720P100_3D  */    {    1280,          720,         1980,    750,   440,         40,         220,         5,          5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     1, 148500,  7200 },
    /* 48: RES_LVDS_720P120_3D  */    {    1280,          720,         1650,    750,   110,         40,         220,         5,          5,          20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },
    /* 49: RES_LVDS_1080P48_3D */     {    1920,         1080,         2750,   1125,   638,         44,         148,        4,           5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      1, 148500, 15000 },
    /* 50: RES_LVDS_1080P50_3D */     {    1920,         1080,         2200,    1350,   32,         44,         204,        229,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      1, 148500, 14400 },
    /* 51: RES_LVDS_1080P60_3D */     {    1920,         1080,         2200,   1125,   32,          44,         204,          4,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      1, 148500, 12000 },
    /* 52: RES_LVDS_1920x540P100_3D */{    1920,          540,          2200,   675,    32,         44,         204,         94,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     1, 148500,  7200 },
    /* 53: RES_LVDS_1920X540P120_3D */{    1920,          540,          2202,   562,    32,         44,         206,          7,         5,          8,    TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },
    /* 54: RES_LVDS_960x1080P100_3D */{     960,         1080,         1100,   1350,   42,          44,         54,         229,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     1, 148500,  7200 },
    /* 55: RES_LVDS_960x1080P120_3D */{     960,         1080,         1100,   1125,   42,          44,         54,           4,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },

    /* 56: RES_4Kx2K2398    */        {    3840,         2160,         5500,   2250,   1276,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_23P98,   0, 296703, 15015 },
    /* 57: RES_4Kx2K24    */          {    3840,         2160,         5500,   2250,   1276,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_24,      0, 297000, 15000 },
    /* 58: RES_4Kx2K24_SMPTE */       {    4096,         2160,         5500,   2250,   1020,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_24,      0, 297000, 15000 },
    /* 59: RES_4Kx2K25    */          {    3840,         2160,         5280,   2250,   1056,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_25,      0, 297000, 14400 },
    /* 60: RES_4Kx2K2997    */        {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_29P97,   0, 296703, 12012 },
    /* 61: RES_4Kx2K30    */          {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 297000, 12000 },
    /* 62: RES_4Kx2K50    */          {    3840,         2160,         5280,   2250,   1056,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_50,      0, 594000,  7200 },
    /* 63: RES_4Kx2K5994    */        {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 593406,  6006 },
    /* 64: RES_4Kx2K60    */          {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 594000,  6000 },
    /* 65: RES_4Kx2K30_HDMI*/         {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 297000, 12000 },
    /* 66: RES_4Kx1K120    */         {    3840,         1080,         4400,   1125,   176,         88,         296,          4,          5,         36,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_120,     0, 594000,  3000 },
    /* 67: RES_720P_4Kx1K120_3D */    {    1280,          720,         1650,   750,    110,         40,         220,          5,          5,         20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     1, 148500,  6000 },

    /* 68: RES_720P100  */            {    1280,          720,         1980,   750,    440,         40,         220,          5,          5,         20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     0, 148500,  3600 },
    /* 69: RES_720P11988  */          {    1280,          720,         1650,   750,    110,         40,         220,          5,          5,         20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_119P88,  0, 148500,  3000 },
    /* 70: RES_720P120  */            {    1280,          720,         1650,   750,    110,         40,         220,          5,          5,         20,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     0, 148500,  3000 },
    /* 71: RES_1080P100 */            {    1920,         1080,         2640,  1125,    528,         44,         148,          4,          5,         36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_100,     0, 297000,  3600 },
    /* 72: RES_1080P11988 */          {    1920,         1080,         2200,  1125,    88,          44,         148,          4,          5,         36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_119P88,  0, 297000,  3003 },
    /* 73: RES_1080P120 */            {    1920,         1080,         2200,  1125,    88,          44,         148,          4,          5,         36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_120,     0, 297000,  3000 },
    /* 74: RES_1080P8991 */            {    1920,         1080,         2200,  1125,    88,          44,         148,          4,          5,         36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_89P91,     0, 222750,  4004 },
    /* 75: RES_1080P90 */            {     1920,          1080,         2200,  1125,    88,          44,         148,          4,          5,         36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_90,     0, 222750,  4000 },

    /* 76: RES_4Kx2K2398_SMPTE*/      {    4096,         2160,         5500,   2250,   1020,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_23P98,   0, 296703, 15015 },
    /* 77: RES_4Kx2K25_SMPTE*/        {    4096,         2160,         5280,   2250,    968,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_25,      0, 297000, 14400 },
    /* 78: RES_4Kx2K2997_SMPTE*/      {    4096,         2160,         4400,   2250,     88,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_29P97,   0, 296703, 12012 },
    /* 79: RES_4Kx2K30_SMPTE*/        {    4096,         2160,         4400,   2250,     88,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_30,      0, 297000, 12000 },
    /* 80: RES_4Kx2K50_SMPTE*/        {    4096,         2160,         5280,   2250,    968,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_50,      0, 594000,  7200 },
    /* 81: RES_4Kx2K5994_SMPTE*/      {    4096,         2160,         4400,   2250,     88,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 593406,  6006 },
    /* 82: RES_4Kx2K60_SMPTE*/        {    4096,         2160,         4400,   2250,    176,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 594000,  6000 },

    //Timings for HDMITX
    /* 83: RES_4Kx2K50_420*/          {    3840,         2160,         5280,   2250,   1056,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_50,      0, 297000,  7200 },
    /* 84: RES_4Kx2K5994_420*/        {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 296703,  6006 },
    /* 85: RES_4Kx2K60_420*/          {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 297000,  6000 },
    /* 86: RES_4Kx2K2398_3D*/         {    3840,         2160,         5500,   2250,   1276,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_47P96,   1, 593406, 15015 },
    /* 87: RES_4Kx2K24_3D */          {    3840,         2160,         5500,   2250,   1276,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_48,      1, 594000, 15000 },
    /* 88: RES_4Kx2K25_3D */          {    3840,         2160,         5280,   2250,   1056,        88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_50,      1, 594000, 14400 },
    /* 89: RES_4Kx2K2997_3D */        {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_59P94,   1, 593406, 12012 },
    /* 90: RES_4Kx2K30_3D */          {    3840,         2160,         4400,   2250,   176,         88,         296,          8,         10,         72,   TYPE_UHD,  SCAN_PROGRESSIVE, FRAME_RATE_60,      1, 594000, 12000 },

    /* 91: RES_LVDS_1088P60 */        {    1932,         1088,         2200,   1125,     60,        44,         110,          4,         5,          36,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 148500,  6000 },
    /* 92: RES_LVDS_1366x768P60 */    {    1366,          768,         1500,    825,     48,        32,          54,          10,         5,         42,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0,  74250,  6000 },
    /* 93: RES_LVDS_1366x768P5994 */  {    1366,          768,         1500,    825,     48,        32,          54,          10,         5,         42,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0,  74176,  6006 },
    /* 94: RES_LVDS_1366x768P50 */    {    1366,          768,         1800,    825,     48,        32,         354,          10,         5,         42,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_50,      0,  74250,  7200 },
    /* 95: RES_LVDS_1366x768P48 */    {    1366,          768,         1875,    825,     48,        32,         429,          10,         5,         42,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_48,      0,  74250,  7500 },
    /* 96: RES_LVDS_1366x768P4796 */  {    1366,          768,         1875,    825,     48,        32,         429,          10,         5,         42,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_47P96,   0,  74176,  7507 },
    /* 97. RES_DSI_540x960P60 */      {     540,          960,          825,   1220,     40,         5,         240,         245,         2,         13, TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0,  60667, 6000 },
    /* 98. RES_DSI_1920x1200P60 */    {    1920,         1200,         2044,   1224,     37,        25,          62,           2,         4,         18,     TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0, 150000, 6000 },
    /* 99. RES_DSI_800x1280P60 */     {     800,         1280,          882,   1327,     40,        20,          22,          20,         4,         23,     TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0,  70220, 6000 },
    /*100: RES_DSI_1080P5994  */      {    1920,         1080,         2080,   1111,     48,        32,          80,           3,         5,         23,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_59P94,   0, 138500, 6006 },
    /*101. RES_DSI_WNC_800x1280P60 */ {     800,         1280,          950,   1312,     60,        30,          60,          16,         2,         14,   TYPE_HD,   SCAN_PROGRESSIVE, FRAME_RATE_60,      0,  74784, 6000 },
    /* RES_RESET   */ {       0,            0,            1,      1,     0,          0,           0,        0,           0,           0,         0,                0, 0,    0 }
};

static int gcd(int m, int n) {
    if(m < n) {
        int t;
        t = m; m = n; n = t;
    }
    if(n == 0) return m;
    return gcd(n, m % n);
}

void video_cfg_num_den(INT resID, int * num, int * den) {
    int n = 0, d = 0;
    if(resID >= 0 && resID < MAX_NUM_RESS) {
        n = 27000;
        d = m_resinfo_table[resID].freq;
        if (m_resinfo_table[resID].freq <= 27027)
        {
            if (m_resinfo_table[resID].freq == 25200)
              d = 100801;   //25200.175*4
            else
              d *=4;
        }
        else if (m_resinfo_table[resID].freq >= 148000)
        {
            d *= 1;
        }
        else
        {
            d *= 2;
        }
        if(d > 0) {
            int g = gcd(d, n);
            if(g > 1) {
                d /= g; n /= g;
            }
        }
    }
    *num = n;
    *den = d;
}
int video_framerate_num_den(INT resID, int * num, int * den) {
    int fram_rate;

    if((resID >= FIRST_RES) && (resID < MAX_NUM_RESS))
        fram_rate = m_resinfo_table[resID].frame_rate;
    else
    {
        *num = 0;
        *den = 0;
        return -1;
    }
    if(fram_rate == FRAME_RATE_23P98){
        *num = 24000;
        *den = 1001;
    }else if(fram_rate == FRAME_RATE_24){
        *num = 24000;
        *den = 1000;
    }else if(fram_rate == FRAME_RATE_25){
        *num = 25000;
        *den = 1000;
    }else if(fram_rate == FRAME_RATE_29P97){
        *num = 30000;
        *den = 1001;
    }else if(fram_rate == FRAME_RATE_30){
        *num = 30000;
        *den = 1000;
    }else if(fram_rate == FRAME_RATE_47P96){
        *num = 48000;
        *den = 1001;
    }else if(fram_rate == FRAME_RATE_48){
        *num = 48000;
        *den = 1000;
    }else if(fram_rate == FRAME_RATE_50){
        *num = 50000;
        *den = 1000;
    }else if(fram_rate == FRAME_RATE_59P94){
        *num = 60000;
        *den = 1001;
    }else if(fram_rate == FRAME_RATE_60){
        *num = 60000;
        *den = 1000;
    }else if(fram_rate == FRAME_RATE_100){
        *num = 100000;
        *den = 1000;
    }else if(fram_rate == FRAME_RATE_119P88){
        *num = 120000;
        *den = 1001;
    }else if(fram_rate == FRAME_RATE_120){
        *num = 120000;
        *den = 1000;
    }else if(fram_rate == FRAME_RATE_89P91){
        *num = 90000;
        *den = 1001;
    }else if(fram_rate == FRAME_RATE_90){
        *num = 90000;
        *den = 1000;
    }else{
        return -1;
    }
    return 0;
}

