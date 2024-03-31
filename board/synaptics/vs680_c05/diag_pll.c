/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#include <common.h>
#include <linux/delay.h>
#include "diag_pll.h"

#define A_F_V(a, f, v)		(a)

ssc_div_t divs[SSC_DIV_MAX] = {
    {32, 	0xF},
    {42, 	0xE},
    {50, 	0xD},
    {64, 	0xC},
    {84, 	0xB},
    {102, 	0xA},
    {128, 	0x9},
    {170, 	0x8},
    {204, 	0x7},
    {256, 	0x6},
    {340, 	0x5},
    {408, 	0x4},
    {512, 	0x3},
    {682, 	0x2},
    {818, 	0x1},
};

static void delay_us_real(unsigned int us)
{
	const unsigned int step_us = 1000000;
	const unsigned int step_ns = 1000 * step_us;
	while (us > step_us) {
		ndelay(step_ns);
		us -= step_us;
	}
	if (us)
		ndelay(1000 * us);
}

// positive inputs
int gcd(int a, int b)
{
    while(a!=b)
    {
        if(a>b)
            a-=b;
        else
            b-=a;
    }
    return a;
}

int lcm(int a, int b)
{
    return a*b/gcd(a,b);
}

int calc_vco(int freq, int freq1)
{
    int vco = lcm(freq, freq1);
    return vco;
}

int rounding(double frac)
{
    int int_part = (int)frac;
    if((frac-0.5)<(double)int_part)
        return int_part;
    else
        return int_part+1;
}

// d1.div < target <d2.div
int closest(ssc_div_t d1, ssc_div_t d2, int target){
    if((target-d1.div)>(d2.div-target))
	return d2.val;
    else
	return d1.val;
}

// binary search to find the closest
// input ssc_div_t in ascending order (of ssc_div_t->div)
int bsearch(ssc_div_t a[], int target){
    int l=0, r=SSC_DIV_MAX;
    int mid;

    // corner case;
    if(target<=a[0].div)
	return a[0].val;
    if(target>=a[SSC_DIV_MAX-1].div)
	return a[SSC_DIV_MAX].val;

    while(l<r){
	mid=l+(r-l)/2;
	if(a[mid].div==target){
	    return a[mid].val;
	}else if(target<a[mid].div){
	    // search the left handside;
	    // check if target in range [mid-1, mid], if so, return the closest;
	    if(mid>0 && target>a[mid-1].div){
		return closest(a[mid-1], a[mid], target);
	    }
	    r=mid;
	}else{
	    // search the right handside
	    // check if target in range [mid, mid+1], if sp, return the closest
	    if(mid<SSC_DIV_MAX-1 && target<a[mid+1].div){
		return closest(a[mid], a[mid+1], target);
	    }
	    l=mid+1;
	}
    }
    return a[mid].val;
}

int get_vco(unsigned int pllbase)
{
    double vco=0;
    int n_int, n_frac;
    int m;
    Tabipll_ctrlC pllctrlc;
    Tabipll_ctrlD pllctrld;
    Tabipll_ctrlE pllctrle;

    PLL_REG_READ(pllbase, 2, pllctrlc.u32[0]);
    PLL_REG_READ(pllbase, 3, pllctrld.u32[0]);
    PLL_REG_READ(pllbase, 4, pllctrle.u32[0]);
    PLL_REG_GET(pllctrlc, C_DIVR, 	m);
    PLL_REG_GET(pllctrld, D_DIVFI, 	n_int);
    PLL_REG_GET(pllctrle, E_DIVFF, 	n_frac);

    double nx = n_int +1 + (double)n_frac/(1<<24);
    // calculate VCO based on register value
    // per ABI SPEC page2, figure 1, there is a default, unchangeable /4 divider
    // to tne input of feedback divider module, so we need to muliply that back
    vco = FREF*nx*4/(m+1);

    return (int)vco;
}

