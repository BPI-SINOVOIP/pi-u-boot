/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Cash Zhang <cazhan@synaptics.com>
 *         Shaojun Feng <Shaojun.feng@synaptics.com>
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h>
#include <memalign.h>
#include <reset.h>
#include <sdhci.h>
#include <clk.h>

DECLARE_GLOBAL_DATA_PTR;

/* Register Offset of SD Host Controller SOCP self-defined register */
#define SDHCI_P_VENDOR_SPECIFIC_AREA	0xE8
#define SDHCI_P_VENDOR_SPECIFIC_AREA_MASK 0xFFF
#define SDHCI_EMMC_CTRL_OFFSET			0x2C

/* phy */
#define PHY_COMMDL_CNFG_REG_OFFSET 0x031C
#define PHY_SMPLDL_CNFG_REG_OFFSET 0x0320
#define PHY_AT_CTRL_R_REG_OFFSET 0x540
#define TUNE_CLK_STOP_EN (1<<16)
#define PRE_CHANGE_DLY (3<<17)
#define POST_CHANGE_DLY (3<<19)
#define PHY_REG_OFFSET 0x300
#define PHY_CNFG_REG   0x300
#define PHY_CNFG_PHY_PWRGOOD_MASK 0x2

#define PHY_SDCLKDL_DC  0x31e
#define CCKDL_DC_MSK	0x7f

#define PHY_SDCLKDL_CNFG 0x31d
#define EXTDLY_EN		(1 << 0)
#define UPDATE_DC       (1 << 4)

#define PHY_DLL_CTRL			0x324
#define  DLL_EN				(1 << 0)

#define PHY_DLL_STATUS			0x32e
#define  LOCK_STS			(1 << 0)

#define PHY_DLLDBG_MLKDC			0x330
#define PHY_DLLDBG_SLKDC			0x332

/*PHY PAD GENERAL modes */
#define PAD_SP_8    0x8
#define PAD_SP_9    0x9
#define PAD_SP_E    0xE

#define PAD_SN_8    0x8
#define PAD_SN_E    0xE

typedef struct {
	unsigned int addr;
	unsigned int sp_bit;
	unsigned int sn_bit;
	unsigned int mask;
	unsigned int sp_value;
	unsigned int sn_value;
} PHY_PAD_GENERAL;

// BG7 PHY RXSEL config structure
typedef struct {
	unsigned int addr;
	unsigned int bit;
	unsigned int mask;
	unsigned int value;
} PHY_RXSEL;

/* PHY WEAKPULL_EN modes */
#define WPE_DISABLE  0x0
#define WPE_PULLUP   0x1
#define WPE_PULLDOWN 0x2
// BG7 PHY WEAKPULL_EN config structure
typedef struct {
	unsigned int addr;
	unsigned int bit;
	unsigned int mask;
	unsigned int value;
} PHY_WEAKPULL_EN;

/* PHY TXSLEW_CTRL_P modes */
#define TX_SLEW_P_0    0x0
#define TX_SLEW_P_2    0x2
#define TX_SLEW_P_3    0x3
// BG7 PHY TXSLEW_CTRL_P config structure
typedef struct {
	unsigned int addr;
	unsigned int bit;
	unsigned int mask;
	unsigned int value;
} PHY_TXSLEW_CTRL_P;

/* PHY TXSLEW_CTRL_N modes */
#define TX_SLEW_N_2    0x2
#define TX_SLEW_N_3    0x3
// BG7 PHY TXSLEW_CTRL_N config structure
typedef struct {
	unsigned int addr;
	unsigned int bit;
	unsigned int mask;
	unsigned int value;
} PHY_TXSLEW_CTRL_N;


struct phy_setting{
	unsigned int addr;
	unsigned int startbit;
	unsigned int mask;
	unsigned int value;
};

struct dwcmshc_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	struct reset_ctl_bulk reset_ctl;
	int num_phy_setting;
	struct phy_setting * phy_setting;
};

/* PHY RX SEL modes */
#define RXSELOFF        0x0
#define SCHMITT1P8		0x1
#define SCHMITT3P3		0x2
#define SCHMITT1P2		0x3
#define COMPARATOR1P8		0x4
#define COMPARATOR1P2		0x5
#define COMPARATOR1P82		0x6
#define INTERNALLPBK		0x7


/* 1v8 default PHY config */
PHY_PAD_GENERAL pad_general_1v8 =
{
	0x300, 16, 20, 0xF, PAD_SP_E, PAD_SN_E
};

