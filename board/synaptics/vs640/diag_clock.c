/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 */

#include "diag_clock.h"
#include "Galois_memmap.h"
#include "global.h"
#include "cpu_wrp.h"
#include "diag_pll.h"
#include "mc_wrap.h"
#include "mc_defines.h"

#define ARRAY_NUM(a)            (sizeof(a)/sizeof(a[0]))
//#define ENABLE_REE_SET_SECURE_CLOCK

// updated for VS640, default from is SYSPLL0F, 600MHz
#define SYSPLL_NAME             "SYSPLL0F"             // for ClkPllSwitch = 0
#define SISS_SYSPLL_NAME        "SIPLLF"               // for ClkPllSwitch = 0, for SISS

static char *AVPLL_name[] =
{
    /* 0 */ "SYSPLL0",              // 500MHz
    /* 1 */ "SIPLL",                // 700MHz
    /* 2 */ "SYSPLL1",              // 700MHz
    /* 3 */ "AVPLL0",               //
    /* 4 */ "SYSPLL1F",             // 800MHz
    // others, "SYSPLL0F"           // 600MHz
};

static char *SISS_AVPLL_name[] =
{
    /* 0 */ "SYSPLL0",              // 500MHz
    /* 1 */ "SIPLL",                // 700MHz
    /* 2 */ "SYSPLL1",              // 700MHz
    /* 3 */ "AVPLL0",               //
    /* 4 */ "AVPLL1",               //
    // others, "SIPLLF"             // 600MHz
};

// index to current_freq[], which is defined in pllDiag.c. it takes the rule: n*2 + (F?1:0), n: PLLn
//float current_freq[8] = {
//    /* 0 */ 500,    // SYSPLL0
//    /* 1 */ 600,    // SYSPLL0F
//    /* 2 */ 700,    // SYSPLL1
//    /* 3 */ 800,    // SYSPLL1F
//    /* 4 */ 700,    // SIPLL
//    /* 5 */ 600,    // SIPLLF
//    /* 6 */ 400,    // AVPLL0
//    /* 7 */ 400,    // AVPLL1
//};

const int SYSPLL_index = 1;
const int SISS_SYSPLL_index = 5;

const int AVPLL_index_for_table[] =
{
    0,  // "SYSPLL0"
    4,  // "SIPLL"
    2,  // "SYSPLL1"
    6,  // "AVPLL0"
    3,  // "SYSPLL1F"
};

const int SISS_AVPLL_index_for_table[] =
{
    0,  // "SYSPLL0"
    4,  // "SIPLL"
    2,  // "SYSPLL1"
    6,  // "AVPLL0"
    7,  // "AVPLL1"
};

struct g_aClock_info_t
{
    const char*     clock_name;
    unsigned int    reg_offset;
    unsigned int    source;
    unsigned int    div;
};

#define SRC_IDX_SYSPLL0_PLLOUTF     SRC_IDX_MAX                         // default SYSPLL0 PLLOUTF for CLKDx and SIPLLOUTF for sissCLKDx
#define SRC_IDX_SISS_PLLOUTF        SRC_IDX_MAX                         // default SYSPLL0 PLLOUTF for CLKDx and SIPLLOUTF for sissCLKDx
#define SRC_IDX_SYSPLL0_PLLOUT      clkD4_ctrl_ClkPllSel_CLKSRC0
#define SRC_IDX_SIPLL_PLLOUT        clkD4_ctrl_ClkPllSel_CLKSRC1
#define SRC_IDX_SYSPLL1_PLLOUT      clkD4_ctrl_ClkPllSel_CLKSRC2
#define SRC_IDX_AVPLL0              clkD4_ctrl_ClkPllSel_CLKSRC3
#define SRC_IDX_SYSPLL1_PLLOUTF     clkD4_ctrl_ClkPllSel_CLKSRC4
#define SRC_IDX_SISS_AVPLL1         clkD4_ctrl_ClkPllSel_CLKSRC4        // SISS uses a slight different index with non-SISS clocks.
#define SRC_IDX_MAX                 (SRC_IDX_SYSPLL1_PLLOUTF + 1)