CLOCKO_t get_clocko(unsigned int pllbase)
{
    int bypass;
    int output, outputf;
    Tabipll_ctrlA pllctrla;
    Tabipll_ctrlF pllctrlf;
    Tabipll_ctrlG pllctrlg;
    CLOCKO_t res;
    int vco=get_vco(pllbase);

    PLL_REG_READ(pllbase, 0, pllctrla.u32[0]);
    PLL_REG_GET(pllctrla, A_BYPASS, 	bypass);
    if(bypass)
    {
        res.clocko=FREF;
        res.clocko1=FREF;
    }
    else
    {
	PLL_REG_READ(pllbase, 5, pllctrlf.u32[0]);
	PLL_REG_READ(pllbase, 6, pllctrlg.u32[0]);

        PLL_REG_GET(pllctrlf, F_DIVQ, 		output);
        PLL_REG_GET(pllctrlg, G_DIVQF, 		outputf);

        res.clocko = vco/((output+1)*2);
        res.clocko1 = vco/(outputf+1);
    }
    return res;
}

unsigned int get_pll_base(E_PLL_SRC pll)
{
    unsigned int pll_base;
    switch(pll)
    {
    case DIAG_SYSPLL0:
        pll_base = MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_sysPll_0;
        break;
    case DIAG_SYSPLL1:
        pll_base = MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_sysPll_1;
        break;
    case DIAG_SYSPLL2:
        pll_base = MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_sysPll_2;
        break;
    case DIAG_CPUPLL:
        pll_base = MEMMAP_CA7_REG_BASE + RA_CPU_WRP_PLL_REG;
        break;
    case DIAG_MEMPLL:
        pll_base = MEMMAP_MCTRLSS_REG_BASE + RA_mc_wrap_memPll;
        break;
    case DIAG_VPLL0:
        pll_base = VPLLWRAP0VPLL;
        break;
    case DIAG_VPLL1:
        pll_base = VPLLWRAP1VPLL;
        break;
    case DIAG_APLL0:
        pll_base = APLLWRAP0APLL;
        break;
    case DIAG_APLL1:
        pll_base = APLLWRAP1APLL;
        break;
    default:
        pll_base=0xDEADBEEF;
        break;
    }

    return pll_base;
}

unsigned int get_avpll_ctrlbase(E_PLL_SRC pll)
{
    unsigned int pll_base;
    switch(pll)
    {
    case DIAG_VPLL0:
        pll_base = VPLLWRAP0VPLLCTRL;
        break;
    case DIAG_VPLL1:
        pll_base = VPLLWRAP1VPLLCTRL;
        break;
    case DIAG_APLL0:
        pll_base = APLLWRAP0APLLCTRL;
        break;
    case DIAG_APLL1:
        pll_base = APLLWRAP1APLLCTRL;
        break;
    default:
        pll_base=0xDEADBEEF;
        break;
    }

    return pll_base;
}
int diag_bypass_pll(unsigned int pllbase, int bypass)
{
    int ret=0;
    Tabipll_ctrlA pllctrla;

    PLL_REG_PRINT(pllbase, 0);

    PLL_REG_READ(pllbase, 0, pllctrla.u32[0]);
    PLL_REG_SET(pllctrla, A_BYPASS, bypass);
    PLL_REG_WRITE(pllbase, 0, pllctrla.u32[0]);

    PLL_REG_PRINT(pllbase, 0);

    return ret;
}

int diag_reset_pll(unsigned int pllbase, int reset)
{
    int ret=0;
    Tabipll_ctrlA pllctrla;

    PLL_REG_PRINT(pllbase, 0);

    PLL_REG_READ(pllbase, 0, pllctrla.u32[0]);
    PLL_REG_SET(pllctrla, A_RESET, reset);
    PLL_REG_WRITE(pllbase, 0, pllctrla.u32[0]);

    PLL_REG_PRINT(pllbase, 0);

    return ret;
}

// TB: APLL/VPLL will be implemented later
int diag_apllwrapper_power(E_PLL_SRC pll, int down)
{
    int ret=0;
    unsigned int val;
    // centralized avpll power control
    // convert apll index
    pll -= 5;

    // power PLL
    val = readl((long)(AVPLL_SWPD_ADDR));
    val &= ~(1<<pll);
    val |= down<<pll;
    writel(val, (long)(AVPLL_SWPD_ADDR));

    return ret;
}


int diag_bypass_pll_global(E_PLL_SRC pll, int bypass)
{
    int ret=0;
    unsigned int val;

    if(pll<=DIAG_CPUPLL)
    {
        val = readl((long)(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch));
        val &= ~(1<<pll);
        val |= bypass<<pll;
        writel(val, (long)(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_ClkSwitch));
    }
    else if(pll<=DIAG_APLL1)
    {
        // APLL global wrapper clkEn
        // per Dheeray Gupta's email @ 10/23, use PD registers as
	// AVPLL global bypass.
	ret = diag_apllwrapper_power(pll, bypass);
    }
    else
    {
        // invalid pll
        ret=-1;
    }

    return ret;
}