PHY_RXSEL pad_rxsel_1v8[5] =
{
/*CMD*/	{0x304, 0, 	0x7, SCHMITT1P8},
/*DAT*/	{0x304, 16, 	0x7, SCHMITT1P8},
/*CLK*/	{0x308, 0, 	0x7, RXSELOFF},
/*STB*/	{0x308, 16, 	0x7, SCHMITT1P8},
/*RST*/	{0x30C, 0, 	0x7, SCHMITT1P8}
};

PHY_WEAKPULL_EN pad_weakpull_en_1v8[5] =
{
/*CMD*/	{0x304, 0 + 3, 	0x3, WPE_PULLUP},
/*DAT*/	{0x304, 16 + 3, 	0x3, WPE_PULLUP},
/*CLK*/	{0x308, 0 + 3, 	0x3, WPE_DISABLE},
/*STB*/	{0x308, 16 + 3, 	0x3, WPE_PULLDOWN},
/*RST*/	{0x30C, 0 + 3, 	0x3, WPE_PULLUP}
};

PHY_TXSLEW_CTRL_P pad_txslew_ctrl_p_1v8[5] =
{
/*CMD*/ {0x304, 0 + 5,	0xF, TX_SLEW_P_0},
/*DAT*/ {0x304, 16 + 5, 	0xF, TX_SLEW_P_0},
/*CLK*/ {0x308, 0 + 5,	0xF, TX_SLEW_P_0},
/*STB*/ {0x308, 16 + 5, 	0xF, TX_SLEW_P_0},
/*RST*/ {0x30C, 0 + 5,	0xF, TX_SLEW_P_0}
};

PHY_TXSLEW_CTRL_N pad_txslew_ctrl_n_1v8[5] =
{
/*CMD*/ {0x304, 0 + 9,	0xF, TX_SLEW_N_3},
/*DAT*/ {0x304, 16 + 9, 	0xF, TX_SLEW_N_3},
/*CLK*/ {0x308, 0 + 9,	0xF, TX_SLEW_N_3},
/*STB*/ {0x308, 16 + 9, 	0xF, TX_SLEW_N_3},
/*RST*/ {0x30C, 0 + 9,	0xF, TX_SLEW_N_3}
};


/* 3v3 default PHY config */
PHY_PAD_GENERAL pad_general_3v3 =
{
	0x300, 16, 20, 0xF, PAD_SP_9, PAD_SN_8
};

PHY_RXSEL pad_rxsel_3v3[5] =
{
/*CMD*/	{0x304, 0, 	0x7, SCHMITT3P3},
/*DAT*/	{0x304, 16, 	0x7, SCHMITT3P3},
/*CLK*/	{0x308, 0, 	0x7, RXSELOFF},
/*STB*/	{0x308, 16, 	0x7, SCHMITT3P3},
/*RST*/	{0x30C, 0, 	0x7, SCHMITT3P3}
};

PHY_WEAKPULL_EN pad_weakpull_en_3v3[5] =
{
/*CMD*/	{0x304, 0 + 3, 	0x3, WPE_PULLUP},
/*DAT*/	{0x304, 16 + 3, 	0x3, WPE_PULLUP},
/*CLK*/	{0x308, 0 + 3, 	0x3, WPE_DISABLE},
/*STB*/	{0x308, 16 + 3, 	0x3, WPE_PULLDOWN},
/*RST*/	{0x30C, 0 + 3, 	0x3, WPE_PULLUP}
};

PHY_TXSLEW_CTRL_P pad_txslew_ctrl_p_3v3[5] =
{
/*CMD*/ {0x304, 0 + 5,	0xF, TX_SLEW_P_3},
/*DAT*/ {0x304, 16 + 5, 	0xF, TX_SLEW_P_3},
/*CLK*/ {0x308, 0 + 5,	0xF, TX_SLEW_P_3},
/*STB*/ {0x308, 16 + 5, 	0xF, TX_SLEW_P_3},
/*RST*/ {0x30C, 0 + 5,	0xF, TX_SLEW_P_3}
};

PHY_TXSLEW_CTRL_N pad_txslew_ctrl_n_3v3[5] =
{
/*CMD*/ {0x304, 0 + 9,	0xF, TX_SLEW_N_2},
/*DAT*/ {0x304, 16 + 9, 	0xF, TX_SLEW_N_2},
/*CLK*/ {0x308, 0 + 9,	0xF, TX_SLEW_N_2},
/*STB*/ {0x308, 16 + 9, 	0xF, TX_SLEW_N_2},
/*RST*/ {0x30C, 0 + 9,	0xF, TX_SLEW_N_2}
};