#define DEF_ACLOCK_INFO(name,source,div) {#name, RA_Gbl_##name, SRC_IDX_##source, div}
const struct g_aClock_info_t g_aClocks[] =
{// VH clock (based on v3p0, , updated for decoderClk=900/gfx3DCoreClk=900/dspClk=900/npuClk=800/tspClk=800)
    DEF_ACLOCK_INFO(    cpufastRefClk         ,   /* 600 */ SYSPLL0_PLLOUT  ,   1),  // 0
    DEF_ACLOCK_INFO(    memfastRefClk         ,   /* 600 */ SYSPLL0_PLLOUT  ,   1),  // 1
    DEF_ACLOCK_INFO(    cfgClk                ,   /* 100 */ SYSPLL0_PLLOUT  ,   6),  // 2
    DEF_ACLOCK_INFO(    perifSysClk           ,   /* 300 */ SYSPLL0_PLLOUT  ,   2),  // 3
    DEF_ACLOCK_INFO(    atbClk                ,   /* 100 */ SYSPLL0_PLLOUT  ,   6),  // 4
    DEF_ACLOCK_INFO(    decoderClk            ,   /* 900 */ SYSPLL0_PLLOUTF ,   1),  // 5
    DEF_ACLOCK_INFO(    encoderClk            ,   /* 300 */ SYSPLL0_PLLOUT  ,   2),  // 6
    DEF_ACLOCK_INFO(    ovpCoreClk            ,   /* 400 */ SYSPLL1_PLLOUTF ,   2),  // 7
    DEF_ACLOCK_INFO(    gfx3DCoreClk          ,   /* 900 */ SYSPLL0_PLLOUTF ,   1),  // 8
    DEF_ACLOCK_INFO(    gfx3DSysClk           ,   /* 700 */ SIPLL_PLLOUT    ,   1),  // 9
    DEF_ACLOCK_INFO(    avioSysClk            ,   /* 400 */ SYSPLL1_PLLOUTF ,   2),  // 10
    DEF_ACLOCK_INFO(    vppSysClk             ,   /* 600 */ SYSPLL0_PLLOUT  ,   1),  // 11
    DEF_ACLOCK_INFO(    esmClk                ,   /* 100 */ SYSPLL0_PLLOUT  ,   6),  // 12
    DEF_ACLOCK_INFO(    avioBiuClk            ,   /* 250 */ SYSPLL1_PLLOUTF ,   2),  // 13
    DEF_ACLOCK_INFO(    vipPipeClk            ,   /* 600 */ SYSPLL0_PLLOUT  ,   1),  // 14
    DEF_ACLOCK_INFO(    avioFpll400_clk       ,   /* 400 */ SYSPLL1_PLLOUTF ,   2),  // 15
    DEF_ACLOCK_INFO(    apbCoreClk            ,   /* 200 */ SYSPLL0_PLLOUT  ,   3),  // 19
    DEF_ACLOCK_INFO(    emmcClk               ,   /* 200 */ SYSPLL0_PLLOUT  ,   3),  // 20
    DEF_ACLOCK_INFO(    sd0Clk                ,   /* 200 */ SYSPLL0_PLLOUT  ,   3),  // 21
    DEF_ACLOCK_INFO(    usb3CoreClk           ,   /* 400 */ SYSPLL1_PLLOUTF ,   2),  // 22
    DEF_ACLOCK_INFO(    npuClk                ,   /* 800 */ SYSPLL1_PLLOUTF ,   1),  // 24
    DEF_ACLOCK_INFO(    hdmiRxrefClk          ,   /* 250 */ SYSPLL1_PLLOUTF ,   2),  // 27
    DEF_ACLOCK_INFO(    aioSysClk             ,   /* 200 */ SYSPLL0_PLLOUT  ,   3),  // 28
    DEF_ACLOCK_INFO(    usimClk               ,   /* 350 */ SIPLL_PLLOUT    ,   2),  // 29
    DEF_ACLOCK_INFO(    dspSysClk             ,   /* 400 */ SYSPLL1_PLLOUTF ,   2),  // 31
    DEF_ACLOCK_INFO(    dsp0Clk               ,   /* 900 */ SYSPLL0_PLLOUTF ,   1),  // 32
    DEF_ACLOCK_INFO(    dsp1Clk               ,   /* 900 */ SYSPLL0_PLLOUTF ,   1),  // 33
    DEF_ACLOCK_INFO(    vnSysClk              ,   /* 700 */ SIPLL_PLLOUT    ,   1),  // 34
#ifdef ENABLE_REE_SET_SECURE_CLOCK
    DEF_ACLOCK_INFO(    tspClk                ,   /* 800 */ SIPLL_PLLOUTF   ,   1),  // 16
    DEF_ACLOCK_INFO(    tspRefClk             ,   /* 200 */ SIPLL_PLLOUTF   ,   4),  // 17
    DEF_ACLOCK_INFO(    nocsClk               ,   /* 200 */ SIPLL_PLLOUTF   ,   4),  // 18
    DEF_ACLOCK_INFO(    bcmClk                ,   /* 200 */ SIPLL_PLLOUTF   ,   4),  // 23
    DEF_ACLOCK_INFO(    sissSysClk            ,   /* 350 */ SIPLL_PLLOUT    ,   2),  // 25
    DEF_ACLOCK_INFO(    ifcpClk               ,   /* 200 */ SIPLL_PLLOUTF   ,   4),  // 26
    DEF_ACLOCK_INFO(    nskClk                ,   /* 200 */ SIPLL_PLLOUTF   ,   4),  // 30
#endif
};