int diag_config_pll_vco(unsigned int pllbase, int refdiv, int fddiv, unsigned int frac)
{
    int ret=0;
    Tabipll_ctrlC pllctrlc;
    Tabipll_ctrlD pllctrld;
    Tabipll_ctrlE pllctrle;

    PLL_REG_PRINT(pllbase, 2);
    PLL_REG_PRINT(pllbase, 3);
    PLL_REG_PRINT(pllbase, 4);

    PLL_REG_READ(pllbase, 2, pllctrlc.u32[0]);
    PLL_REG_READ(pllbase, 3, pllctrld.u32[0]);
    PLL_REG_READ(pllbase, 4, pllctrle.u32[0]);

    PLL_REG_SET(pllctrlc, C_DIVR, 	refdiv);
    PLL_REG_SET(pllctrld, D_DIVFI, 	fddiv);
    PLL_REG_SET(pllctrle, E_DIVFF, 	frac);

    PLL_REG_WRITE(pllbase, 2, pllctrlc.u32[0]);
    PLL_REG_WRITE(pllbase, 3, pllctrld.u32[0]);
    PLL_REG_WRITE(pllbase, 4, pllctrle.u32[0]);

    PLL_REG_PRINT(pllbase, 2);
    PLL_REG_PRINT(pllbase, 3);
    PLL_REG_PRINT(pllbase, 4);

    return ret;
}

int diag_config_pll_feedback(unsigned int pllbase, int fddiv, unsigned int frac)
{
    int ret=0;
    Tabipll_ctrlD pllctrld;
    Tabipll_ctrlE pllctrle;

    PLL_REG_PRINT(pllbase, 3);
    PLL_REG_PRINT(pllbase, 4);

    PLL_REG_READ(pllbase, 3, pllctrld.u32[0]);
    PLL_REG_READ(pllbase, 4, pllctrle.u32[0]);

    PLL_REG_SET(pllctrld, D_DIVFI, 	fddiv);
    PLL_REG_SET(pllctrle, E_DIVFF, 	frac);

    PLL_REG_WRITE(pllbase, 3, pllctrld.u32[0]);
    PLL_REG_WRITE(pllbase, 4, pllctrle.u32[0]);

    PLL_REG_PRINT(pllbase, 3);
    PLL_REG_PRINT(pllbase, 4);

    return ret;
}

int diag_config_pll_dp(unsigned int pllbase, int dp, int dp1)
{
    int ret=0;
    Tabipll_ctrlF pllctrlf;
    Tabipll_ctrlG pllctrlg;

    PLL_REG_READ(pllbase, 5, pllctrlf.u32[0]);
    PLL_REG_READ(pllbase, 6, pllctrlg.u32[0]);

    PLL_REG_SET(pllctrlf, F_DIVQ, 		dp);
    PLL_REG_SET(pllctrlg, G_DIVQF, 		dp1);

    PLL_REG_WRITE(pllbase, 5, pllctrlf.u32[0]);
    PLL_REG_WRITE(pllbase, 6, pllctrlg.u32[0]);

    PLL_REG_PRINT(pllbase, 5);
    PLL_REG_PRINT(pllbase, 6);

    return ret;
}

int diag_config_pll_newdiv(unsigned int pllbase, int enable)
{
    int ret=0;
    Tabipll_ctrlA pllctrla;

    PLL_REG_PRINT(pllbase, 0);

    PLL_REG_READ(pllbase, 0, pllctrla.u32[0]);
    PLL_REG_SET(pllctrla, A_NEWDIV, enable);
    PLL_REG_WRITE(pllbase, 0, pllctrla.u32[0]);

    PLL_REG_PRINT(pllbase, 0);

    return ret;
}

// input here is A/VPLLWRAP0/1A/VPLLCTRL
int diag_config_pll_avpll_clken(unsigned int pllbase, int enable){
    int ret=0;
    unsigned int val;

    val = readl((long)(pllbase));
    val &= ~(1<<LSb32APLL_WRAP_APLL_CTRL_clkEn);
    val |= (enable<<LSb32APLL_WRAP_APLL_CTRL_clkEn);
    writel(val, (long)(pllbase));

    return ret;
}

