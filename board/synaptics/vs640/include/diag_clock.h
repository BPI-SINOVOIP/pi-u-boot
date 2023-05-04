/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 */
#ifndef _DIAG_CLOCK_H_
#define _DIAG_CLOCK_H_

enum Clocks
{
    CPUFASTREFCLK         ,  // 0
    MEMFASTREFCLK         ,  // 1
    CFGCLK                ,  // 2
    PERIFSYSCLK           ,  // 3
    ATBCLK                ,  // 4
    DECODERCLK            ,  // 5
    ENCODERCLK            ,  // 6
    OVPCORECLK            ,  // 7
    GFX3DCORECLK          ,  // 8
    GFX3DSYSCLK           ,  // 9
    AVIOSYSCLK            ,  // 10
    VPPSYSCLK             ,  // 11
    ESMCLK                ,  // 12
    AVIOBIUCLK            ,  // 13
    VIPPIPECLK            ,  // 14
    AVIOFPLL400_CLK       ,  // 15
    TSPCLK                ,  // 16
    TSPREFCLK             ,  // 17
    NOCSCLK               ,  // 18
    APBCORECLK            ,  // 19
    EMMCCLK               ,  // 20
    SD0CLK                ,  // 21
    USB3CORECLK           ,  // 22
    BCMCLK                ,  // 23
    NPUCLK                ,  // 24
    SISSSYSCLK            ,  // 25
    IFCPCLK               ,  // 26
    HDMIRXREFCLK          ,  // 27
    AIOSYSCLK             ,  // 28
    USIMCLK               ,  // 29
    NSKCLK                ,  // 30
    DSPSYSCLK             ,  // 31
    DSP0CLK               ,  // 32
    DSP1CLK               ,  // 33
    VNSYSCLK              ,  // 34
    USB2TESTCLK           ,  // 35
    USB2TESTCLK480MGROUP0 ,  // 36
    USB2TESTCLK480MGROUP1 ,  // 37
    USB2TESTCLK480MGROUP2 ,  // 38
    USB2TESTCLK100MGROUP0 ,  // 39
    USB2TESTCLK100MGROUP1 ,  // 40
    USB2TESTCLK100MGROUP2 ,  // 41
    USB2TESTCLK100MGROUP3 ,  // 42
    USB2TESTCLK100MGROUP4 ,  // 43
    PERIFTESTCLK50MGROUP0 ,  // 44
    PERIFTESTCLK125MGROUP0,  // 45
    PERIFTESTCLK200MGROUP0,  // 46
    PERIFTESTCLK200MGROUP1,  // 47
    PERIFTESTCLK250MGROUP0,  // 48
    PERIFTESTCLK500MGROUP0,  // 49
};

enum
{
	LIST_CPU_DDR_SPEEDS,
	LIST_ALL_SPEEDS
};

#define LIST_ALL_SPEEDS 1


extern int diag_clock_change_otherClk(unsigned int index, unsigned int
		pllSwitch, unsigned int pllSel, unsigned int divider, int en_print);
extern unsigned int get_divider(unsigned int D3Switch, unsigned int Switch,
		unsigned int Select);
extern void list_speed(int level);
extern int diag_clock_set_clocks(unsigned int set, unsigned int mask);
extern void diag_clock_list(void);

int diag_clock_get_otherClk(int index);

#define CLOCK_SET_T2        0
#define CLOCK_SET_VL        1
#define CLOCK_SET_VH        2


// ClkSel,0:cpupll_out, 1:cpupll_outF

int diag_clock_change_cpuClk(unsigned int index, unsigned int ClkSel, unsigned int divider, int en_print);

// USBOTG_REFCLK (20MHz) is sourced from SYSPLL2_F on VS680, it is from SYSPLL1_F on VS640 and AS470.
// for 50% duty, low_count = divval/2. divval should be even.
int diag_clock_USBOTG_REFCLK_div(int divval, int low_count);

#endif // _DIAG_CLOCK_H_
