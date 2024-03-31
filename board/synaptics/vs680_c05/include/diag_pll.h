/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#include <common.h>
#include <asm/io.h>
#include "Galois_memmap.h"
#include "global.h"
#include "cpu_wrp.h"
#include "mc_wrap.h"
#include "avioGbl.h"
#include "avio_memmap.h"
//#include "debug.h"

#define DOWN            1
#define UP              0

#define ENABLE          1
#define DISABLE         0

#define ABYPASS          1
#define DEABYPASS        0

#define ARESET          1
#define DEARESET        0

#define INT             0
#define FRACTION        1

#define DPONLY          1
#define ALL             0

// REF CLOCK divider, recommended value from ASIC Vinson as 1.
// FREF=25Mhz, FREF/Mint [1:100], FREF/Mfrac [10:100] ==> M [1:2]
// 1 is recommended by ASIC.
#define M1              1
#define M2              2
#define M4		4
#define M5		5
#define M6		6
#define FREF            25

#define VPLLWRAP0BASE_ADDR    	(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_VPLL_WRAP0)
#define VPLLWRAP1BASE_ADDR    	(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_VPLL_WRAP1)
#define APLLWRAP0BASE_ADDR    	(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_APLL_WRAP0)
#define APLLWRAP1BASE_ADDR    	(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_APLL_WRAP1)

#define APLLWRAP0APLLCTRL     	APLLWRAP0BASE_ADDR + RA_APLL_WRAP_APLL_CTRL
#define APLLWRAP1APLLCTRL     	APLLWRAP1BASE_ADDR + RA_APLL_WRAP_APLL_CTRL
#define APLLWRAP0APLL     	APLLWRAP0BASE_ADDR + RA_APLL_WRAP_APLL
#define APLLWRAP1APLL     	APLLWRAP1BASE_ADDR + RA_APLL_WRAP_APLL

#define VPLLWRAP0VPLLCTRL     	VPLLWRAP0BASE_ADDR + RA_VPLL_WRAP_VPLL_CTRL
#define VPLLWRAP1VPLLCTRL     	VPLLWRAP1BASE_ADDR + RA_VPLL_WRAP_VPLL_CTRL
#define VPLLWRAP0VPLL     	VPLLWRAP0BASE_ADDR + RA_VPLL_WRAP_VPLL
#define VPLLWRAP1VPLL     	VPLLWRAP1BASE_ADDR + RA_VPLL_WRAP_VPLL


//#define APLL_CLKEN_ADDR   (MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_AVPLLA_CLK_EN)
//#define APLL_SWRST_ADDR   (MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_SWRST_CTRL)
#define AVPLL_SWPD_ADDR    (MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_AVPLL_CTRL0)

enum PLLMODE {
    INT_MODE=0,
    FRAC_MODE,
    SSC_MODE,
    RSVD
};

#define PLL_REG_SET(tmpstr, field123, val123)     tmpstr.uctrl##field123 = val123
#define PLL_REG_GET(tmpstr, field123, val123)     val123 = tmpstr.uctrl##field123

#define PLL_REG_READ(baseaddr, offset, val123)                                          \
                        do{                                                             \
                            val123 = readl((long)(baseaddr+offset*4));          \
                        }while(0)

#define PLL_REG_WRITE(baseaddr, offset, val123)                                         \
                        do{                                                             \
                            writel(val123, (long)(baseaddr+offset*4));          \
                        }while(0)

#define PLL_REG_PRINT(baseaddr, offset)                                                 \
                        do{                                                             \
                            volatile unsigned int val123;                               \
                            val123 = readl((long)(baseaddr+offset*4));          \
                            debug("0x%08x: 0x%08x\n",                              \
                                    (baseaddr+offset*4),                                \
                                    val123);                                            \
                        }while(0)
#define SSC_DIV_MAX		15


typedef enum {
    DIAG_SYSPLL0=0,
    DIAG_SYSPLL1,
    DIAG_SYSPLL2,
    DIAG_MEMPLL,
    DIAG_CPUPLL,
    DIAG_VPLL0,
    DIAG_VPLL1,
    DIAG_APLL0,
    DIAG_APLL1,
    DIAG_AVPLL,
    DIAG_SYSPLL
} E_PLL_SRC;

enum divider_index
{
	DIVIDED_BY_2 = 1,
	DIVIDED_BY_4,
	DIVIDED_BY_6,
	DIVIDED_BY_8,
	DIVIDED_BY_12
};

typedef struct {
    int clocko;
    int clocko1;
} CLOCKO_t;

typedef struct {
    unsigned int dm;
    unsigned int dn;
    unsigned int frac;
    unsigned int dp;
    unsigned int dp1;
} APLLCFG_t;

typedef struct {
    unsigned int ssc;
    unsigned int freq;
    unsigned int amp;
} SSC_t;

typedef struct {
    int div;
    int val;
} ssc_div_t;

int diag_change_syspll0(unsigned int freq, unsigned int freq1, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode);
int diag_change_syspll1(unsigned int freq, unsigned int freq1, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode);
int diag_change_syspll2(unsigned int freq, unsigned int freq1, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode);
int diag_change_cpupll(unsigned int freq, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode);
//int diag_change_mempll(unsigned int freq, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode);
int diag_change_mempll(unsigned int freq, unsigned int freq1, unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned int ssc_mode);
int diag_change_avpll(unsigned int apll_index, unsigned int freq,
        unsigned int ssc, unsigned int ssc_freq, unsigned int ssc_amp, unsigned
	int ssc_mode, unsigned int calc, APLLCFG_t pll_cfg);


int diag_change_syspll0_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff);
int diag_change_syspll1_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff);
int diag_change_syspll2_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff);
int diag_change_cpupll_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff);
int diag_change_mempll_ppmadjustment(unsigned int ndivr, unsigned int divfi, unsigned int divff);
int diag_change_avpll_ppmadjustment(int apllindex, unsigned int ndivr, unsigned int divfi, unsigned int divff);

CLOCKO_t diag_get_syspll0(void);
CLOCKO_t diag_get_syspll1(void);
CLOCKO_t diag_get_syspll2(void);
CLOCKO_t diag_get_cpupll(void);
CLOCKO_t diag_get_mempll(void);
CLOCKO_t diag_get_avpll(int apllindex);