double diag_config_pll_getfpfd(unsigned int pllbase){
    Tabipll_ctrlC pllctrlc;
    int divr;

    PLL_REG_PRINT(pllbase, 2);
    // get DIVR
    PLL_REG_READ(pllbase, 2, pllctrlc.u32[0]);
    PLL_REG_GET(pllctrlc, C_DIVR, 	divr);

    double fpfd=(double)FREF/(divr+1);
    return fpfd;
}

int diag_config_pll_getpfdrange(unsigned int pllbase){
    int r=0;
    double fpfd=diag_config_pll_getfpfd(pllbase);

    // 8 possible ranges
    if(fpfd>=5 && fpfd<7.5){
	r=0;
    }else if(fpfd>=7.5 && fpfd<11){
	r=1;
    }else if(fpfd>=11 && fpfd<18){
	r=2;
    }else if(fpfd>=18 && fpfd<30){
	r=3;
    }else if(fpfd>=30 && fpfd<50){
	r=4;
    }else if(fpfd>=50 && fpfd<80){
	r=5;
    }else if(fpfd>=80 && fpfd<130){
	r=6;
    }else if(fpfd>=130 && fpfd<=200){
	r=7;
    }else{
        debug("ILLEGAL input FPFD!!\n");
	r=0;
    }
    return r;
}

int diag_config_pll_setpfdrange(unsigned int pllbase)
{
    int ret=0;
    Tabipll_ctrlA pllctrla;

    int range=diag_config_pll_getpfdrange(pllbase);

    PLL_REG_PRINT(pllbase, 0);

    PLL_REG_READ(pllbase, 0, pllctrla.u32[0]);
    PLL_REG_SET(pllctrla, A_RANGE, range);
    PLL_REG_WRITE(pllbase, 0, pllctrla.u32[0]);

    PLL_REG_PRINT(pllbase, 0);

    return ret;
}


// TB: ABI PLL support both downspread and centerspread
// so add a new parameter
// Hint: need to double the input ssc_amp if the ssc_mode is centerspread
// since the SSC depth is peak to peak frequency distance.
int diag_config_pll_ssc(unsigned int pllbase, int ssc_freq, int ssc_amp, int ssc_mode, int enable)
{
    int ret=0, ssfreq, ssdepth, ssmode;
    int ssc_div;
    Tabipll_ctrlB pllctrlb;
    double wssc_amp = 0, fpfd, ssc_freqd;

    if(enable)
    {
        // input parameter validate
	// SSC only in fraction mode.
	// use 6.25Mhz Fpfd, which is 25MHz/4
        if(ssc_freq<8||ssc_freq>195||ssc_amp<0||ssc_amp>40)
            return -1;

        // SSC calculation
        // SSC freq range [6.11, 234] KHz
        // SSC amp range [0.2%, 4%] both downspread and centerspread

	// select ssc depth, input ssc_amp = actual_ssc_amp * 10, which is to
	// avoid using floating point input
	// invalid input checked previously, so all ssc_amp should be valid
	if(ssc_amp<=2){
	    ssdepth=0;
	    wssc_amp=0.2;
	}else if(ssc_amp<=5){
	    ssdepth=1;
	    wssc_amp=0.5;
	}else if(ssc_amp<=8){
	    ssdepth=2;
	    wssc_amp=0.8;
	}else if(ssc_amp<=10){
	    ssdepth=3;
	    wssc_amp=1.0;
	}else if(ssc_amp<=16){
	    ssdepth=4;
	    wssc_amp=1.6;
	}else if(ssc_amp<=20){
	    ssdepth=5;
	    wssc_amp=2.0;
	}else if(ssc_amp<=31){
	    ssdepth=6;
	    wssc_amp=3.1;
	}else if(ssc_amp<=40){
	    ssdepth=7;
	    wssc_amp=4.0;
	}

        debug("written SSC depth is: %.1f,  input SSC depth is: %.1f\n", wssc_amp, (double)ssc_amp/10);
	fpfd=diag_config_pll_getfpfd(pllbase);
	// per SPEC page 8, exmaple in Table5B, SSC_freq = Fpfd/SSMF[3:0]
	// ssc_freq input in KHz, so to transform to Mhz, we need to divide by
	// 1000
	ssc_freqd=(double)ssc_freq/1000;
	ssc_div=rounding(fpfd/ssc_freqd);
    debug("Calculated FPFD frequency is: %.2f\n", fpfd);
    debug("Calculated SSCD frequency is: %.2f\n", ssc_freqd);
    debug("Calculated SSC frequency is: %.2f\n", fpfd/ssc_div);
	// select SSMF base on SSC_DIV
	// binary search, closest
	ssfreq=bsearch(divs, ssc_div);
    debug("Table Searched SSMF is: %d\n", ssfreq);
	ssmode=ssc_mode;
    }else{
        ssfreq = 0;
        ssdepth= 0;
	ssmode = 0;
    }

    // update corresponding SSRATE and SLOPE register
    PLL_REG_PRINT(pllbase, 1);
    PLL_REG_READ(pllbase, 1, pllctrlb.u32[0]);

    PLL_REG_SET(pllctrlb, B_SSMF, 	ssfreq);
    PLL_REG_SET(pllctrlb, B_SSMD, 	ssdepth);
    PLL_REG_SET(pllctrlb, B_SSE, 	enable);
    PLL_REG_SET(pllctrlb, B_SSDS, 	ssmode);

    PLL_REG_WRITE(pllbase, 1, pllctrlb.u32[0]);

    PLL_REG_PRINT(pllbase, 1);

    return ret + ((int)wssc_amp ^ (int)wssc_amp);
}