int dwcmsh_PHYconfig(struct sdhci_host *host, struct phy_setting *ps, int num)
{
	int i;
	volatile unsigned int val;
	int tmout = 100, ret = 0;

	for(i = 0; i < num; i++)
	{
		val = sdhci_readl(host, ps[i].addr);
		val &= ~(ps[i].mask << ps[i].startbit);
		val |= (ps[i].value << ps[i].startbit);
		sdhci_writel(host, val, ps[i].addr);
	}

	do {
		val = sdhci_readl(host, PHY_CNFG_REG);

		mdelay(1);
		if(!tmout) {
			printf("EMMC PHY's PowerGood status is not ready !\n");
			ret = -1;
		}
	}while(!(val & PHY_CNFG_PHY_PWRGOOD_MASK) && tmout--);

	return ret;
}

void dwcmsh_PHYreset(struct sdhci_host *host, int rst)
{
	volatile unsigned int val;

	val = sdhci_readl(host, PHY_CNFG_REG);
	val &= ~(0x1 << 0);
	val |= rst;
	sdhci_writel(host, val, PHY_CNFG_REG);
}

static int dwcmsh_phy_switch_power(struct sdhci_host *host)
{
	int tmout=100, ret=0;
	int i;
	volatile unsigned int val;

	if(host->mmc->signal_voltage == MMC_SIGNAL_VOLTAGE_180)
	{
		//config PHY_CNFG, general configuration
	    //Dolphin_BG7_PHY_bring_up_sequence.xlsx
	    //step 10
		val = sdhci_readl(host, pad_general_1v8.addr);
		val &= ~(pad_general_1v8.mask<<pad_general_1v8.sp_bit);
		val |= (pad_general_1v8.sp_value<<pad_general_1v8.sp_bit);
		val &= ~(pad_general_1v8.mask<<pad_general_1v8.sn_bit);
		val |= (pad_general_1v8.sn_value<<pad_general_1v8.sn_bit);
		sdhci_writel(host, val, pad_general_1v8.addr);

		//Dolphin_BG7_PHY_bring_up_sequence.xlsx
		//step 11~15
		for(i=0; i<5; i++)
		{
			//config PHY RXSEL
			val = sdhci_readl(host, pad_rxsel_1v8[i].addr);
			val &= ~(pad_rxsel_1v8[i].mask<<pad_rxsel_1v8[i].bit);
			val |= (pad_rxsel_1v8[i].value<<pad_rxsel_1v8[i].bit);
			sdhci_writel(host, val, pad_rxsel_1v8[i].addr);


			//config PHY WEAKPULL_EN
			val = sdhci_readl(host, pad_weakpull_en_1v8[i].addr);
			val &= ~(pad_weakpull_en_1v8[i].mask<<pad_weakpull_en_1v8[i].bit);
			val |= (pad_weakpull_en_1v8[i].value<<pad_weakpull_en_1v8[i].bit);
			sdhci_writel(host, val, pad_weakpull_en_1v8[i].addr);
			//config PHY TXSLEW_CTRL_P
			val = sdhci_readl(host, pad_txslew_ctrl_p_1v8[i].addr);
			val &= ~(pad_txslew_ctrl_p_1v8[i].mask<<pad_txslew_ctrl_p_1v8[i].bit);
			val |= (pad_txslew_ctrl_p_1v8[i].value<<pad_txslew_ctrl_p_1v8[i].bit);
			sdhci_writel(host, val, pad_txslew_ctrl_p_1v8[i].addr);

			//config PHY TXSLEW_CTRL_N
			val = sdhci_readl(host, pad_txslew_ctrl_n_1v8[i].addr);
			val &= ~(pad_txslew_ctrl_n_1v8[i].mask<<pad_txslew_ctrl_n_1v8[i].bit);
			val |= (pad_txslew_ctrl_n_1v8[i].value<<pad_txslew_ctrl_n_1v8[i].bit);
			sdhci_writel(host, val, pad_txslew_ctrl_n_1v8[i].addr);
		}
	}
	else if(host->mmc->signal_voltage == MMC_SIGNAL_VOLTAGE_330)
	{
		//config PHY_CNFG, general configuration
	    //Dolphin_BG7_PHY_bring_up_sequence.xlsx
	    //step 10
		val = sdhci_readl(host, pad_general_3v3.addr);
		val &= ~(pad_general_3v3.mask<<pad_general_3v3.sp_bit);
		val |= (pad_general_3v3.sp_value<<pad_general_3v3.sp_bit);
		val &= ~(pad_general_3v3.mask<<pad_general_3v3.sn_bit);
		val |= (pad_general_3v3.sn_value<<pad_general_3v3.sn_bit);
		sdhci_writel(host, val, pad_general_3v3.addr);

		//Dolphin_BG7_PHY_bring_up_sequence.xlsx
		//step 11~15
		for(i=0; i<5; i++)
		{
			//config PHY RXSEL
			val = sdhci_readl(host, pad_rxsel_3v3[i].addr);
			val &= ~(pad_rxsel_3v3[i].mask<<pad_rxsel_3v3[i].bit);
			val |= (pad_rxsel_3v3[i].value<<pad_rxsel_3v3[i].bit);
			sdhci_writel(host, val, pad_rxsel_3v3[i].addr);


			//config PHY WEAKPULL_EN
			val = sdhci_readl(host, pad_weakpull_en_3v3[i].addr);
			val &= ~(pad_weakpull_en_3v3[i].mask<<pad_weakpull_en_3v3[i].bit);
			val |= (pad_weakpull_en_3v3[i].value<<pad_weakpull_en_3v3[i].bit);
			sdhci_writel(host, val, pad_weakpull_en_3v3[i].addr);

			//config PHY TXSLEW_CTRL_P
			val = sdhci_readl(host, pad_txslew_ctrl_p_3v3[i].addr);
			val &= ~(pad_txslew_ctrl_p_3v3[i].mask<<pad_txslew_ctrl_p_3v3[i].bit);
			val |= (pad_txslew_ctrl_p_3v3[i].value<<pad_txslew_ctrl_p_3v3[i].bit);
			sdhci_writel(host, val, pad_txslew_ctrl_p_3v3[i].addr);

			//config PHY TXSLEW_CTRL_N
			val = sdhci_readl(host, pad_txslew_ctrl_n_3v3[i].addr);
			val &= ~(pad_txslew_ctrl_n_3v3[i].mask<<pad_txslew_ctrl_n_3v3[i].bit);
			val |= (pad_txslew_ctrl_n_3v3[i].value<<pad_txslew_ctrl_n_3v3[i].bit);
			sdhci_writel(host, val, pad_txslew_ctrl_n_3v3[i].addr);
		}
	}
	else
	{
		printf("EMMC PHY's Power signal_voltage %d not support!", host->mmc->signal_voltage);

	}

    //wait for PHY powergood
    //Dolphin_BG7_PHY_bring_up_sequence.xlsx
    //step 16
	do {
		val = sdhci_readl(host, PHY_CNFG_REG);
		if(val & PHY_CNFG_PHY_PWRGOOD_MASK)
			break;

		mdelay(1);
		if(!tmout) {
			printf("EMMC PHY's PowerGood status is not ready !\n");
			ret = -1;
		}
	}while(!ret && tmout--);

	return ret;
}

