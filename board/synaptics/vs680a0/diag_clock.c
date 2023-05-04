/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
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

const char *SYSPLLs[] =
{
	"SYSPLL2",
	"SYSPLL0F",
	"SYSPLL1F",
	"SYSPLL2F",
	"SYSPLL1",
};

const int AVPLL_index_for_table[] =     // n*2 + (F?1:0)
{
	4,
	1,
	3,
	5,
	2,
};

struct g_aClock_info_t
{
    char            *clock_name;
    unsigned int    reg_offset;
    unsigned int    source;
    unsigned int    div;
};

#define SRC_IDX_SYSPLL0_PLLOUT      SRC_IDX_MAX
#define SRC_IDX_SYSPLL2_PLLOUT      clkD4_ctrl_ClkPllSel_CLKSRC0
#define SRC_IDX_SYSPLL0_PLLOUTF     clkD4_ctrl_ClkPllSel_CLKSRC1
#define SRC_IDX_SYSPLL1_PLLOUTF     clkD4_ctrl_ClkPllSel_CLKSRC2
#define SRC_IDX_SYSPLL2_PLLOUTF     clkD4_ctrl_ClkPllSel_CLKSRC3
#define SRC_IDX_SYSPLL1_PLLOUT      clkD4_ctrl_ClkPllSel_CLKSRC4
#define SRC_IDX_MAX                 (SRC_IDX_SYSPLL1_PLLOUT + 1)