//static const char* g_aClocks[] =
//{
//     "cpufastRefClk",           // 0
//     "memfastRefClk",           // 1
//     "cfgClk",                  // 2
//     "perifSysClk",             // 3
//     "atbClk",                  // 4
//     "decoderClk",              // 5
//     "encoderClk",              // 6
//     "ovpCoreClk",              // 7
//     "gfx3DCoreClk",            // 8
//     "gfx3DSysClk",             // 9
//     "avioSysClk",              // 10
//     "vppSysClk",               // 11
//     "esmClk",                  // 12
//     "avioBiuClk",              // 13
//     "vipPipeClk",              // 14
//     "avioFpll400_clk",         // 15
//     "tspClk",                  // 16
//     "tspRefClk",               // 17
//     "nocsClk",                 // 18
//     "apbCoreClk",              // 19
//     "emmcClk",                 // 20
//     "sd0Clk",                  // 21
//     "usb3CoreClk",             // 22
//     "bcmClk",                  // 23
//     "npuClk",                  // 24
//     "sissSysClk",              // 25
//     "ifcpClk",                 // 26
//     "hdmiRxrefClk",            // 27
//     "aioSysClk",               // 28
//     "usimClk",                 // 29
//     "nskClk",                  // 30
//     "dspSysClk",               // 31
//     "dsp0Clk",                 // 32
//     "dsp1Clk",                 // 33
//     "vnSysClk",                // 34
//     "usb2TestClk",             // 35
//     "usb2TestClk480mGroup0",   // 36
//     "usb2TestClk480mGroup1",   // 37
//     "usb2TestClk480mGroup2",   // 38
//     "usb2TestClk100mGroup0",   // 39
//     "usb2TestClk100mGroup1",   // 40
//     "usb2TestClk100mGroup2",   // 41
//     "usb2TestClk100mGroup3",   // 42
//     "usb2TestClk100mGroup4",   // 43
//     "perifTestClk50mGroup0",   // 44
//     "perifTestClk125mGroup0",  // 45
//     "perifTestClk200mGroup0",  // 46
//     "perifTestClk200mGroup1",  // 47
//     "perifTestClk250mGroup0",  // 48
//     "perifTestClk500mGroup0",  // 49
//};
//
//static const unsigned int g_aClkOffset[]=
//{
//    RA_Gbl_cpufastRefClk         ,  // 0
//    RA_Gbl_memfastRefClk         ,  // 1
//    RA_Gbl_cfgClk                ,  // 2
//    RA_Gbl_perifSysClk           ,  // 3
//    RA_Gbl_atbClk                ,  // 4
//    RA_Gbl_decoderClk            ,  // 5
//    RA_Gbl_encoderClk            ,  // 6
//    RA_Gbl_ovpCoreClk            ,  // 7
//    RA_Gbl_gfx3DCoreClk          ,  // 8
//    RA_Gbl_gfx3DSysClk           ,  // 9
//    RA_Gbl_avioSysClk            ,  // 10
//    RA_Gbl_vppSysClk             ,  // 11
//    RA_Gbl_esmClk                ,  // 12
//    RA_Gbl_avioBiuClk            ,  // 13
//    RA_Gbl_vipPipeClk            ,  // 14
//    RA_Gbl_avioFpll400_clk       ,  // 15
//    RA_Gbl_tspClk                ,  // 16
//    RA_Gbl_tspRefClk             ,  // 17
//    RA_Gbl_nocsClk               ,  // 18
//    RA_Gbl_apbCoreClk            ,  // 19
//    RA_Gbl_emmcClk               ,  // 20
//    RA_Gbl_sd0Clk                ,  // 21
//    RA_Gbl_usb3CoreClk           ,  // 22
//    RA_Gbl_bcmClk                ,  // 23
//    RA_Gbl_npuClk                ,  // 24
//    RA_Gbl_sissSysClk            ,  // 25
//    RA_Gbl_ifcpClk               ,  // 26
//    RA_Gbl_hdmiRxrefClk          ,  // 27
//    RA_Gbl_aioSysClk             ,  // 28
//    RA_Gbl_usimClk               ,  // 29
//    RA_Gbl_nskClk                ,  // 30
//    RA_Gbl_dspSysClk             ,  // 31
//    RA_Gbl_dsp0Clk               ,  // 32
//    RA_Gbl_dsp1Clk               ,  // 33
//    RA_Gbl_vnSysClk              ,  // 34
//    RA_Gbl_usb2TestClk           ,  // 35
//    RA_Gbl_usb2TestClk480mGroup0 ,  // 36
//    RA_Gbl_usb2TestClk480mGroup1 ,  // 37
//    RA_Gbl_usb2TestClk480mGroup2 ,  // 38
//    RA_Gbl_usb2TestClk100mGroup0 ,  // 39
//    RA_Gbl_usb2TestClk100mGroup1 ,  // 40
//    RA_Gbl_usb2TestClk100mGroup2 ,  // 41
//    RA_Gbl_usb2TestClk100mGroup3 ,  // 42
//    RA_Gbl_usb2TestClk100mGroup4 ,  // 43
//    RA_Gbl_perifTestClk50mGroup0 ,  // 44
//    RA_Gbl_perifTestClk125mGroup0,  // 45
//    RA_Gbl_perifTestClk200mGroup0,  // 46
//    RA_Gbl_perifTestClk200mGroup1,  // 47
//    RA_Gbl_perifTestClk250mGroup0,  // 48
//    RA_Gbl_perifTestClk500mGroup0,  // 49
//};


const unsigned int clock_divider[] =
{
    1,
    2,
    4,
    6,
    8,
    12,
    1,
    1
};

int speed_cpu;
unsigned int RefClkIn = 25;
unsigned int RefDiv = 2;
float current_freq[8];
float cpupll_freqs[2];