static void dwcmshc_set_phy_tx_delay(struct sdhci_host *host, u8 delay)
{
	u8 valb, valdc;

	valb = sdhci_readb(host, PHY_SDCLKDL_CNFG);
	valb |= UPDATE_DC;
	sdhci_writeb(host, valb, PHY_SDCLKDL_CNFG);

	valdc = sdhci_readb(host, PHY_SDCLKDL_DC);
	valdc &= ~CCKDL_DC_MSK;
	valdc |= delay;
	sdhci_writeb(host, valdc, PHY_SDCLKDL_DC);

	//disable extdelay，from diag
	valb = sdhci_readb(host, PHY_SDCLKDL_CNFG);
	valb &= ~EXTDLY_EN;
	sdhci_writeb(host, valb, PHY_SDCLKDL_CNFG);

	valb = sdhci_readb(host, PHY_SDCLKDL_CNFG);
	valb &= ~UPDATE_DC;
	sdhci_writeb(host, valb, PHY_SDCLKDL_CNFG);
}

static int dwcmshc_phy_dll_cal(struct sdhci_host *host)
{
	u8 val, valm, vals;
	int ret,i;
	static int cal_val = 0;
	if(cal_val > 0)
		return cal_val;

	val = sdhci_readb(host, PHY_DLL_CTRL);
	val &= ~DLL_EN;
	sdhci_writeb(host, val, PHY_DLL_CTRL);
	val |= DLL_EN;
	sdhci_writeb(host, val, PHY_DLL_CTRL);

	for(i=0; i < 20; i++){
		val = sdhci_readb(host, PHY_DLL_STATUS);
		if(val & LOCK_STS){
			ret = 0;
			break;
		}

		mdelay(1);
	}

	if (ret)
		return -1;

	valm = sdhci_readb(host, PHY_DLLDBG_MLKDC);
	vals = sdhci_readb(host, PHY_DLLDBG_SLKDC);
	if (!valm || !vals)
		return -EINVAL;
	val = valm / vals;
	if (val == 4)
		ret = 5000 / valm;
	else if (val == 2)
		ret = 5000 / 2 / valm;
	else
		ret = 5000 / 4 / vals;
	if (!ret)
		return -EINVAL;
	ret = 1400 / ret;
	ret++;

	if(ret > 0)
		cal_val = ret;

	printf("dll-calibration result: %d\n", ret);
	return ret;
}

