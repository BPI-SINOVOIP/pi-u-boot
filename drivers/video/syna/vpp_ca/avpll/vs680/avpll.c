/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Shanmugam Ramachandran <sramacha@synaptics.com>
 *
 */

#include "vpp_api.h"
#include "avpll.h"
#include "vpp_cfg.h"
#include "diag_pll.h"

#ifdef CONFIG_VPP_ENABLE_AVPLL
#ifndef CONFIG_VPP_ENABLE_AVPLL_LOCAL
static struct clk *vpp_clk;
#endif

typedef enum {
    VPP_CLK_ID_AVIOSYSCLK,
    VPP_CLK_ID_VPPSYSCLK,
    VPP_CLK_ID_BIUCLK,
    VPP_CLK_ID_VPPPIPECLK,
    VPP_CLK_ID_OVPCLK,
    VPP_CLK_ID_FPLL400CLK,
    VPP_CLK_ID_TXESCCLK,
    VPP_CLK_ID_VCLK0,
    VPP_CLK_ID_DPICLK,
} VPP_CLK_ID;

typedef struct __SetClockFreq_Data_ {
    int pllsrc;
    int isInterlaced;
    int refresh_rate;
    int h_active;
    int v_active;
} SetClockFreq_Data;

typedef struct __PPL_CONFIG__ {
    float field_rate;
    int frame_rate_enum;
    int h_active;
    int v_active;
    int isInterlaced;
    int h_total;
    int v_total;
    float video_freq;
    float pixel_freq;
    int Dm;
    int Dn;
    float dummy;
    double frac;
    float vco_freq;
    int Dp;
    float vpll_freq;
    int vclk_div;
    float vclk;
}PPL_CONFIG;