static int is_sissclk(int clock_index)
{
    if (clock_index == SISSSYSCLK   ||
        clock_index == IFCPCLK      ||
        clock_index == BCMCLK       ||
        clock_index == TSPCLK       ||
        clock_index == TSPREFCLK    ||
        clock_index == NOCSCLK      ||
        clock_index == NSKCLK
        )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


static float get_syspll_freq(int clock_index)
{
    int pll_index;

    if (is_sissclk(clock_index))
    {
        pll_index = SISS_SYSPLL_index;
    }
    else
    {
        pll_index = SYSPLL_index;
    }

    return current_freq[pll_index];
}

static char* get_syspll_name(int clock_index)
{
    //int pll_index;

    if (is_sissclk(clock_index))
    {
        return SISS_SYSPLL_NAME;
    }
    else
    {
        return SYSPLL_NAME;
    }
}

static float get_avpll_freq(int clock_index, int pll_sel)
{
    int pll_index;

    if (is_sissclk(clock_index))
    {
        pll_index = SISS_AVPLL_index_for_table[pll_sel];
    }
    else
    {
        pll_index = AVPLL_index_for_table[pll_sel];
    }

    return current_freq[pll_index];
}

static char* get_avpll_name(int clock_index, int pll_sel)
{
    if (is_sissclk(clock_index))
    {
        return SISS_AVPLL_name[pll_sel];
    }
    else
    {
        return AVPLL_name[pll_sel];
    }
}


int diag_clock_change_otherClk(unsigned int index, unsigned int pllSwitch, unsigned int pllSelect, unsigned int divider, int en_print)
{
//    TGbl_ClkSwitch    ClkSwitch;
    TclkD1_ctrl    clkDx;

#if (PLATFORM == FPGA)
    debug( "Warning: real clock is not changed for FPGA\n");
#endif

    if ((index >= ARRAY_NUM(g_aClocks)) || (pllSwitch > 1) || (pllSelect > 4))
    {
        debug( " invalid parameter!\n");
        return -1;
    }
    else if ((divider != 1) && (divider != 2) && (divider != 3) && (divider != 4) &&
            (divider != 6) && (divider != 8) && (divider != 12))
    {
        debug( " invalid divider!\n");
        return -1;
    }

//     1) program divider to 12
    SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset), &clkDx.u32[0]);
    clkDx.uctrl_ClkD3Switch = 0;
    clkDx.uctrl_ClkSwitch = 1;
    clkDx.uctrl_ClkSel = DIVIDED_BY_12;
    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset), clkDx.u32[0]);

//    2) switch to syspll by setting clkPllSwitch
    clkDx.uctrl_ClkPllSwitch = 0;
    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset), clkDx.u32[0]);

//    3) program clkPllSel (if target clock source is avpll or syspll_clkoutp)
    clkDx.uctrl_ClkPllSel = pllSelect;
    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset), clkDx.u32[0]);

//    4) switch to avpll/syspll_clkoutp by setting clkPllSwitch (if target clock source is avpll or syspll_clkoutp)
    clkDx.uctrl_ClkPllSwitch = pllSwitch;
    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset), clkDx.u32[0]);

//    5) program proper divider
//    SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch), &ClkSwitch.u32[0]);
    SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset), &clkDx.u32[0]);

    // sysPll bypass ON
//    ClkSwitch.uClkSwitch_sysPLLSWBypass = 1;
//    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch), ClkSwitch.u32[0]);

    // use default sysPll
//    clkDx.uctrl_ClkPllSwitch = 0;
//    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index]), clkDx.u32[0]);

    // change divider to divided-by-3 first
//    clkDx.uctrl_ClkD3Switch = 1;
//    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index]), clkDx.u32[0]);

    // change divider to target
    switch (divider)
    {
        case 1:
            clkDx.uctrl_ClkD3Switch = 0;
            clkDx.uctrl_ClkSwitch = 0;
            break;
        case 2:
            clkDx.uctrl_ClkD3Switch = 0;
            clkDx.uctrl_ClkSwitch = 1;
            clkDx.uctrl_ClkSel = DIVIDED_BY_2;
            break;
        case 3:
            clkDx.uctrl_ClkD3Switch = 1;
            break;
        case 4:
            clkDx.uctrl_ClkD3Switch = 0;
            clkDx.uctrl_ClkSwitch = 1;
            clkDx.uctrl_ClkSel = DIVIDED_BY_4;
            break;
        case 6:
            clkDx.uctrl_ClkD3Switch = 0;
            clkDx.uctrl_ClkSwitch = 1;
            clkDx.uctrl_ClkSel = DIVIDED_BY_6;
            break;
        case 8:
            clkDx.uctrl_ClkD3Switch = 0;
            clkDx.uctrl_ClkSwitch = 1;
            clkDx.uctrl_ClkSel = DIVIDED_BY_8;
            break;
        case 12:
            clkDx.uctrl_ClkD3Switch = 0;
            clkDx.uctrl_ClkSwitch = 1;
            clkDx.uctrl_ClkSel = DIVIDED_BY_12;
            break;
        default:
            debug( " this is impossible\n");
            break;
    }
    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset), clkDx.u32[0]);

    // turn off divided-by-3 if not divided by 3
//    if (divider != 3)
//    {
//        clkDx.uctrl_ClkD3Switch = 0;
//        SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index]), clkDx.u32[0]);
//    }

    // change Pll Switch
//    clkDx.uctrl_ClkPllSwitch = pllSwitch;
//    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index]), clkDx.u32[0]);

    // bypass OFF