static void dwcmshc_sdhci_set_control_reg(struct sdhci_host *host)
{
	volatile unsigned int val = 0;

	//disable SD_CLK
	val = sdhci_readw(host, SDHCI_EMMC_CTRL_OFFSET);
	sdhci_writew(host, val & ~(0x1 << 2), SDHCI_EMMC_CTRL_OFFSET);

	//switch_phy_power
	dwcmsh_phy_switch_power(host);
	sdhci_set_control_reg(host);

	//recover SD_CLK
	sdhci_writew(host, val, SDHCI_EMMC_CTRL_OFFSET);

	return;
}

static int dwcmshc_sdhci_set_delay(struct sdhci_host *host)
{
	u8 delay = 0;
	//set delay line
	if(host->mmc->selected_mode == UHS_SDR104)
		delay = 75; //val copy from kernel
	else if (host->mmc->selected_mode ==  MMC_HS)
		//delay = 100;
		delay = 26; //evk test ok
	else if (host->mmc->selected_mode ==  MMC_DDR_52)
		//delay = 90; //copy from kernel,can't working
		delay = 26; //evk test ok
	else if (host->mmc->selected_mode == MMC_HS_200)
		//delay = 40; //work ok
		delay = 26; //evk test ok. value from diag
	else if (host->mmc->selected_mode == MMC_HS_400)
	{
		sdhci_writew(host,  0x103, SDHCI_CLOCK_CONTROL);
		delay = dwcmshc_phy_dll_cal(host);
	}
		//delay = 29;// from evk dwcmshc_phy_dll_cal

	if(delay)
		dwcmshc_set_phy_tx_delay(host, delay);

	return 0;
}

void dwcmshc_sdhci_reset_tuning(struct sdhci_host *host)
{
	u16 ctrl;

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	ctrl &= ~SDHCI_CTRL_TUNED_CLK;
	ctrl &= ~SDHCI_CTRL_EXEC_TUNING;
	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
}

void debug_dump_reg(struct sdhci_host *host)
{

printf("SDHCI_HOST_CONTROL(%x):     0x%x\n",SDHCI_HOST_CONTROL,
		   sdhci_readb(host, SDHCI_HOST_CONTROL));

printf("SDHCI_POWER_CONTROL(%x):     0x%x\n",SDHCI_POWER_CONTROL,
		   sdhci_readb(host, SDHCI_POWER_CONTROL));

printf("SDHCI_CLOCK_CONTROL(%x):     0x%x\n",SDHCI_CLOCK_CONTROL,
		   sdhci_readw(host, SDHCI_CLOCK_CONTROL));

printf("SDHCI_HOST_CONTROL2(%x):     0x%x\n",SDHCI_HOST_CONTROL2,
		   sdhci_readw(host, SDHCI_HOST_CONTROL2));

//phy
printf("(%x):     0x%x\n",0x300,sdhci_readl(host, 0x300));
printf("(%x):     0x%x\n",0x304,sdhci_readw(host, 0x304));
printf("(%x):     0x%x\n",0x306,sdhci_readw(host, 0x306));
printf("(%x):     0x%x\n",0x308,sdhci_readw(host, 0x308));
printf("(%x):     0x%x\n",0x30a,sdhci_readw(host, 0x30a));
printf("(%x):     0x%x\n",0x30c,sdhci_readw(host, 0x30c));
printf("(%x):     0x%x\n",0x31d,sdhci_readb(host, 0x31d));
printf("(%x):     0x%x\n",0x31e,sdhci_readb(host, 0x31e));

//vendor1
printf("(%x):     0x%x\n",0x30c,sdhci_readw(host, 0x52c));
printf("(%x):     0x%x\n",0x540,sdhci_readl(host, 0x540));
printf("(%x):     0x%x\n",0x544,sdhci_readl(host, 0x544));

}