#define DEF_ACLOCK_INFO(name,source,div) {#name, RA_Gbl_##name, SRC_IDX_##source, div}
const struct g_aClock_info_t g_aClocks[] =
{   // T2 clock (based on v2p3)
    DEF_ACLOCK_INFO(    cpufastRefClk               ,   SYSPLL0_PLLOUTF     ,   1       ),  // 0
    DEF_ACLOCK_INFO(    memfastRefClk               ,   SYSPLL0_PLLOUTF     ,   1       ),  // 1
    DEF_ACLOCK_INFO(    cfgClk                      ,   SYSPLL0_PLLOUT      ,   12      ),  // 2
    DEF_ACLOCK_INFO(    perifSysClk                 ,   SYSPLL0_PLLOUT      ,   3       ),  // 3
    DEF_ACLOCK_INFO(    atbClk                      ,   SYSPLL0_PLLOUT      ,   12      ),  // 4
    DEF_ACLOCK_INFO(    decoderClk                  ,   SYSPLL2_PLLOUT      ,   1       ),  // 5
    DEF_ACLOCK_INFO(    encoderClk                  ,   SYSPLL1_PLLOUT      ,   1       ),  // 6
    DEF_ACLOCK_INFO(    ovpCoreClk                  ,   SYSPLL0_PLLOUT      ,   3       ),  // 7
    DEF_ACLOCK_INFO(    gfx3DCoreClk                ,   SYSPLL1_PLLOUTF     ,   1       ),  // 8
    DEF_ACLOCK_INFO(    gfx3DSysClk                 ,   SYSPLL1_PLLOUTF     ,   1       ),  // 9
    DEF_ACLOCK_INFO(    avioSysClk                  ,   SYSPLL0_PLLOUT      ,   2       ),  // 10
    DEF_ACLOCK_INFO(    vppSysClk                   ,   SYSPLL0_PLLOUT      ,   2       ),  // 11
    DEF_ACLOCK_INFO(    hpiClk                      ,   SYSPLL0_PLLOUT      ,   12      ),  // 12
    DEF_ACLOCK_INFO(    avioBiuClk                  ,   SYSPLL2_PLLOUT      ,   4       ),  // 13
    DEF_ACLOCK_INFO(    vipPipeClk                  ,   SYSPLL0_PLLOUT      ,   2       ),  // 14
    DEF_ACLOCK_INFO(    avioFpll400_clk             ,   SYSPLL0_PLLOUT      ,   3       ),  // 15
    DEF_ACLOCK_INFO(    tspClk                      ,   SYSPLL1_PLLOUTF     ,   1       ),  // 16
    DEF_ACLOCK_INFO(    tspRefClk                   ,   SYSPLL0_PLLOUT      ,   4       ),  // 17
    DEF_ACLOCK_INFO(    nocsClk                     ,   SYSPLL0_PLLOUT      ,   6       ),  // 18
    DEF_ACLOCK_INFO(    apbCoreClk                  ,   SYSPLL0_PLLOUT      ,   6       ),  // 19
    DEF_ACLOCK_INFO(    emmcClk                     ,   SYSPLL0_PLLOUT      ,   3       ),  // 20
    DEF_ACLOCK_INFO(    sd0Clk                      ,   SYSPLL0_PLLOUT      ,   12      ),  // 21
    DEF_ACLOCK_INFO(    gethRgmiiClk                ,   SYSPLL2_PLLOUT      ,   4       ),  // 22
    DEF_ACLOCK_INFO(    avioOvpClk                  ,   SYSPLL0_PLLOUTF     ,   1       ),  // 23
    DEF_ACLOCK_INFO(    perifTestClk125mGroup0      ,   SYSPLL2_PLLOUT      ,   8       ),  // 24
    DEF_ACLOCK_INFO(    usb2TestClk                 ,   SYSPLL0_PLLOUTF     ,   12      ),  // 25
    DEF_ACLOCK_INFO(    perifTestClk250mGroup0      ,   SYSPLL2_PLLOUTF     ,   4       ),  // 26
    DEF_ACLOCK_INFO(    usb3CoreClk                 ,   SYSPLL0_PLLOUT      ,   3       ),  // 27
    DEF_ACLOCK_INFO(    bcmClk                      ,   SYSPLL2_PLLOUT      ,   4       ),  // 28
    DEF_ACLOCK_INFO(    vxSysClk                    ,   SYSPLL1_PLLOUTF     ,   1       ),  // 29
    DEF_ACLOCK_INFO(    npuClk                      ,   SYSPLL0_PLLOUTF     ,   1       ),  // 30
    DEF_ACLOCK_INFO(    sissSysClk                  ,   SYSPLL0_PLLOUT      ,   3       ),  // 31
    DEF_ACLOCK_INFO(    ifcpClk                     ,   SYSPLL2_PLLOUT      ,   4       ),  // 32
    DEF_ACLOCK_INFO(    issSysClk                   ,   SYSPLL1_PLLOUT      ,   2       ),  // 33
    DEF_ACLOCK_INFO(    ispClk                      ,   SYSPLL0_PLLOUT      ,   2       ),  // 34
    DEF_ACLOCK_INFO(    ispBeClk                    ,   SYSPLL0_PLLOUT      ,   3       ),  // 35
    DEF_ACLOCK_INFO(    ispDscClk                   ,   SYSPLL0_PLLOUTF     ,   1       ),  // 36
    DEF_ACLOCK_INFO(    ispCsi0Clk                  ,   SYSPLL0_PLLOUT      ,   2       ),  // 37
    DEF_ACLOCK_INFO(    ispCsi1Clk                  ,   SYSPLL0_PLLOUT      ,   4       ),  // 38
    DEF_ACLOCK_INFO(    hdmiRxrefClk                ,   SYSPLL2_PLLOUT      ,   4       ),  // 39
    DEF_ACLOCK_INFO(    mipiRxScanByteClk           ,   SYSPLL0_PLLOUT      ,   4       ),  // 40
    DEF_ACLOCK_INFO(    usb2TestClk480mGroup0       ,   SYSPLL2_PLLOUT      ,   2       ),  // 41
    DEF_ACLOCK_INFO(    usb2TestClk480mGroup1       ,   SYSPLL2_PLLOUT      ,   2       ),  // 42
    DEF_ACLOCK_INFO(    usb2TestClk480mGroup2       ,   SYSPLL2_PLLOUT      ,   2       ),  // 43
    DEF_ACLOCK_INFO(    usb2TestClk100mGroup0       ,   SYSPLL0_PLLOUT      ,   12      ),  // 44
    DEF_ACLOCK_INFO(    usb2TestClk100mGroup1       ,   SYSPLL0_PLLOUT      ,   12      ),  // 45
    DEF_ACLOCK_INFO(    usb2TestClk100mGroup2       ,   SYSPLL0_PLLOUT      ,   12      ),  // 46
    DEF_ACLOCK_INFO(    usb2TestClk100mGroup3       ,   SYSPLL0_PLLOUT      ,   12      ),  // 47
    DEF_ACLOCK_INFO(    usb2TestClk100mGroup4       ,   SYSPLL0_PLLOUT      ,   12      ),  // 48
    DEF_ACLOCK_INFO(    perifTestClk200mGroup0      ,   SYSPLL0_PLLOUT      ,   6       ),  // 49
    DEF_ACLOCK_INFO(    perifTestClk200mGroup1      ,   SYSPLL0_PLLOUT      ,   6       ),  // 50
    DEF_ACLOCK_INFO(    perifTestClk500mGroup0      ,   SYSPLL2_PLLOUT      ,   2       ),  // 51
    DEF_ACLOCK_INFO(    txClkEsc                    ,   SYSPLL0_PLLOUT      ,   6       ),  // 52
    DEF_ACLOCK_INFO(    aioSysClk                   ,   SYSPLL0_PLLOUTF     ,   2       ),  // 53
};

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