PPL_CONFIG vpll_cfg[] = {
    { 23.98, FRAME_RATE_23P98, 1280, 720, SCAN_PROGRESSIVE, 3300, 750, 23.98, 59.340659, 1, 20, 0.362637363, 6084045, 2136.263736, 36, 59.34065934, 1, 59.340659},
    { 23.98, FRAME_RATE_23P98, 1920, 1080, SCAN_PROGRESSIVE, 2750, 1125, 23.98, 74.175824, 1, 16, 0.802197802, 13458645, 1780.21978, 24, 74.17582418, 1, 74.175824},
    { 23.98, FRAME_RATE_23P98, 3840, 2160, SCAN_PROGRESSIVE, 5500, 2250, 23.98, 296.703297, 1, 16, 0.802197802, 13458645, 1780.21978, 6, 296.7032967, 1, 296.703297},
    { 24.00, FRAME_RATE_24, 1280, 720, SCAN_PROGRESSIVE, 3300, 750, 24.00, 59.400000, 1, 20, 0.384, 6442450, 2138.4, 36, 59.4, 1, 59.400000},
    { 24.00, FRAME_RATE_24, 1920, 1080, SCAN_PROGRESSIVE, 2750, 1125, 24.00, 74.250000, 1, 16, 0.82, 13757317, 1782, 24, 74.25, 1, 74.250000},
    { 24.00, FRAME_RATE_24, 3840, 2160, SCAN_PROGRESSIVE, 5500, 2250, 24.00, 297.000000, 1, 16, 0.82, 13757317, 1782, 6, 297, 1, 297.000000},
    { 29.97, FRAME_RATE_29P97, 1280, 720, SCAN_PROGRESSIVE, 3300, 750, 29.97, 74.175824, 1, 16, 0.802197802, 13458645, 1780.21978, 24, 74.17582418, 1, 74.175824},
    { 29.97, FRAME_RATE_29P97, 1920, 1080, SCAN_PROGRESSIVE, 2200, 1125, 29.97, 74.175824, 1, 16, 0.802197802, 13458645, 1780.21978, 24, 74.17582418, 1, 74.175824},
    { 29.97, FRAME_RATE_29P97, 3840, 2160, SCAN_PROGRESSIVE, 4400, 2250, 29.97, 296.703297, 1, 16, 0.802197802, 13458645, 1780.21978, 6, 296.7032967, 1, 296.703297},
    { 30.00, FRAME_RATE_30, 1280, 720, SCAN_PROGRESSIVE, 3300, 750, 30.00, 74.250000, 1, 16, 0.82, 13757317, 1782, 24, 74.25, 1, 74.250000},
    { 30.00, FRAME_RATE_30, 1920, 1080, SCAN_PROGRESSIVE, 2200, 1125, 30.00, 74.250000, 1, 16, 0.82, 13757317, 1782, 24, 74.25, 1, 74.250000},
    { 30.00, FRAME_RATE_30, 3840, 2160, SCAN_PROGRESSIVE, 4400, 2250, 30.00, 297.000000, 4, 296, 0, 0, 5940, 9, 297, 1, 297.000000},
    { 50.00, FRAME_RATE_50, 720, 576, SCAN_PROGRESSIVE, 864, 625, 50.00, 27.000000, 1, 11, 0.96, 16106127, 1296, 48, 27, 1, 27.000000},
    { 50.00, FRAME_RATE_50, 1280, 720, SCAN_PROGRESSIVE, 1980, 750, 50.00, 74.250000, 1, 16, 0.82, 13757317, 1782, 24, 74.25, 1, 74.250000},
    { 50.00, FRAME_RATE_50, 1920, 1080, SCAN_INTERLACED, 2640, 1125, 50.00, 74.250000, 1, 16, 0.82, 13757317, 1782, 24, 74.25, 1, 74.250000},
    { 50.00, FRAME_RATE_50, 1920, 1080, SCAN_PROGRESSIVE, 2640, 1125, 50.00, 148.500000, 1, 16, 0.82, 13757317, 1782, 12, 148.5, 1, 148.500000},
    { 50.00, FRAME_RATE_50, 3840, 2160, SCAN_PROGRESSIVE, 5280, 2250, 50.00, 594.000000, 1, 34, 0.64, 10737418, 3564, 6, 594, 1, 594.000000},
    { 59.94, FRAME_RATE_59P94, 640, 480, SCAN_PROGRESSIVE, 800, 525, 59.94, 25.174825, 1, 11, 0.083916084, 1407878, 1208.391608, 48, 25.17482517, 1, 25.174825},
    { 59.94, FRAME_RATE_59P94, 720, 480, SCAN_PROGRESSIVE, 858, 525, 59.94, 27.000000, 1, 11, 0.96, 16106127, 1296, 48, 27, 1, 27.000000},
    { 59.94, FRAME_RATE_59P94, 1280, 720, SCAN_PROGRESSIVE, 1650, 750, 59.94, 74.175824, 1, 16, 0.802197802, 13458645, 1780.21978, 24, 74.17582418, 1, 74.175824},
    { 59.94, FRAME_RATE_59P94, 1920, 1080, SCAN_INTERLACED, 2200, 1125, 59.94, 74.175824, 1, 16, 0.802197802, 13458645, 1780.21978, 24, 74.17582418, 1, 74.175824},
    { 59.94, FRAME_RATE_59P94, 1920, 1080, SCAN_PROGRESSIVE, 2200, 1125, 59.94, 148.351648, 1, 16, 0.802197802, 13458645, 1780.21978, 12, 148.3516484, 1, 148.351648},
    { 59.94, FRAME_RATE_59P94, 3840, 2160, SCAN_PROGRESSIVE, 4400, 2250, 59.94, 593.406593, 1, 34, 0.604395604, 10140075, 3560.43956, 6, 593.4065934, 1, 593.406593},
    { 60.00, FRAME_RATE_60, 640, 480, SCAN_PROGRESSIVE, 800, 525, 60.00, 25.200000, 1, 11, 0.096, 1610612, 1209.6, 48, 25.2, 1, 25.200000},
    { 60.00, FRAME_RATE_60, 720, 480, SCAN_PROGRESSIVE, 858, 525, 60.00, 27.027000, 1, 11, 0.97296, 16323560, 1297.296, 48, 27.027, 1, 27.027000},
    { 60.00, FRAME_RATE_60, 1280, 720, SCAN_PROGRESSIVE, 1650, 750, 60.00, 74.250000, 3, 171, 0.82, 4362076, 1782, 28, 74.25, 1, 74.250000},
    { 60.00, FRAME_RATE_60, 1920, 1080, SCAN_INTERLACED, 2200, 1125, 60.00, 74.250000, 3, 171, 0.82, 4362076, 1782, 28, 74.25, 1, 74.250000},
    { 60.00, FRAME_RATE_60, 1920, 1080, SCAN_PROGRESSIVE, 2200, 1125, 60.00, 148.500000, 4, 296, 0, 0, 5940, 19, 148.5, 1, 148.500000},
    { 60.00, FRAME_RATE_60, 3840, 2160, SCAN_PROGRESSIVE, 4400, 2250, 60.00, 594.000000, 4, 296, 0, 0, 5940, 4, 594, 1, 594.000000},
    { 60.00, FRAME_RATE_60,  800, 1280, SCAN_PROGRESSIVE,  950, 1312, 60.00, 74.7840000, 0,  12, 0.46112,  7736309, 1346.112, 8,  74.784000, 1,  74.784000},
    { 60.00, FRAME_RATE_60,  800, 1280, SCAN_PROGRESSIVE,  882, 1327, 60.00, 70.2200000, 0,  11, 0.63960, 10730707, 1263.960, 8,  70.220000, 1,  70.220000},
    { 60.00, FRAME_RATE_60, 1920, 1080, SCAN_PROGRESSIVE, 2080, 1111, 60.00, 138.500000, 0,  12, 0.85000, 14260633, 1385.000, 4, 138.500000, 1, 138.500000},
};