#define SDHCI_MAX_TUNING_LOOP_COUNT	150//40
static int dwcmshc_sdhci_execute_tuning(struct mmc *mmc, u8 opcode)
{
	struct sdhci_host *host = (struct sdhci_host *)mmc->priv;
	char tuning_loop_counter = SDHCI_MAX_TUNING_LOOP_COUNT;
	struct mmc_cmd cmd;
	u32 ctrl, blk_size, val, i, int_en, signal_en;
	u16 clk;
	int ret = -1;

	//1.prepare
	dwcmshc_sdhci_reset_tuning(host);
	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	sdhci_writew(host, clk & ~SDHCI_CLOCK_CARD_EN, SDHCI_CLOCK_CONTROL);
	val = sdhci_readl(host, AT_CTRL_R);
	val &= ~SWIN_TH_EN;
	val &= ~RPT_TUNE_ERR;
	val |= AT_EN;
	sdhci_writel(host, val, AT_CTRL_R);
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);


	//2. start tuning,sdhci_execute_tuning
	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	ctrl |= SDHCI_CTRL_EXEC_TUNING;
	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

	int_en = sdhci_readl(host, SDHCI_INT_ENABLE);
	signal_en = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);

	sdhci_writel(host, SDHCI_INT_DATA_AVAIL, SDHCI_INT_ENABLE);
	sdhci_writel(host, SDHCI_INT_DATA_AVAIL, SDHCI_SIGNAL_ENABLE);


	cmd.cmdidx = opcode;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;

	for(i = 0; i < tuning_loop_counter; i++) {
	/*
		* In response to CMD19, the card sends 64 bytes of tuning
		* block to the Host Controller. So we set the block size
		* to 64 here.
		*/

		blk_size = SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG, 64);
		if (opcode == MMC_CMD_SEND_TUNING_BLOCK_HS200 && host->mmc->bus_width == 8)
			blk_size = SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG, 128);
		sdhci_writew(host, blk_size, SDHCI_BLOCK_SIZE);

		/*
		* The tuning block is sent by the card to the host controller.
		* So we set the TRNS_READ bit in the Transfer Mode register.
		* This also takes care of setting DMA Enable and Multi Block
		* Select in the same register to 0.
		*/
		sdhci_writew(host, SDHCI_TRNS_READ, SDHCI_TRANSFER_MODE);

		mmc_send_cmd(mmc, &cmd, NULL);

		mdelay(1);

		ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		if (!(ctrl & SDHCI_CTRL_EXEC_TUNING)) {
			if (ctrl & SDHCI_CTRL_TUNED_CLK) {
				ret = 0; /* Success! */
				break;
			}
			else {
				dwcmshc_sdhci_reset_tuning(host);
				ret = -1;
				printf("tuning failed\n");
				break;
			}
		}
	}

	//3.restor interrupts,end tuning
	sdhci_writel(host, int_en, SDHCI_INT_ENABLE);
	sdhci_writel(host, signal_en, SDHCI_SIGNAL_ENABLE);

	return ret;
}

static int dwcmshc_hs400_enhanced_strobe(struct sdhci_host *host)
{
	u32 val = 0;
	val = sdhci_readl(host, EMMC_CTRL_R);
	val |= ENH_STROBE_ENABLE;
	sdhci_writel(host, val, EMMC_CTRL_R);
	return 0;
}

const struct sdhci_ops dwcmshc_ops = {
	.set_control_reg = &dwcmshc_sdhci_set_control_reg,
	.platform_execute_tuning = &dwcmshc_sdhci_execute_tuning,
	.set_delay = &dwcmshc_sdhci_set_delay,
	.set_enhanced_strobe = &dwcmshc_hs400_enhanced_strobe,
};

static void dwcmshc_sdhci_reset(struct dwcmshc_sdhci_plat *plat,
			      struct sdhci_host *host)
{
	u32 data, val;