int diag_clock_change_otherClk(unsigned int index, unsigned int pllSwitch, unsigned int pllSelect, unsigned int divider, int en_print)
{
//	TGbl_ClkSwitch	ClkSwitch;
	TclkD1_ctrl	clkDx;

    if ((index >= (unsigned int)ARRAY_NUM(g_aClocks)) || (pllSwitch > 1) || (pllSelect > 4))
    {
        debug(" invalid parameter!\n");
        return -1;
    }
    else if ((divider != 1) && (divider != 2) && (divider != 3) && (divider != 4) &&
            (divider != 6) && (divider != 8) && (divider != 12))
    {
        debug(" invalid divider!\n");
        return -1;
    }

// 	1) program divider to 12
	clkDx.u32[0] = readl((long)(MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset));
	clkDx.uctrl_ClkD3Switch = 0;
	clkDx.uctrl_ClkSwitch = 1;
	clkDx.uctrl_ClkSel = DIVIDED_BY_12;
	writel(clkDx.u32[0], (long)(MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset));

//	2) switch to syspll by setting clkPllSwitch
	clkDx.uctrl_ClkPllSwitch = 0;
	writel(clkDx.u32[0], (long)(MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset));

//	3) program clkPllSel (if target clock source is avpll or syspll_clkoutp)
	clkDx.uctrl_ClkPllSel = pllSelect;
	writel(clkDx.u32[0], (long)(MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset));

//	4) switch to avpll/syspll_clkoutp by setting clkPllSwitch (if target clock source is avpll or syspll_clkoutp)
	clkDx.uctrl_ClkPllSwitch = pllSwitch;
	writel(clkDx.u32[0], (long)(MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset));

//	5) program proper divider
//	readl((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch), &ClkSwitch.u32[0]);
	clkDx.u32[0] = readl((long)(MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset));

	// sysPll bypass ON
//	ClkSwitch.uClkSwitch_sysPLLSWBypass = 1;
//	writel((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch), ClkSwitch.u32[0]);

	// use default sysPll
//	clkDx.uctrl_ClkPllSwitch = 0;
//	writel((MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index]), clkDx.u32[0]);

	// change divider to divided-by-3 first
//	clkDx.uctrl_ClkD3Switch = 1;
//	writel((MEMMAP_CHIP_CTRL_REG_BASE + g_aClkOffset[index]), clkDx.u32[0]);

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
			debug(" this is impossible\n");
			break;
	}
	writel(clkDx.u32[0], (long)(MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[index].reg_offset));

 	if (en_print)
 		debug("  Changed %s, now\n", g_aClocks[index].clock_name);

    return 0;
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