int diag_wait_pll_lock(unsigned int pllbase)
{
    // wait long enough for PLL LOCK
    // SPEC doesn't speficy timeout time
    int ret=0, tout = A_F_V(3000, 30, 30);
    // get statue
    volatile Tabipll_status pllstatus;
    PLL_REG_PRINT(pllbase, 7);
    PLL_REG_READ(pllbase, 7, pllstatus.u32[0]);
    while(!pllstatus.ustatus_LOCK&&tout)
    {
        delay_us_real(1);
        PLL_REG_READ(pllbase, 7, pllstatus.u32[0]);
        tout--;
    }

    if(!tout)
        ret=-1;
    return ret;
}

int diag_wait_pll_divack(unsigned int pllbase, unsigned int level)
{
    // wait long enough for PLL LOCK
    // SPEC doesn't speficy timeout time
    int ret=0, tout = A_F_V(3000, 30, 30);
    // get statue
    volatile Tabipll_status pllstatus;
    PLL_REG_PRINT(pllbase, 7);
    PLL_REG_READ(pllbase, 7, pllstatus.u32[0]);
    while(!(pllstatus.ustatus_DIVACK==level)&&tout)
    {
        delay_us_real(1);
        PLL_REG_READ(pllbase, 7, pllstatus.u32[0]);
        tout--;
    }

    if(!tout)
        ret=-1;
    return ret;
}

int diag_check_feedback_only(unsigned int pllbase, unsigned int ndivr){
    double nfpfd=(double)FREF/(ndivr+1);
    double fpfd=diag_config_pll_getfpfd(pllbase);
    debug("Calculated FPFD is: %.2f, Current FPFD is: %.2f\n", nfpfd, fpfd);

    return nfpfd==fpfd;
}

int diag_change_feedback_otf(unsigned int pllbase, unsigned int ndivr, unsigned int divfi, unsigned int divff){
    int ret=0;
    if(diag_check_feedback_only(pllbase, ndivr)){
	// Fpfd not changed, apply handshaking feeback divider update sequence
	// 1. update DIVFI DIVFF register value, it won't be latched into PLL
	ret |= diag_config_pll_feedback(pllbase, divfi, divff);
	// 2. wait tNS, which is 1ns per SPEC p3, table1
	delay_us_real(1);
	// 3. assert newdiv signal
	ret |= diag_config_pll_newdiv(pllbase, ENABLE);
	// 4. wait DIVACK
	if(diag_wait_pll_divack(pllbase, ENABLE)){
            debug("WAIT for DIVACK ASSERT TIME OUT!\n");
	    ret=-1;
	}
	// 5. de-assert newdiv
	ret |= diag_config_pll_newdiv(pllbase, DISABLE);
	// 6. wait tDH, which is 1ns per SPEC p3, table1
	delay_us_real(1);
	// 7. wait divack been de-assert
	if(diag_wait_pll_divack(pllbase, DISABLE)){
            debug("WAIT for DIVACK DE-ASSERT TIME OUT!\n");
	    ret=-1;
	}
    }
    return ret;
}