	/* eMMC reset Host */
	reset_assert_bulk(&plat->reset_ctl);
	mdelay(1);
	reset_deassert_bulk(&plat->reset_ctl);

	/* eMMC reset device */
	data = sdhci_readl(host, SDHCI_P_VENDOR_SPECIFIC_AREA);
	data &= SDHCI_P_VENDOR_SPECIFIC_AREA_MASK;
	data += SDHCI_EMMC_CTRL_OFFSET;
	val = sdhci_readl(host, data);
	val |= 0x1;
	val |= (0x1 << 3);
	val &= ~(0x1 << 2);
	sdhci_writel(host, val, data);
	mdelay(1);
	val = sdhci_readl(host, data);
	val |= (0x1 << 2);
	sdhci_writel(host, val, data);
	mdelay(1);
	val = sdhci_readl(host, data);
}

static void dwcmshc_sdhci_slot_init(struct sdhci_host *host)
{
	u32 offset, data;

	offset = sdhci_readl(host, SDHCI_P_VENDOR_SPECIFIC_AREA);
	offset &= SDHCI_P_VENDOR_SPECIFIC_AREA_MASK;
	offset += SDHCI_EMMC_CTRL_OFFSET;
	data = sdhci_readl(host, offset);
	data |= 0x1;
	sdhci_writel(host, data, offset);
}


static void SDHC_PHYDelayLineSetup(struct udevice *dev)
{
	struct sdhci_host *host = dev_get_priv(dev);
	unsigned int value0, value1;

	value0 = sdhci_readl(host, PHY_COMMDL_CNFG_REG_OFFSET);
	value1 = sdhci_readl(host, PHY_SMPLDL_CNFG_REG_OFFSET);

	value0 &= ~(0x1f03);
	sdhci_writel(host, value0, PHY_COMMDL_CNFG_REG_OFFSET);

	value1 &= ~(0x313);
	value1 |= 0xc0c;
	sdhci_writel(host, value1, PHY_SMPLDL_CNFG_REG_OFFSET);

	value0 = sdhci_readl(host, PHY_COMMDL_CNFG_REG_OFFSET);
	value0 |= 0x1000;
	sdhci_writel(host, value0, PHY_COMMDL_CNFG_REG_OFFSET);
	value0 |= 0x7f0000;
	sdhci_writel(host, value0, PHY_COMMDL_CNFG_REG_OFFSET);

	value0 = sdhci_readl(host, PHY_COMMDL_CNFG_REG_OFFSET);
	value0 &= ~0x1000;
	sdhci_writel(host, value0, PHY_COMMDL_CNFG_REG_OFFSET);
}

static void SDHC_PHYTuningSetup(struct udevice *dev)
{
	struct sdhci_host *host = dev_get_priv(dev);
	unsigned int value;

	value = sdhci_readl(host, PHY_AT_CTRL_R_REG_OFFSET);
	value &= ~TUNE_CLK_STOP_EN & ~POST_CHANGE_DLY & ~PRE_CHANGE_DLY;
	value |= (1<<16) | (3<<17) | (3<<19);
	sdhci_writel(host, value, PHY_AT_CTRL_R_REG_OFFSET);
}