static void AVPLL_SetClockFreq_Pack(int *p_packed_data, SetClockFreq_Data *p_data) {
    p_packed_data[0] = p_data->pllsrc;
    p_packed_data[1] = p_data->isInterlaced << 8 | p_data->refresh_rate;
    p_packed_data[2] = p_data->h_active << 16 | p_data->v_active;
    p_packed_data[3] = 0;
}

//unpack in CA-AVPLL and pass it to get_pll_cfg() to get corresponding APLLCFG_t
static void AVPLL_SetClockFreq_UnPack(int *p_packed_data, SetClockFreq_Data *p_data) {
    p_data->pllsrc = p_packed_data[0];
    p_data->isInterlaced = p_packed_data[1] >> 8;
    p_data->refresh_rate = p_packed_data[1] & 0xFF;
    p_data->h_active = p_packed_data[2] >> 16;
    p_data->v_active = p_packed_data[2] & 0xFFFF;
}

static int get_pll_cfg(SetClockFreq_Data *p_data, APLLCFG_t *p_apll_cfg) {
    int i,n;
    n = sizeof(vpll_cfg) / sizeof(PPL_CONFIG);

    for(i=0;i<n;i++) {
        if((p_data->isInterlaced == vpll_cfg[i].isInterlaced) &&
                (p_data->refresh_rate == vpll_cfg[i].frame_rate_enum) &&
                (p_data->h_active == vpll_cfg[i].h_active) &&
                (p_data->v_active == vpll_cfg[i].v_active) ) {
                p_apll_cfg->dm = vpll_cfg[i].Dm;
                p_apll_cfg->dn = vpll_cfg[i].Dn;
                p_apll_cfg->frac = vpll_cfg[i].frac;
                p_apll_cfg->dp = vpll_cfg[i].Dp;
                p_apll_cfg->dp1 = 0;
                return 1;
        }
    }
    return 0;
}

int AVPLL_SetClockFreq(int avpllGroupId, int vco_freq_index, unsigned int target_freq, int chId) {
#ifdef CONFIG_VPP_ENABLE_AVPLL_LOCAL
    int packed_data[4];
    SetClockFreq_Data data;
    APLLCFG_t apll_cfg;

    packed_data[0] = avpllGroupId;
    packed_data[1] = vco_freq_index;
    packed_data[2] = target_freq;
    packed_data[3] = chId;

    //unpack in CA-AVPLL and pass it to get_pll_cfg() to get corresponding APLLCFG_t
    AVPLL_SetClockFreq_UnPack(&packed_data[0], &data);

    get_pll_cfg(&data, &apll_cfg);

    //show_SetClockFreq_Data(&data);
    //show_pll_cfg(&apll_cfg);
    diag_change_avpll(0+data.pllsrc, 0, 0, 0, 0, 0, 0, apll_cfg);
#else
    if(vpp_clk) {
        clk_set_rate(vpp_clk, target_freq);
    }
#endif
    return 0;
}


void MV_VPP_ProbeClk(struct udevice *dev, int channel_id)
{
#ifndef CONFIG_VPP_ENABLE_AVPLL_LOCAL
    VPP_CLK_ID clk_id;
    char *clk_name;
    int ret;

    if(channel_id == 1) {
        clk_id = VPP_CLK_ID_VCLK0;
        clk_name = "avio_vclk0";
    } else {
        clk_id = VPP_CLK_ID_DPICLK;
        clk_name = "avio_dpiclk";
    }

	vpp_clk = devm_clk_get(dev, clk_name);
    if (!IS_ERR(vpp_clk)) {
        ret = clk_prepare_enable(vpp_clk);
    } else {
        vpp_clk = NULL;
    }
#endif

}

void AVPLL_SetClockFreqForResID(int resID)
{
    SetClockFreq_Data clock_data;
    int packed_data[4];
    clock_data.pllsrc = 0;
    clock_data.isInterlaced = m_resinfo_table[resID].scan;
    clock_data.refresh_rate = m_resinfo_table[resID].frame_rate;
    clock_data.h_active = m_resinfo_table[resID].active_width;
    clock_data.v_active = m_resinfo_table[resID].active_height;
    AVPLL_SetClockFreq_Pack(&packed_data[0], &clock_data);
    AVPLL_SetClockFreq(packed_data[0], packed_data[1], packed_data[2], packed_data[3]);
}

#endif //CONFIG_VPP_ENABLE_AVPLL