// CLKO and CLKO1 as inputs
// integer freq change
int diag_change_pll(E_PLL_SRC pll, int freq, int freq1, int ssc, int ssc_freq,
        int ssc_amp, int ssc_mode, int calc, APLLCFG_t apll_cfg)
{
    int ret=0;
    // reference divider is 1 by default
    int dm=M1;
    // get PLL base address with checker
    unsigned int pllbase = get_pll_base(pll);
    int dp, dp1, INT_N, frac, vco=0, target_vco=0;
    int divr, divfi, divff, divq, divqf;
    double N=0.0, FRAC_N=0.0;

    if(pllbase == 0xDEADBEEF)
    {
        ret = -1;
        return ret;
    }

    // if calculate PLL settings inside code
    if(calc)
    {
        // calculate VCO first
        vco = calc_vco(freq, freq1);
        // check if vco is valid
        // SPEC defined VCO should be in [1200,6000]
        if(vco>6000)
        {
            debug("INVALID VCO: %d\n", vco);
            ret = -1;
            return ret;
        }

        // multiply by 2 to get a valid VCO
	target_vco=vco;
	int i=2;
	// make sure:
	// 1. VCO is greater than 1200
	// 2. DIVQ value must be even number, starts from 2 [2,4,6,8,10...]
        while(target_vco<1200 || (target_vco/freq)%2==1)
        {
            target_vco=vco*i;
	    i++;
        }
	vco=target_vco;

        // VCO is finalized now, check DM
	// Fpfd=Fref/DIVR
	// for integer mode: Fpfd [10, 200]
	// for fractional mode: Fpfd [5, 7.5]
	// Per SPEC, the higher the Fpfd, the better jitter scenario
	// so integer mode, use DIVR=1 [2 is another candidate]
	// fraction mode, use DIVR=6 [5 is another candidate]
	// all SSC mode are in fraction, so, use fraction mode DIVR if SSC is
	// intend to enable
        if(vco%25==0 && !ssc)
            dm=M1;
        else if((vco*10)%125==0 && !ssc)
            dm=M2;
        else
	    // change the fraction mode ref divider to 4
            dm=M4;

        // calculate dp
        dp = vco/freq;
	// since dp is between [2, 64], vco must be twice the freq of Fout
	if(dp==1)
	    vco*=2;
        if(vco>6000)
        {
            debug("INVALID VCO: %d\n", vco);
            ret = -1;
            return ret;
	}
	dp=vco/freq;
        dp1 = vco/freq1;
	// dp1 between [1:7]
	if(dp1>7 || dp>64 || dp1<1 || dp<2)
	{
            debug("INVALID DIVQ and DIVQF: %d %d\n", dp, dp1);
            ret = -1;
            return ret;
	}
	// ABI PLL doesn't have separte sequnce for update DIVR/DIVFI/DIVFF or
	// DIVQ/DIVQR only.
        // previous vco
        //pvco = get_vco(pllbase);

	// calculate VCO(CLK) M and N
	// in case fraction mode needed
	// N here is N.X
	// need to multiply first
	// ABI PLL PLLOUT=REF/DIVR_VAL*4*DIVF_VAL/DIVQ_VAL
	// DIVR_VAL=DIVR_DIVR[5:0]+1
	// DIVF_VAL=DIVFI+1+(DIVFF/1<<24)
	// DIVQ_VAL=(DIVQ[4:0]+1)*2
	N = (double)vco*dm/FREF/4;
	// calculate integer part N
	INT_N = vco*dm/FREF/4;
	// calculate fraction part N
	FRAC_N = N-INT_N;
	// since calculated frac may cause output frequency slightly less than
	// target. < 2*10^-10
	// we choose over clock that amount, which also satisfy the timing
	frac = FRAC_N==0?0:(FRAC_N*(1<<24)+1);

    	// value need to by register program ready, so translate here
	divr=dm-1;
	divfi=INT_N-1;
	divff=frac;
	divq=dp/2-1;
	divqf=dp1-1;

	debug(" calculated DIVFI is: %d\n", INT_N);
	debug(" calculated fraction is: %f\n", FRAC_N);
	debug(" calculated DIVFF is: %d\n", frac);
	debug(" calculated DIVQ and DIVQF is: %d %d\n", divq, divqf);
    }
    else
    {
        divr=apll_cfg.dm;
	divfi=apll_cfg.dn;
	divff=apll_cfg.frac;
        divq=apll_cfg.dp;
        divqf=apll_cfg.dp1;

	// these parameters are needed to decide if we could do a glitch-free on
	// the fly feedback divider change.
	dm=divr+1;
	INT_N=divfi+1;
	frac=divff;
    }


    // PLL global bypass
    ret |= diag_bypass_pll_global(pll, ABYPASS);

    // check if on-the-fly feedback divider would work.
    // only if SSC is not enabled. confirm with ASIC
    // waiting for ASIC confirmation, 10/23/19
    // On-the-fly adjustment is mainly for AVIO Plls for minor adjustment
    // remove from sequence.
    // Add individual command to support
    //
#if 0
    if(!ssc)
    	ret = diag_change_feedback_otf(pllbase, divr, divfi, divff);

    if(ret){
	if(ret==-1)
#if TARGET != BOOT
	    debug("On-The-Fly feedback change handshaking issue happens!\n");
#else
	    PRINT("On-The-Fly feedback change handshaking issue happens!\n");
#endif
	// change post divider
	ret |= diag_config_pll_dp(pllbase, divq, divqf);
	delay_us_real(1);

    }else{
#endif
	// PLL internal bypass, this will also power down the PLL per SPEC
	// p3
	ret |= diag_bypass_pll(pllbase, ABYPASS);


	// hold reset
	// PER SPEC, not necessary to hold both BYPASS and RESET, but we do it for
	// secure
	ret |= diag_reset_pll(pllbase, ARESET);


	// VS680 TB: no PLL mode selection in ABI PLL
	// change mode

	// update vco
	ret |= diag_config_pll_vco(pllbase, divr, divfi, divff);

	// update ssc settings
	// we need to use SSC configuration to decide if current Fpfd working in
	// fraction mode;
	int ssc_ret;
	ssc_ret = diag_config_pll_ssc(pllbase, ssc_freq, ssc_amp, ssc_mode, ssc);
	if(ssc_ret != 0)
	    debug("SSC change fail, SSC config not changed!\n");

	// update Fpfd range
	ret |= diag_config_pll_setpfdrange(pllbase);

	// update dp, dp1
	ret |= diag_config_pll_dp(pllbase, divq, divqf);
	// wait at least 2 ref_clock
	// assume FREF=25Mhz, we need to wait at least 80ns
	delay_us_real(1);

	// don't need to toggle power here, ABI PLL doesn't have output port
	// powerdown feature.

	// since de-assert BYPASS will make DVFI and DVFF being latched, we don't
	// need to utilize the glitch free fraction divider update here.
	// will have separate function to support it.

	// release RESET
	ret |= diag_reset_pll(pllbase, DEARESET);
	delay_us_real(1);

	// set BYPASS to 0
	// PLL internal bypass
	ret |= diag_bypass_pll(pllbase, DEABYPASS);

	// wait lock here
	// per SPEC p2, frequency lock time is 125 Fpfd cycles for integer mode and
	// 250 Fpfd cycles for fraction mode. For safety, we always use the slowest
	//     Fpfd cycle to calculate: 5Mhz --> 200ns --> 50us.
	// TB: per ASIC PLL sequence, change the delay to >=120 us
	//delay_us_real(50);
	delay_us_real(120);
	ret |= diag_wait_pll_lock(pllbase);
	// should still keep the clock settings, assume the clock is there
	// unbypass anyways.
	// NEED TO BE UPDATED
	if(ret)
	{
	    //debug("PLL CHANGE FAIL!!!\nPLL in BYPASS mode!!!\n");
	    debug("PLL NOT LOCK!!!\nASSUME PLL IS READY!!\n");
	    //return ret;
	}
#if 0
    }
#endif // end if of on-the-flt feedback divider change
    // PLL global bypass
    ret |= diag_bypass_pll_global(pll, DEABYPASS);

    //// update global ssc_status
    //curr_ssc[pll].ssc = ssc;
    //curr_ssc[pll].freq = ssc_freq;
    //curr_ssc[pll].amp = ssc_amp;
    return ret;
}