static int dwcmshc_sdhci_probe(struct udevice *dev)
{
	struct dwcmshc_sdhci_plat *plat = dev_get_platdata(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	int ret = 0;
	int val;
	struct clk clk;
	unsigned long clock;

	dwcmshc_sdhci_reset(plat, host);
	ret = clk_get_by_index(dev, 0, &clk);
	if (ret < 0) {
		dev_err(dev, "failed to get clock\n");
		return ret;
	}

	clock = clk_get_rate(&clk);
	if (IS_ERR_VALUE(clock)) {
		dev_err(dev, "failed to get rate\n");
		return clock;
	}

	host->max_clk = clock;
	host->mmc = &plat->mmc;
	host->mmc->dev = dev;

	host->quirks = SDHCI_QUIRK_NO_1_8_V;
	if (dev_read_bool(dev, "1_8v-signalling"))
		host->mmc->signal_voltage = MMC_SIGNAL_VOLTAGE_180;
	if (dev_read_bool(dev, "3_3v-signalling"))
		host->mmc->signal_voltage = MMC_SIGNAL_VOLTAGE_330;


#ifdef CONFIG_DM_GPIO
		struct gpio_desc desc;
		ret = gpio_request_by_name(dev, "snps,select-sd-gpio", 0,
			&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
		if(ret)
			pr_debug("snps,select-sd-gpio error,ret is %d\n", ret);
#endif

	ret = sdhci_setup_cfg(&plat->cfg, host, plat->cfg.f_max, 0);
	if (ret)
		return ret;

	upriv->mmc = host->mmc;
	host->mmc->priv = host;

	dwcmshc_sdhci_slot_init(host);
	ret = sdhci_probe(dev);
	if(ret)
		return ret;

	if(plat->num_phy_setting && plat->phy_setting) {
		SDHC_PHYDelayLineSetup(dev);
		SDHC_PHYTuningSetup(dev);
		if(MMC_SIGNAL_VOLTAGE_180 == host->mmc->signal_voltage) {
			val = sdhci_readl(host, pad_general_1v8.addr);
			val &= ~(pad_general_1v8.mask<<pad_general_1v8.sp_bit);
			val |= (pad_general_1v8.sp_value<<pad_general_1v8.sp_bit);
			val &= ~(pad_general_1v8.mask<<pad_general_1v8.sn_bit);
			val |= (pad_general_1v8.sn_value<<pad_general_1v8.sn_bit);
			sdhci_writel(host, val, pad_general_1v8.addr);
		} else if(MMC_SIGNAL_VOLTAGE_330 == host->mmc->signal_voltage) {
			val = sdhci_readl(host, pad_general_3v3.addr);
			val &= ~(pad_general_3v3.mask<<pad_general_3v3.sp_bit);
			val |= (pad_general_3v3.sp_value<<pad_general_3v3.sp_bit);
			val &= ~(pad_general_3v3.mask<<pad_general_3v3.sn_bit);
			val |= (pad_general_3v3.sn_value<<pad_general_3v3.sn_bit);
			sdhci_writel(host, val, pad_general_3v3.addr);
		}
		ret = dwcmsh_PHYconfig(host, plat->phy_setting, plat->num_phy_setting);
		dwcmsh_PHYreset(host, 1);
	}

	return ret;
}

static int dwcmshc_sdhci_ofdata_to_platdata(struct udevice *dev)
{
	struct dwcmshc_sdhci_plat *plat = dev_get_platdata(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	struct mmc_config *cfg = &plat->cfg;
	int ret;
	int len, i, j;
	const unsigned int * ps = 0;

	if (reset_get_bulk(dev, &plat->reset_ctl)) {
		printf("%s: can't get reset index for xenon_sdhci\n", __func__);
		return -ENODEV;
	}
	host->name = dev->name;
	host->ioaddr = (void *)dev_read_addr(dev);

	ps = dev_read_prop(dev, "phy-setting", &len);
	if(ps) {
		if(len % sizeof(struct phy_setting)) {
			printf("%s: phy setting is not correct\n", __func__);
			return -EINVAL;
		}
		plat->num_phy_setting = len / sizeof(struct phy_setting);
		plat->phy_setting = malloc(len);
		for(i = 0, j = 0; i < plat->num_phy_setting ; i++) {
			plat->phy_setting[i].addr = fdt32_to_cpu(ps[j++]);
			plat->phy_setting[i].startbit = fdt32_to_cpu(ps[j++]);
			plat->phy_setting[i].mask = fdt32_to_cpu(ps[j++]);
			plat->phy_setting[i].value = fdt32_to_cpu(ps[j++]);
		}
	}

	ret = mmc_of_parse(dev, cfg);
	if (ret)
		return ret;

	host->ops = &dwcmshc_ops;

	return 0;
}

static int dwcmshc_sdhci_bind(struct udevice *dev)
{
	struct dwcmshc_sdhci_plat *plat = dev_get_platdata(dev);
	int ret;

	ret = sdhci_bind(dev, &plat->mmc, &plat->cfg);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id dwcmshc_sdhci_ids[] = {
	{ .compatible = "snps,dwcmshc-sdhci" },
	{ }
};

U_BOOT_DRIVER(dwcmshc_sdhci) = {
	.name	= "dwcmshc-sdhci",
	.id	= UCLASS_MMC,
	.of_match = dwcmshc_sdhci_ids,
	.ofdata_to_platdata = dwcmshc_sdhci_ofdata_to_platdata,
	.ops	= &sdhci_ops,
	.bind	= dwcmshc_sdhci_bind,
	.probe	= dwcmshc_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct sdhci_host),
	.platdata_auto_alloc_size = sizeof(struct dwcmshc_sdhci_plat),
};