//    ClkSwitch.uClkSwitch_sysPLLSWBypass = 0;
//    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch), ClkSwitch.u32[0]);

     if (en_print) debug( "  Changed %s, now\n", g_aClocks[index].clock_name);

    return 0;
}

void diag_clock_change_otherClk_pllSwitch(unsigned int index, unsigned int pllSwitch)
{
//    unsigned int address;
//    unsigned int old_value;
//    unsigned int new_vlaue;
//    TclkD1_ctrl    clkDx;
//
//#if (PLATFORM == FPGA)
//    debug( "Warning: real clock is not changed for FPGA\n");
//#endif
//
//    address = MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index];
//    SYNA_CLK_READ32(address, &old_value);
//    clkDx.u32[0] = old_value;
//    clkDx.uctrl_ClkPllSwitch = pllSwitch;
//    new_vlaue = clkDx.u32[0];
//    SYNA_CLK_WRITE32(address, clkDx.u32[0]);
//
//     debug( "  Changed %s[0x%08x], old: 0x%08x, new: 0x%08x\n", g_aClocks[index], address, old_value, new_vlaue);
}

void diag_clock_change_otherClk_pllSelect(unsigned int index, unsigned int pllSelect)
{
//    unsigned int address;
//    unsigned int old_value;
//    unsigned int new_vlaue;
//    TclkD1_ctrl    clkDx;
//
//#if (PLATFORM == FPGA)
//    debug( "Warning: real clock is not changed for FPGA\n");
//#endif
//
//    address = MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index];
//    SYNA_CLK_READ32(address, &old_value);
//    clkDx.u32[0] = old_value;
//    clkDx.uctrl_ClkPllSel = pllSelect;
//    new_vlaue = clkDx.u32[0];
//    SYNA_CLK_WRITE32(address, clkDx.u32[0]);
//
//     debug( "  Changed %s[0x%08x], old: 0x%08x, new: 0x%08x\n", g_aClocks[index], address, old_value, new_vlaue);
}

void diag_clock_change_otherClk_divider(unsigned int index, unsigned int divider)
{
//    unsigned int address;
//    unsigned int old_value;
//    unsigned int new_vlaue;
//    TclkD1_ctrl    clkDx;
//
//#if (PLATFORM == FPGA)
//    debug( "Warning: real clock is not changed for FPGA\n");
//#endif
//
//    address = MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index];
//    SYNA_CLK_READ32(address, &old_value);
//    clkDx.u32[0] = old_value;
//
//    switch (divider)
//    {
//        case 1:
//            clkDx.uctrl_ClkD3Switch = 0;
//            clkDx.uctrl_ClkSwitch = 0;
//            break;
//        case 2:
//            clkDx.uctrl_ClkD3Switch = 0;
//            clkDx.uctrl_ClkSwitch = 1;
//            clkDx.uctrl_ClkSel = DIVIDED_BY_2;
//            break;
//        case 3:
//            clkDx.uctrl_ClkD3Switch = 1;
//            break;
//        case 4:
//            clkDx.uctrl_ClkD3Switch = 0;
//            clkDx.uctrl_ClkSwitch = 1;
//            clkDx.uctrl_ClkSel = DIVIDED_BY_4;
//            break;
//        case 6:
//            clkDx.uctrl_ClkD3Switch = 0;
//            clkDx.uctrl_ClkSwitch = 1;
//            clkDx.uctrl_ClkSel = DIVIDED_BY_6;
//            break;
//        case 8:
//            clkDx.uctrl_ClkD3Switch = 0;
//            clkDx.uctrl_ClkSwitch = 1;
//            clkDx.uctrl_ClkSel = DIVIDED_BY_8;
//            break;
//        case 12:
//            clkDx.uctrl_ClkD3Switch = 0;
//            clkDx.uctrl_ClkSwitch = 1;
//            clkDx.uctrl_ClkSel = DIVIDED_BY_12;
//            break;
//        default:
//            debug( " this is impossible\n");
//            break;
//    }
//
//    new_vlaue = clkDx.u32[0];
//    SYNA_CLK_WRITE32(address, clkDx.u32[0]);
//
//     debug( "  Changed %s[0x%08x], old: 0x%08x, new: 0x%08x\n", g_aClocks[index], address, old_value, new_vlaue);
}

unsigned int get_divider(unsigned int D3Switch, unsigned int Switch, unsigned int Select)
{
    unsigned int divider;
    if (D3Switch)
        divider = 3;
    else
    {
        if (!Switch)
            divider = 1;
        else
            divider = clock_divider[Select];
    }
    return divider;
}

const char* cpu_clks_name[] =
{
    "cpu0",
    "cpu1",
    "cpu2",
    "cpu3",
    "dsu",
};