APLLCFG_t dumb_cfg = {
    .dm=1,
    .dn=32,
    .frac=0,
    .dp=1,
    .dp1=1
};

int diag_change_syspll0(unsigned int freq, unsigned int freq1, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode)
{
    return diag_change_pll(DIAG_SYSPLL0, freq, freq1, ssc, ssc_freq, ssc_amp, ssc_mode, 1, dumb_cfg);
}

int diag_change_syspll1(unsigned int freq, unsigned int freq1, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode)
{
    return diag_change_pll(DIAG_SYSPLL1, freq, freq1, ssc, ssc_freq, ssc_amp, ssc_mode, 1, dumb_cfg);
}

int diag_change_syspll2(unsigned int freq, unsigned int freq1, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode)
{
    return diag_change_pll(DIAG_SYSPLL2, freq, freq1, ssc, ssc_freq, ssc_amp, ssc_mode, 1, dumb_cfg);
}

int diag_change_cpupll(unsigned int freq, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode)
{
    return diag_change_pll(DIAG_CPUPLL, freq, freq, ssc, ssc_freq, ssc_amp, ssc_mode, 1, dumb_cfg);
}

int diag_change_mempll(unsigned int freq, unsigned int freq1, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode)
{
    return diag_change_pll(DIAG_MEMPLL, freq, freq1, ssc, ssc_freq, ssc_amp, ssc_mode, 1, dumb_cfg);
}