void list_speed(int level)
{
    CLOCKO_t sysclock0 = diag_get_syspll0();
    CLOCKO_t sysclock1 = diag_get_syspll1();
    CLOCKO_t sysclock2 = diag_get_syspll2();
    CLOCKO_t cpuclock = diag_get_cpupll();
    CLOCKO_t memclock = diag_get_mempll();

	// memPll
	debug(" %-32sfrequency %d\n", "memPLL", memclock.clocko);

	// sysPll0
	debug(" %-32sfrequency %d\n", "sys0PLL", sysclock0.clocko);

	// sysPll0_outputf
	debug(" %-32sfrequency %d\n", "sys0PLL_OUTPUTF", sysclock0.clocko1);

	// sysPll1
	debug(" %-32sfrequency %d\n", "sys1PLL", sysclock1.clocko);

	// sysPll1_outputf
	debug(" %-32sfrequency %d\n", "sys1PLL_OUTPUTF", sysclock1.clocko1);

	// sysPll2
	debug(" %-32sfrequency %d\n", "sys2PLL", sysclock2.clocko);

	// sysPll2_outputf
	debug(" %-32sfrequency %d\n", "sys2PLL_OUTPUTF", sysclock2.clocko1);

	// cpuPll
	debug(" %-32sfrequency %d\n", "cpuPLL", cpuclock.clocko);

	// DDR
	debug(" %-32sfrequency %d\n", "dclk", memclock.clocko);

    //unsigned int sysPll_diff_p;
    unsigned int divider;
    unsigned int D3Switch, Switch, Select;
    unsigned int PllSel;
    TclkD1_ctrl	clkDx;
    unsigned int clken;

    float read_freq[6] = {
	    sysclock0.clocko,   	// SYSPLL0
	    sysclock0.clocko1,   	// SYSPLL0F
	    sysclock1.clocko,   	// SYSPLL1
	    sysclock1.clocko1,   	// SYSPLL1F
	    sysclock2.clocko,   	// SYSPLL2
	    sysclock2.clocko1,   	// SYSPLL2F
    };


    if (level == LIST_ALL_SPEEDS)
    {
        int i;
        for (i = 0; i < (int)ARRAY_NUM(g_aClocks); i++)
        {
        	clkDx.u32[0] = readl((long)(MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[i].reg_offset));
        	clken = clkDx.uctrl_ClkEn;
        	D3Switch = clkDx.uctrl_ClkD3Switch;
        	Switch = clkDx.uctrl_ClkSwitch;
        	Select = clkDx.uctrl_ClkSel;
        	divider = get_divider(D3Switch, Switch, Select);
        	PllSel = clkDx.uctrl_ClkPllSel;
        	debug(" 0x%X: 0x%X: clken %d, D3Switch %d, Switch %d, Select %d, divider %d, PllSel %d\n",
        							MEMMAP_CHIP_CTRL_REG_BASE + g_aClocks[i].reg_offset, clkDx.u32[0], clken, D3Switch, Switch, Select, divider, PllSel);
        	if (clkDx.uctrl_ClkPllSwitch)
        	{
        		if (PllSel < ARRAY_NUM(AVPLL_index_for_table))
        			debug(" %-32sfrequency %4.0f [%s]\n",
        					g_aClocks[i].clock_name, read_freq[AVPLL_index_for_table[PllSel]] / divider * clken, SYSPLLs[PllSel]);
        		else if (PllSel < ARRAY_NUM(SYSPLLs))
        			debug(" %-32sfrequency %d [%s]\n",
        					g_aClocks[i].clock_name, sysclock0.clocko / divider * clken, SYSPLLs[PllSel]);
        	}
        	else	// sysPll (single ended output)
        	{
        		debug(" %-32sfrequency %4d\n", g_aClocks[i].clock_name, sysclock0.clocko / divider * clken);
        	}
        }
    }
}

int diag_clock_set_clocks(unsigned int set, unsigned int mask)
{
    unsigned int i;
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
            debug("not supported\n");
            break;
        case CLOCK_SET_VH:
            //TODO
            debug("not supported\n");
            break;
    }
    if (mask & 1)   //cpupll
    {
    diag_change_cpupll(1800, 0, 0, 0, 1);
    }
    if (mask & 2)   //syspll0
    {
    diag_change_syspll0(1200, 800, 0, 0, 0, 1);
    }
    if (mask & 4)   //syspll1
    {
    diag_change_syspll1(1050, 700, 0, 0, 0, 1);
    }
    if (mask & 8)   //syspll2
    {
    diag_change_syspll2(1000, 400, 0, 0, 0, 1);
    }
    return 0;
}