void list_cpu_speed(void)
{
    int index;

    unsigned int    reg_cpuClkSel;
    unsigned int    reg_cpuClkSwitch;
    unsigned int    reg_cpuClkD3Switch;
    unsigned int    reg_cpuClkDivSel;

    SYNA_CLK_READ32(MEMMAP_CA7_REG_BASE + RA_CPU_WRP_CPU_REG + RA_CPU_REG_cpuClkSel,         &reg_cpuClkSel);
    SYNA_CLK_READ32(MEMMAP_CA7_REG_BASE + RA_CPU_WRP_CPU_REG + RA_CPU_REG_cpuClkSwitch,      &reg_cpuClkSwitch);
    SYNA_CLK_READ32(MEMMAP_CA7_REG_BASE + RA_CPU_WRP_CPU_REG + RA_CPU_REG_cpuClkD3Switch,    &reg_cpuClkD3Switch);
    SYNA_CLK_READ32(MEMMAP_CA7_REG_BASE + RA_CPU_WRP_CPU_REG + RA_CPU_REG_cpuClkDivSel,      &reg_cpuClkDivSel);

    for (index = 0; index < ARRAY_NUM(cpu_clks_name); index++)
    {
        int divider;
        int D3Switch = (reg_cpuClkD3Switch>>index) & 1;
        int Switch = (reg_cpuClkSwitch>>index) & 1;
        int Select = (reg_cpuClkDivSel>>(4*index)) & 0xf;
        int ClkSel = (reg_cpuClkSel>>index) & 1;

        divider = get_divider(D3Switch, Switch, Select);

        if (index != 4)  ClkSel = !ClkSel;

        debug( " %-32sfrequency %4.0f [%s/%d]\n",
                cpu_clks_name[index], cpupll_freqs[ClkSel]/divider, ClkSel?"CPUPLL_OUTF":"CPUPLL_OUT", divider);

        if (index == 0)
        {
            speed_cpu = cpupll_freqs[ClkSel]/divider;   // use core0 freq for global cpu speed
        }
    }
}