int diag_change_avpll(unsigned int apll_index, unsigned int freq,
        unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned
	int ssc_mode, unsigned int calc, APLLCFG_t pll_cfg)
{
    int ret=0;
    ret = diag_change_pll(apll_index+5, freq, freq, ssc, ssc_freq, ssc_amp, ssc_mode, calc, pll_cfg);

    if((int)apll_index<0 || (int)apll_index >3){
	debug("INVALID input apll index!\n");
	return -1;
    }

    return ret;
}

int diag_change_syspll0_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff)
{
    unsigned int pllbase = get_pll_base(DIAG_SYSPLL0);
    return diag_change_feedback_otf(pllbase, ndivr, divfi, divff);
}

int diag_change_syspll1_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff)
{
    unsigned int pllbase = get_pll_base(DIAG_SYSPLL1);
    return diag_change_feedback_otf(pllbase, ndivr, divfi, divff);
}

int diag_change_syspll2_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff)
{
    unsigned int pllbase = get_pll_base(DIAG_SYSPLL2);
    return diag_change_feedback_otf(pllbase, ndivr, divfi, divff);
}

int diag_change_cpupll_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff)
{
    unsigned int pllbase = get_pll_base(DIAG_CPUPLL);
    return diag_change_feedback_otf(pllbase, ndivr, divfi, divff);
}

int diag_change_mempll_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff)
{
    unsigned int pllbase = get_pll_base(DIAG_MEMPLL);
    return diag_change_feedback_otf(pllbase, ndivr, divfi, divff);
}

int diag_change_avpll_ppmadjustment(int apllindex, unsigned int ndivr, unsigned int divfi, unsigned int divff)
{
    unsigned int pllbase = get_pll_base(apllindex+5);
    return diag_change_feedback_otf(pllbase, ndivr, divfi, divff);
}

CLOCKO_t diag_get_syspll0(void)
{
    unsigned int pllbase = get_pll_base(DIAG_SYSPLL0);
    return get_clocko(pllbase);
}

CLOCKO_t diag_get_syspll1(void)
{
    unsigned int pllbase = get_pll_base(DIAG_SYSPLL1);
    return get_clocko(pllbase);
}

CLOCKO_t diag_get_syspll2(void)
{
    unsigned int pllbase = get_pll_base(DIAG_SYSPLL2);
    return get_clocko(pllbase);
}

CLOCKO_t diag_get_cpupll(void)
{
    unsigned int pllbase = get_pll_base(DIAG_CPUPLL);
    return get_clocko(pllbase);
}

CLOCKO_t diag_get_mempll(void)
{
    unsigned int pllbase = get_pll_base(DIAG_MEMPLL);
    return get_clocko(pllbase);
}

CLOCKO_t diag_get_avpll(int apllindex)
{
    unsigned int pllbase = get_pll_base(apllindex+5);
    return get_clocko(pllbase);
}