void list_speed(int level)
{
    //unsigned int FbDiv;
    //unsigned int RefDiv;
    //unsigned int vcoDiv_sel_setting;
    //unsigned int vcoDiv_sel_setting_diff;
    //pll_ctrl    pllCtl;;
    //unsigned int memPll;
    //unsigned int cpuPll;
    //unsigned int sysPll_diff_p;
    unsigned int divider;
    unsigned int D3Switch, Switch, Select;
    unsigned int PllSel;
    //unsigned int read;
    TclkD1_ctrl    clkDx;
    //unsigned int sysPll_vco;
    unsigned int clken;


    //DIAG_ASSERT(ARRAY_NUM(g_aClocks)==ARRAY_NUM(g_aClkOffset));

#if PLATFORM == ASIC
    CLOCKO_t sysclock0 = diag_get_syspll0();
    CLOCKO_t sysclock1 = diag_get_syspll1();
#ifdef ENABLE_REE_SET_SECURE_CLOCK
    CLOCKO_t sipll = diag_get_sipll();
#endif
    CLOCKO_t cpuclock = diag_get_cpupll();
    CLOCKO_t memclock = diag_get_mempll();
    CLOCKO_t avpll0clock = diag_get_avpll(0);
    CLOCKO_t avpll1clock = diag_get_avpll(0);

    // update global pll freq table here
    current_freq[0] = sysclock0.clocko;
    current_freq[1] = sysclock0.clocko1;
    current_freq[2] = sysclock1.clocko;
    current_freq[3] = sysclock1.clocko1;
#ifdef ENABLE_REE_SET_SECURE_CLOCK
    current_freq[4] = sipll.clocko;
    current_freq[5] = sipll.clocko1;
#endif
    current_freq[6] = avpll0clock.clocko;
    current_freq[7] = avpll1clock.clocko;

    cpupll_freqs[0] = cpuclock.clocko;
    cpupll_freqs[1] = cpuclock.clocko1;

    // memPll
	debug( " %-32sfrequency %d\n", "memPLL", memclock.clocko);

    // sysPll
	debug( " %-32sfrequency %d\n", "sysPLL0", sysclock0.clocko);

    // sysPll clkoutp
	debug( " %-32sfrequency %d\n", "sysPLL0_CLKO1", sysclock0.clocko1);

    // sysPll
	debug( " %-32sfrequency %d\n", "sysPLL1", sysclock1.clocko);

    // sysPll clkoutp
	debug( " %-32sfrequency %d\n", "sysPLL1_CLKO1", sysclock1.clocko1);
#ifdef ENABLE_REE_SET_SECURE_CLOCK
    // siPll
	debug( " %-32sfrequency %d\n", "siPLL", sipll.clocko);

    // siPll clkoutp
	debug( " %-32sfrequency %d\n", "siPLL_CLKO1", sipll.clocko1);
#endif
    // cpuPll
	debug( " %-32sfrequency %d\n", "cpuPLL", cpuclock.clocko);
	debug( " %-32sfrequency %d\n", "cpuPLLO1", cpuclock.clocko1);

    // avPll0
	debug( " %-32sfrequency %d\n", "AVPLL0", avpll0clock.clocko);

    // avPll1
	debug( " %-32sfrequency %d\n", "AVPLL1", avpll1clock.clocko);

    // DDR
	debug( " %-32sfrequency %d\n", "dclk", memclock.clocko);
#endif

    // memPll
    //SYNA_CLK_READ32((MEMPLL_CTRL_BASE + RA_pll_ctrl), &pllCtl.u32[0]);
    //SYNA_CLK_READ32((MEMPLL_CTRL_BASE + RA_pll_ctrl1), &pllCtl.u32[1]);
    //FbDiv = pllCtl.uctrl_FBDIV;
    //RefDiv = pllCtl.uctrl_REFDIV;
    //vcoDiv_sel_setting = pllCtl.uctrl_CLKOUT_DIFF_DIV_SEL;
    //memPll = FbDiv * 4 * RefClkIn / RefDiv;
    //memPll = (float)memPll / vcoDiv_sel_setting;
    //debug( " %-32sfrequency %d\n", "memPLL", memPll);

    // sysPll
    //SYNA_CLK_READ32((SYSPLL_CTRL_BASE + RA_pll_ctrl), &pllCtl.u32[0]);
    //SYNA_CLK_READ32((SYSPLL_CTRL_BASE + RA_pll_ctrl1), &pllCtl.u32[1]);
    //FbDiv = pllCtl.uctrl_FBDIV;
    //RefDiv = pllCtl.uctrl_REFDIV;
    //vcoDiv_sel_setting = pllCtl.uctrl_CLKOUT_SE_DIV_SEL;
    //sysPll_vco = FbDiv * 4 * RefClkIn / RefDiv;
    //sysPll = (float)sysPll_vco / vcoDiv_sel_setting;
    //debug( " %-32sfrequency %d\n", "sysPLL", sysPll);

    // sysPll clkoutp
    //SYNA_CLK_READ32((SYSPLL_CTRL_BASE + RA_pll_ctrl), &pllCtl.u32[0]);
    //SYNA_CLK_READ32((SYSPLL_CTRL_BASE + RA_pll_ctrl1), &pllCtl.u32[1]);
    //FbDiv = pllCtl.uctrl_FBDIV;
    //RefDiv = pllCtl.uctrl_REFDIV;
    //vcoDiv_sel_setting = pllCtl.uctrl_CLKOUT_DIFF_DIV_SEL;
    //sysPll_vco = FbDiv * 4 * RefClkIn / RefDiv;
    //sysPll_diff_p = (float)sysPll_vco / vcoDiv_sel_setting;
    //debug( " %-32sfrequency %d\n", "sysPLL_clkoutp", sysPll_diff_p);

    // cpuPll
    //SYNA_CLK_READ32((CPUPLL_CTRL_BASE + RA_pll_ctrl), &pllCtl.u32[0]);
    //SYNA_CLK_READ32((CPUPLL_CTRL_BASE + RA_pll_ctrl1), &pllCtl.u32[1]);
    //FbDiv = pllCtl.uctrl_FBDIV;
    //RefDiv = pllCtl.uctrl_REFDIV;
    //vcoDiv_sel_setting = pllCtl.uctrl_CLKOUT_SE_DIV_SEL;
    //cpuPll = FbDiv * 4 * RefClkIn / RefDiv;
    //cpuPll = (float)cpuPll / vcoDiv_sel_setting;
    //debug( " %-32sfrequency %d\n", "cpuPLL", cpuPll);

    //SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch), &clkSwitch.u32);
    //SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch1), &ClkSwitch.u32[1]);
    //SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_clkSelect), &clkSelect.u32[0]);
    //SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_clkSelect1), &clkSelect.u32[1]);
    //SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_clkSelect2), &clkSelect.u32[2]);

    // DDR
    //SYNA_CLK_READ32((MEMMAP_MCTRLSS_REG_BASE + RA_MC6Ctrl_MC6_4TO1), &read);
    //if (read & 1)        // 4 to 1
    //    debug( " %-32sfrequency %d\n", "dclk", memPll / 4);
    //else                // 2 to 1
    //    debug( " %-32sfrequency %d\n", "dclk", memPll / 2);

    // CPU
    list_cpu_speed();

    if (level == LIST_ALL_SPEEDS)
    {
        int i;
        for (i = 0; i < ARRAY_NUM(g_aClocks); i++)
        {
            float source_freq;
            char* source_name;

            SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[i].reg_offset), &clkDx.u32[0]);
            clken = clkDx.uctrl_ClkEn;
            D3Switch = clkDx.uctrl_ClkD3Switch;
            Switch = clkDx.uctrl_ClkSwitch;
            Select = clkDx.uctrl_ClkSel;
            divider = get_divider(D3Switch, Switch, Select);
            PllSel = clkDx.uctrl_ClkPllSel;
            debug( " 0x%X: 0x%X: clken %d, D3Switch %d, Switch %d, Select %d, divider %d, PllSel %d\n",
                                    MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[i].reg_offset, clkDx.u32[0], clken, D3Switch, Switch, Select, divider, PllSel);
            if (clkDx.uctrl_ClkPllSwitch)
            {
                if (PllSel < ARRAY_NUM(AVPLL_index_for_table))
                {
                    source_freq = (float)get_avpll_freq(i, PllSel);
                    source_name = get_avpll_name(i, PllSel);
                }
                else
                {
                    source_freq = -1;
                    source_name = "Unknown";
                }
            }
            else
            {
                source_freq = (float)get_syspll_freq(i);
                source_name = get_syspll_name(i);
            }

        	debug( " %-32sfrequency %.0f [%s/%d]\n",
                	g_aClocks[i].clock_name, (float)source_freq / divider * clken, source_name, divider);
        }
    }
}

void diag_clock_list(void)
{
    int i;
    for (i = 0; i < ARRAY_NUM(g_aClocks); i++)
    {
        debug( "%-2d: %s\n", i, g_aClocks[i].clock_name);
    }

    debug( "\nPLL index list:\n");
    for (i = 0; i < ARRAY_NUM(AVPLL_name); i++)
    {
        debug( " %d: %s (%4.0fMHz)\n", i, AVPLL_name[i], current_freq[AVPLL_index_for_table[i]]);
        if (i == 4) // special for SISS CLKSRC4
        {
            debug( "*%d: %s (%4.0fMHz)\n", i, SISS_AVPLL_name[i], current_freq[SISS_AVPLL_index_for_table[i]]);
        }
    }

    debug( " default: %s (%4.0fMHz)\n", SYSPLL_NAME, current_freq[SYSPLL_index]);
    debug( "*default: %s (%4.0fMHz)\n", SISS_SYSPLL_NAME, current_freq[SISS_SYSPLL_index]);
}

int diag_clock_get_otherClk(int index)
{
    unsigned int divider;
    unsigned int D3Switch, Switch, Select;
    unsigned int PllSel;
    //unsigned int read;
    TclkD1_ctrl    clkDx;
    int clock_out_freq; // MHz

    if (index >= ARRAY_NUM(g_aClocks))
    {
        debug( "invalid index:%d\n", index);
        return -1;
    }

    SYNA_CLK_READ32((MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset), &clkDx.u32[0]);
    D3Switch = clkDx.uctrl_ClkD3Switch;
    Switch = clkDx.uctrl_ClkSwitch;
    Select = clkDx.uctrl_ClkSel;
    divider = get_divider(D3Switch, Switch, Select);
    PllSel = clkDx.uctrl_ClkPllSel;

    debug( " 0x%X: 0x%X: D3Switch %d, Switch %d, Select %d, divider %d, PllSel %d\n",
                            MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset, clkDx.u32[0], D3Switch, Switch, Select, divider, PllSel);
    if (clkDx.uctrl_ClkPllSwitch)
    {
        if (PllSel < ARRAY_NUM(AVPLL_index_for_table))
        {
            clock_out_freq = (int)get_avpll_freq(index, PllSel) / divider;
        }
        else
        {
            clock_out_freq = -1;
        }
    }
    else
    {
        int sysPll = (int)get_syspll_freq(index);
        clock_out_freq = sysPll / divider;
    }

    return clock_out_freq;
}

int diag_clock_USBOTG_REFCLK_div(int divval, int low_count)
{
    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_USBOTG_REFCLK_CTRL0), divval);
    SYNA_CLK_WRITE32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_USBOTG_REFCLK_CTRL1), low_count);

    debug( " USBOTG refclk div (0x%08x):%d, low_count (0x%08x):%d\n",
        (MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_USBOTG_REFCLK_CTRL0), divval,
        (MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_USBOTG_REFCLK_CTRL1), low_count);

    return 0;
}

int diag_clock_set_clocks(unsigned int set, unsigned int mask)
{
    unsigned int i;

#if PLATFORM == ASIC
    // ;cpupll=1800
    // sendln 'CLOCK CPUPLL 1350 1800'
    // wait 'BerlinDiag>'
    //
    // ; SYSPLL0=500, SYSPLL0F=600
    // sendln 'CLOCK SYSPLL 0 500 600'
    // wait 'BerlinDiag>'
    //
    // ; SYSPLL1=700, SYSPLL1F=800
    // sendln 'CLOCK SYSPLL 1 700 800'
    // wait 'BerlinDiag>'
    //
    // ; SIPLL=700, SIPLLF=600
    // sendln 'CLOCK SIPLL 700 600'
    // wait 'BerlinDiag>'

    if (mask & 1)   //cpupll
    {
        diag_change_cpupll(1350, 1800, 0, 0, 0, 1);
        speed_cpu = diag_get_cpupll().clocko;
    }
    if (mask & 2)   //syspll0
    {
        diag_change_syspll0(600, 900, 0, 0, 0, 1);
    }
    if (mask & 4)   //syspll1
    {
        diag_change_syspll1(500, 800, 0, 0, 0, 1);
        diag_clock_USBOTG_REFCLK_div(800/20, 800/20/2);
    }
#ifdef ENABLE_REE_SET_SECURE_CLOCK
    if (mask & 8)   //sipll
    {
        diag_change_sipll(700, 800, 0, 0, 0, 1);
    }
#endif
#endif

    switch (set)
    {
        case CLOCK_SET_T2:
            for (i=0; i<ARRAY_NUM(g_aClocks); i++)
            {
                diag_clock_change_otherClk(i, g_aClocks[i].source<SRC_IDX_MAX, g_aClocks[i].source<SRC_IDX_MAX?g_aClocks[i].source:0x4, g_aClocks[i].div, 0);
            }
            break;
        case CLOCK_SET_VL:
            //TODO
            debug( "not supported\n");
            break;
        case CLOCK_SET_VH:
            for (i=0; i<ARRAY_NUM(g_aClocks); i++)
            {
                diag_clock_change_otherClk(i, g_aClocks[i].source<SRC_IDX_MAX, g_aClocks[i].source<SRC_IDX_MAX?g_aClocks[i].source:0x4, g_aClocks[i].div, 0);
            }
            break;
    }
    return 0;
}
