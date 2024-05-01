// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Spacemit
 */

#ifdef CONFIG_K1_X_BOARD_ASIC

#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <div64.h>
#include <fdtdec.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/sizes.h>
#include "ddr_init_asic.h"
#include <mapmem.h>
#include <u-boot/crc.h>
#include "ddr_freq.h"

#define BOOT_PP		0
#define PMUA_REG_BASE	0xd4282800
#define PMUA_MCK_CTRL	(PMUA_REG_BASE + 0xe8)
#define PMUA_MC_HW_SLP_TYPE	(PMUA_REG_BASE + 0xb0)
#define REG32(x)	(*((volatile uint32_t *)((uintptr_t)(x))))

#define LOGLEVEL 0
#if (LOGLEVEL > 0)
#define LogMsg(level, format, args...) \
	do { \
		if (level < LOGLEVEL) \
			printf(format, ##args); \
	} while (0)
#else
#define LogMsg(level, format, args...)
#endif

extern u32 ddr_cs_num;
extern u32 ddr_get_mr8(void);
extern uint32_t get_manufacture_id(void);
extern uint32_t get_ddr_rev_id(void);

struct addrmap_info {
	u32 io_width_per_channel;
	u32 density_per_channel;
	u32 bank_num;
	u32 row_num;
	u32 col_num;
};

static const struct addrmap_info ddr_addrmap[] = {
	{IO_X16, DDR_1Gb , BANK_8, ROW_13, COL_10},
	{IO_X16, DDR_2Gb , BANK_8, ROW_14, COL_10},
	{IO_X16, DDR_3Gb , BANK_8, ROW_15, COL_10},
	{IO_X16, DDR_4Gb , BANK_8, ROW_15, COL_10},
	{IO_X16, DDR_6Gb , BANK_8, ROW_16, COL_10},
	{IO_X16, DDR_8Gb , BANK_8, ROW_16, COL_10},
	{IO_X16, DDR_12Gb, BANK_8, ROW_17, COL_10},
	{IO_X16, DDR_16Gb, BANK_8, ROW_17, COL_10},
	{IO_X8 , DDR_1Gb , BANK_8, ROW_14, COL_10},
	{IO_X8 , DDR_2Gb , BANK_8, ROW_15, COL_10},
	{IO_X8 , DDR_3Gb , BANK_8, ROW_16, COL_10},
	{IO_X8 , DDR_4Gb , BANK_8, ROW_16, COL_10},
	{IO_X8 , DDR_6Gb , BANK_8, ROW_17, COL_10},
	{IO_X8 , DDR_8Gb , BANK_8, ROW_17, COL_10},
	{IO_X8 , DDR_12Gb, BANK_8, ROW_18, COL_10},
	{IO_X8 , DDR_16Gb, BANK_8, ROW_18, COL_10},
};

void enable_PLL(void)
{
	unsigned read_data = 0;
	REG32(0xd4282800 + 0x3b4) &= 0xFFFFFCFF;
	REG32(0xd4282800 + 0x3b4) |= (0x1 << 11) | (0x1 << 8) | (0x1 << 9);

	/* wait pll stable */
	read_data = REG32(0xd4282800 + 0x3b4);
	while ((read_data & 0x30000) != 0x30000) {
		read_data = REG32(0xd4282800 + 0x3b4);

	}

	return;
}

void ddr_dfc(unsigned freqNo)
{
	unsigned read_data;

	REG32(0xd4282800 + 0x098) |= 0x10;
	REG32(0xc0000000 + 0x148) = 0x80ac0000;
	switch (freqNo) {
	case 0:
		LogMsg(0, "change to 1200 \n");
		REG32(0xd4282800 + 0x3b4) = 0x00003B50;
		REG32(0xd4282800 + 0x0b0) =
			(1 << TOP_DDRPHY0_EN_OFFSET) |
			(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
			(1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) |
			(0x0 << TOP_MC_REQ_TABLE_NUM_OFFSET);
		break;

	case 1:
		LogMsg(0, "change to 1600 \n");
		REG32(0xd4282800 + 0x3b4) = 0x00003B04;
		REG32(0xd4282800 + 0x0b0) =
			(1 << TOP_DDRPHY0_EN_OFFSET) |
			(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
			(1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) |
			(0x1 << TOP_MC_REQ_TABLE_NUM_OFFSET);
		break;

	case 2:
		LogMsg(0, "change to 2400 \n");
		REG32(0xd4282800 + 0x3b4) = 0x00003b40;
		REG32(0xd4282800 + 0x0b0) =
			(1 << TOP_DDRPHY0_EN_OFFSET) |
			(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
			(1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) |
			(0x2 << TOP_MC_REQ_TABLE_NUM_OFFSET);
		break;

	case 3:
		LogMsg(0, "change to 3200 \n");
		REG32(0xd4282800 + 0x3b4) = 0x00003b00;
		REG32(0xd4282800 + 0x0b0) =
			(1 << TOP_DDRPHY0_EN_OFFSET) |
			(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
			(1 << TOP_FREQ_PLL_CHG_MODE_OFFSET) |
			(0x3 << TOP_MC_REQ_TABLE_NUM_OFFSET);
		break;

	case 4:
		LogMsg(0, "change to ext clock\n");
		REG32(0xd4282800 + 0x3b4) = 0x00003b02;
		REG32(0xd4282800 + 0x0b0) =
			(1 << TOP_DDRPHY0_EN_OFFSET) |
			(1 << TOP_DCLK_BYPASS_RST_OFFSET) |
			(1 << TOP_DCLK_BYPASS_CLK_EN_OFFSET) |
			(1 << TOP_DCLK_BYPASS_DIV_OFFSET);
		break;

	default:
		LogMsg(0, "no this case\n");
		break;
	}

	REG32(0xd4282800 + 0x004) = (0x1 << TOP_DDR_FREQ_CHG_REQ);
	read_data = REG32(0xd4282800 + 0x004);
	while ((read_data & 0x400000) != 0x0) {
		read_data = REG32(0xd4282800 + 0x004);
	}
	LogMsg(0, "frequency change done!!!! \n");

	return;
}

void mck6_sw_fc_top(unsigned freqNo)
{
	unsigned read_data;

	switch (freqNo) {
	case 0:
		/* 1200MT */
		REG32(0xd4282800 + 0x3b4) = 0x00003B50;
		break;

	case 1:
		/* 1600MT */
		REG32(0xd4282800 + 0x3b4) = 0x00003B04;
		break;

	case 2:
		/* 2400MT */
		REG32(0xd4282800 + 0x3b4) = 0x00003B40;
		break;

	case 3:
		/* 3200MT */
		REG32(0xd4282800 + 0x3b4) = 0x00003B00;
		break;

	case 4:
		LogMsg(0, "sw frequency change to ext clk!!!! \n");
		REG32(0xd4282800 + 0x3b4) = 0x00003B02;
		LogMsg(0, "sw frequency change to ext clk done!!!! \n");
		break;

	default:
		LogMsg(0, "not support frequency change !!!! \n");
		return;

	}

	REG32(0xd4282800 + 0x0b0) = 0x40600400;
	REG32(0xd4282800 + 0x004) = 0x04000000;
	do {
		read_data = REG32(0xd4282800 + 0x004);
	} while ((read_data & 0x4000000) != 0x0);
}

void fp_timing_init(unsigned DDRC_BASE)
{
	unsigned int read_data=0;

	REG32(DDRC_BASE+MC_CH0_BASE+0x0104)= 0xF0800400;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00000E20;
	REG32(DDRC_BASE+MC_CH0_BASE+0x010c)= 0x19194314;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x20440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x20440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00000030;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0190) = 0x06400030;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x80e001c0;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000C005E;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0198) = 0x01CC01CC;
	REG32(DDRC_BASE+MC_CH0_BASE+0x019c) = 0x00181818;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a0) = 0x08180C0C;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a4) = 0x00000003;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a8) = 0x00000217;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01ac) = 0x30651D44;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b0) = 0x1120080F;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b4) = 0x08001000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b8) = 0x00000C00;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01bc) = 0x02020404;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c0) = 0x10000004;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000006;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01d8) = 0x00010190;
	REG32(DDRC_BASE+MC_CH0_BASE+0x014c) = 0x000c4090;
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x15000A02;
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03ec) = 0x0000046c;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0104)= 0xA0800400;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x00000C18;
	REG32(DDRC_BASE+MC_CH0_BASE+0x010c)= 0x9d194314;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00430000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0190) = 0x05350028;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x80A80151;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000C005E;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0198) = 0x017F017F;
	REG32(DDRC_BASE+MC_CH0_BASE+0x019c) = 0x00141414;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a0) = 0x07140A0A;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a4) = 0x00000003;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a8) = 0x00000213;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01ac) = 0x36541838;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b0) = 0x1c180a18;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b4) = 0x08000E00;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b8) = 0x00000E00;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01bc) = 0x02020404;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c0) = 0x10000004;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000004;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01d8) = 0x0000D94E;
	REG32(DDRC_BASE+MC_CH0_BASE+0x014c) = 0x0007204a;
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x13000802;
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03ec) = 0x00000450;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0104)= 0x50800400;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x0000080e;
	REG32(DDRC_BASE+MC_CH0_BASE+0x010c)= 0x9d194314;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00280018;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0190) = 0x03200018;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x807000e0;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000C005E;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0198) = 0x00e600e6;
	REG32(DDRC_BASE+MC_CH0_BASE+0x019c) = 0x000c0c0c;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a0) = 0x050c0606;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a4) = 0x00000003;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a8) = 0x0000020c;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01ac) = 0x18330f22;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b0) = 0x110f080f;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b4) = 0x08000800;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b8) = 0x00000600;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01bc) = 0x02020404;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c0) = 0x00000003;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000003;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01d8) = 0x00008190;
	REG32(DDRC_BASE+MC_CH0_BASE+0x014c) = 0x00030848;
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x0a000402;
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03ec) = 0x00000480;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0104)= 0x00800400;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0100)= 0x0000080e;
	REG32(DDRC_BASE+MC_CH0_BASE+0x010c)= 0x9d194314;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0110)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0114)= 0x00440000;
	REG32(DDRC_BASE+MC_CH0_BASE+0x018c) = 0x00280018;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0190) = 0x03200018;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0194) = 0x805400A8;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01fc) = 0x000C005E;
	REG32(DDRC_BASE+MC_CH0_BASE+0x0198) = 0x00e600e6;
	REG32(DDRC_BASE+MC_CH0_BASE+0x019c) = 0x000c0c0c;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a0) = 0x050c0606;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a4) = 0x00000003;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01a8) = 0x0000020c;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01ac) = 0x18330f22;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b0) = 0x110f080f;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b4) = 0x08000800;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01b8) = 0x00000600;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01bc) = 0x02020404;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c0) = 0x00000002;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01c4) = 0x00000003;
	REG32(DDRC_BASE+MC_CH0_BASE+0x01d8) = 0x00008190;
	REG32(DDRC_BASE+MC_CH0_BASE+0x014c) = 0x00030848;
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03e4) = 0x0a000402;
	REG32(DDRC_BASE+MC_CH0_PHY_BASE+0x03ec) = 0x00000480;

	read_data=REG32(DDRC_BASE+MC_CH0_BASE+0x0108);
	read_data &= 0xF00FFFFF;
	read_data |= (0x10<<20);
	REG32(DDRC_BASE+MC_CH0_BASE+0x0108)=read_data;

	return;
}

void fp_sel(unsigned DDRC_BASE,unsigned int fp)
{
	uint32_t data;
	data = REG32(DDRC_BASE + MC_CH0_BASE + 0x0104);
	data &= ~(0xf << 28);
	data |= (fp << 28) | (fp << 30);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = data;
	LogMsg(0, "ADDR[0x%08x]=0x%08x !!!! \n", (DDRC_BASE + MC_CH0_BASE + 0x104), REG32(DDRC_BASE + MC_CH0_BASE + 0x104));

	return;
}

void init_table_mc_tim (uint32_t ddrc_base, uint32_t *idx) {
        uint32_t i;
        uint32_t read_data;
        uint32_t mc_ch0_phy_base = 0x1000;
        volatile unsigned addrs[ ] = {
                MC_CH0_BASE+0x0100,
                MC_CH0_BASE+0x010c,
                MC_CH0_BASE+0x0110,
                MC_CH0_BASE+0x0114,
                MC_CH0_BASE+0x018c,
                MC_CH0_BASE+0x0190,
                MC_CH0_BASE+0x0194,
                MC_CH0_BASE+0x0198,
                MC_CH0_BASE+0x019c,
                MC_CH0_BASE+0x01a0,
                MC_CH0_BASE+0x01a4,
                MC_CH0_BASE+0x01a8,
                MC_CH0_BASE+0x01ac,
                MC_CH0_BASE+0x01b0,
                MC_CH0_BASE+0x01b4,
                MC_CH0_BASE+0x01b8,
                MC_CH0_BASE+0x01bc,
                MC_CH0_BASE+0x01c0,
                MC_CH0_BASE+0x01c4,
                MC_CH0_BASE+0x0200,
                MC_CH0_BASE+0x01d8,
                MC_CH0_BASE+0x014c,
                mc_ch0_phy_base+0x03e4,
                mc_ch0_phy_base+0x03ec
        };
        uint32_t tim_size = sizeof(addrs)>>2;

        for(i=0;i<tim_size;i++) {
                read_data = REG32(ddrc_base + addrs[i]);
                REG32(ddrc_base + 0x0074) = read_data;
                REG32(ddrc_base + 0x0078) = addrs[i];
                REG32(ddrc_base + 0x0070) = (*idx)++;
        }
}


void init_table_mc_a0(uint32_t ddrc_base)
{
	uint32_t idx = 0x200;
	uint32_t i = 0;
	volatile unsigned mc_cfg2_addr = MC_CH0_BASE + 0x0104;
	uint32_t temp_data, mc_cfg2_org, mc_cfg2_fp, mc_ctl0_org;
	volatile unsigned addrs[] = {
		0x0048,
		0x0054,
		0x0058,
		0x0060,
		0x0064,
		0x0148,
		0x014c,
		MC_CH0_BASE + 0x0000,
		MC_CH0_BASE + 0x0004,
		MC_CH0_BASE + 0x0008,
		MC_CH0_BASE + 0x000c,
		MC_CH0_BASE + 0x0020,
		MC_CH0_BASE + 0x0024,
		MC_CH0_BASE + 0x00c4,
		MC_CH0_BASE + 0x00c0,
		MC_CH0_BASE + 0x0180,
		MC_CH0_BASE + 0x0184,
		MC_CH0_BASE + 0x0188,
		0x80,
		0xa00,
		0xac0,
		0xacc,
	};

	mc_ctl0_org = REG32(ddrc_base + 0x44);
	temp_data = mc_ctl0_org | (BIT(2) | BIT(12));
	REG32(ddrc_base + 0x74) = temp_data;
	REG32(ddrc_base + 0x78) = 0x00000044 | (0x1 << 16);
	REG32(ddrc_base + 0x70) = idx++;

	uint32_t cfg_size = sizeof(addrs) >> 2;
	for (i = 0; i < cfg_size; i++) {
		temp_data = REG32(ddrc_base + addrs[i]);
		REG32(ddrc_base + 0x74) = temp_data;
		REG32(ddrc_base + 0x78) = addrs[i] & 0xffff;
		REG32(ddrc_base + 0x70) = idx++;
	}

	mc_cfg2_org = REG32(ddrc_base + mc_cfg2_addr);
	temp_data = mc_cfg2_org;
	temp_data &= ~(0xf << 28);

	mc_cfg2_fp = temp_data;
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	mc_cfg2_fp = temp_data | (0x5 << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28);
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	mc_cfg2_fp = temp_data | (0xa << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28);
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	mc_cfg2_fp = temp_data | (0xf << 28);
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_fp;
	mc_cfg2_fp &= ~(0x3 << 28);
	REG32(ddrc_base + 0x0074) = mc_cfg2_fp;
	REG32(ddrc_base + 0x0078) = mc_cfg2_addr;
	REG32(ddrc_base + 0x0070) = idx++;
	init_table_mc_tim(ddrc_base, &idx);

	REG32(ddrc_base + 0x0074) = 0x00020200;
	REG32(ddrc_base + 0x0078) = 0x000013e0;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x13000010;
	REG32(ddrc_base + 0x0078) = 0x000013d0;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x00010000;
	REG32(ddrc_base + 0x0078) = 0x000033fc;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x00010000;
	REG32(ddrc_base + 0x0078) = 0x000033fc;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x13000008;
	REG32(ddrc_base + 0x0078) = 0x00000020;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x13000004;
	REG32(ddrc_base + 0x0078) = 0x00000020;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x13020000;
	REG32(ddrc_base + 0x0078) = 0x00000028;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x13000001;
	REG32(ddrc_base + 0x0078) = 0x000013d0;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x00008000;
	REG32(ddrc_base + 0x0078) = 0x000033fc;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x00008000;
	REG32(ddrc_base + 0x0078) = 0x000033fc;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x10000100;
	REG32(ddrc_base + 0x0078) = 0x000013d0;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x00008000;
	REG32(ddrc_base + 0x0078) = 0x000033fc;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x00008000;
	REG32(ddrc_base + 0x0078) = 0x000033fc;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x1302000d;
	REG32(ddrc_base + 0x0078) = 0x00000024;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = 0x13020003;
	REG32(ddrc_base + 0x0078) = 0x00000024;
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x0074) = mc_ctl0_org;
	REG32(ddrc_base + 0x0078) = 0x44 | (0x1 << 17);
	REG32(ddrc_base + 0x0070) = idx++;

	REG32(ddrc_base + 0x40000 + 0x10104) = 0x00001100;
	REG32(ddrc_base + 0x40000 + 0x10108) = 0x00010000;
	REG32(ddrc_base + 0x40000 + 0x10100) = 0x00000020;
	REG32(ddrc_base + 0x40000 + 0x10104) = 0x000000ff;
	REG32(ddrc_base + 0x40000 + 0x10108) = 0x0001001c;
	REG32(ddrc_base + 0x40000 + 0x10100) = 0x00000021;
	REG32(ddrc_base + 0x40000 + 0x10104) = 0x00000000;
	REG32(ddrc_base + 0x40000 + 0x10108) = 0x0005001c;
	REG32(ddrc_base + 0x40000 + 0x10100) = 0x00000022;
	REG32(ddrc_base + mc_cfg2_addr) = mc_cfg2_org;
}

void ddr_dfc_table_init(unsigned int DDRC_BASE)
{
	REG32(DDRC_BASE + 0x74) = 0x00040303;
	REG32(DDRC_BASE + 0x78) = 0x00000044;
	REG32(DDRC_BASE + 0x70) = 0x00000000;
	REG32(DDRC_BASE + 0x74) = 0x13000008;
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x00000001;
	REG32(DDRC_BASE + 0x74) = 0x13010000;
	REG32(DDRC_BASE + 0x78) = 0x00000028;
	REG32(DDRC_BASE + 0x70) = 0x00000002;
	REG32(DDRC_BASE + 0x74) = 0x1302000d;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000003;
	REG32(DDRC_BASE + 0x74) = 0x13020001;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000004;
	REG32(DDRC_BASE + 0x74) = 0x13020002;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000005;
	REG32(DDRC_BASE + 0x74) = 0x13020003;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000006;
	REG32(DDRC_BASE + 0x74) = 0x1302000b;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000007;
	REG32(DDRC_BASE + 0x74) = 0x1302000c;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000008;
	REG32(DDRC_BASE + 0x74) = 0x1302000e;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000009;
	REG32(DDRC_BASE + 0x74) = 0x13020016;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x0000000a;
	REG32(DDRC_BASE + 0x74) = 0x13008000;
	REG32(DDRC_BASE + 0x78) = 0x00000028;
	REG32(DDRC_BASE + 0x70) = 0x0000000b;
	REG32(DDRC_BASE + 0x74) = 0x1302000d;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x0000000c;
	REG32(DDRC_BASE + 0x74) = 0x13000010;
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x0000000d;
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000000e;
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000000f;
	REG32(DDRC_BASE + 0x74) = 0x13000001;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000010;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000011;
	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000012;
	REG32(DDRC_BASE + 0x74) = 0x00040303;
	REG32(DDRC_BASE + 0x78) = 0x00010044;
	REG32(DDRC_BASE + 0x70) = 0x00000013;
	REG32(DDRC_BASE + 0x74) = 0x10000100;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000014;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000015;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000016;
	REG32(DDRC_BASE + 0x74) = 0x13000004;
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x00000017;
	REG32(DDRC_BASE + 0x74) = 0x1302000d;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x00000018;
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000019;
	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000001a;
	REG32(DDRC_BASE + 0x74) = 0x00040380;
	REG32(DDRC_BASE + 0x78) = 0x00020044;
	REG32(DDRC_BASE + 0x70) = 0x0000001b;
	REG32(DDRC_BASE + 0x74) = 0x00040380;
	REG32(DDRC_BASE + 0x78) = 0x00020044;
	REG32(DDRC_BASE + 0x70) = 0x0000012e;
	REG32(DDRC_BASE + 0x74) = 0x00040b43;
	REG32(DDRC_BASE + 0x78) = 0x00000044;
	REG32(DDRC_BASE + 0x70) = 0x00000180;
	REG32(DDRC_BASE + 0x74) = 0x13000010;
	REG32(DDRC_BASE + 0x78) = 0x00000020;
	REG32(DDRC_BASE + 0x70) = 0x00000181;
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000182;
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x00000183;
	REG32(DDRC_BASE + 0x74) = 0x13000001;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000184;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000185;
	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000186;
	REG32(DDRC_BASE + 0x74) = 0x00040b43;
	REG32(DDRC_BASE + 0x78) = 0x00010044;
	REG32(DDRC_BASE + 0x70) = 0x00000187;
	REG32(DDRC_BASE + 0x74) = 0x10000100;
	REG32(DDRC_BASE + 0x78) = 0x000013d0;
	REG32(DDRC_BASE + 0x70) = 0x00000188;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x00000189;
	REG32(DDRC_BASE + 0x74) = 0x00008000;
	REG32(DDRC_BASE + 0x78) = 0x000033fc;
	REG32(DDRC_BASE + 0x70) = 0x0000018a;
	REG32(DDRC_BASE + 0x74) = 0x1302000d;
	REG32(DDRC_BASE + 0x78) = 0x00000024;
	REG32(DDRC_BASE + 0x70) = 0x0000018b;
	REG32(DDRC_BASE + 0x74) = 0x00000002;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000018c;
	REG32(DDRC_BASE + 0x74) = 0x00000000;
	REG32(DDRC_BASE + 0x78) = 0x00002008;
	REG32(DDRC_BASE + 0x70) = 0x0000018d;
	REG32(DDRC_BASE + 0x74) = 0x00040b00;
	REG32(DDRC_BASE + 0x78) = 0x00020044;
	REG32(DDRC_BASE + 0x70) = 0x0000018e;
}

void top_DDR_MC_init(unsigned DDRC_BASE, unsigned int fp)
{
	REG32(DDRC_BASE + 0x44) = 0x00040300;
	REG32(DDRC_BASE + 0x48) = 0x00000001;
	REG32(DDRC_BASE + 0x64) = 0x100d0803;
	REG32(DDRC_BASE + 0x50) = 0x000000ff;
	REG32(DDRC_BASE + 0x58) = 0x3fd53fd5;
	REG32(DDRC_BASE + 0x180) = 0x00010200;
	REG32(DDRC_BASE + MC_CH0_BASE) = 0x100001;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x4) = 0x0;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x8) = 0x100001;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc) = 0x1;
	REG32(DDRC_BASE + 0x0080) = 0x00000000;
	REG32(DDRC_BASE + 0x0a00) = 0x00000000;
	REG32(DDRC_BASE + 0x0ac0) = 0x00000000;
	REG32(DDRC_BASE + 0x0acc) = 0xffffffff;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x20) = 0x05030732;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x24) = 0x05030732;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc0) = 0x14008000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc4) = 0x000000b8;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc8) = 0x0000FFFF;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xcc) = 0x200;
	fp_timing_init(DDRC_BASE);
	fp_sel(DDRC_BASE, fp);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0180) = 0x30D400;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0184) = 0x4E200;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0188) = 0xC800000;
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3E0) |= fp << 2;

	return;
}
void top_DDR_wr_ds_odt_vref(unsigned DPHY0_BASE,unsigned combination)
{
	unsigned data = 0;
	unsigned d_reg2 = 0;

	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0xc);
	switch (combination) {
	case 2:
		d_reg2 = 0xd8;
		data &= 0xFFFF00FF;
		data |= (d_reg2 << 8);
		REG32(DPHY0_BASE + COMMON_OFFSET + 0xc) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;

		break;

	default:
		LogMsg(0, "not support.....\n");
		break;
	}
}

void top_DDR_rx_ds_odt_vref(unsigned DPHY0_BASE,unsigned combination)
{
	unsigned data = 0;
	unsigned d_reg3 = 0;
	unsigned rx_ref_d1 = 0x0, rx_ref_d2 = 0x0;

	switch (combination) {
	case 2:
		data = REG32(DPHY0_BASE + COMMON_OFFSET + 0xc);
		data &= 0xFF00FFFF;
		d_reg3 = 0xE4;
		data |= (d_reg3 << 16);
		REG32(DPHY0_BASE + COMMON_OFFSET + 0xc) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0xc) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0xc) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0xc) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0xc) = data;

		data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x4);
		data &= 0x0000FFFF;
		rx_ref_d1 = 0x55;
		rx_ref_d2 = 0x55;
		data |= (rx_ref_d1 << 16) | (rx_ref_d2 << 24);
		REG32(DPHY0_BASE + COMMON_OFFSET + 0x4) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
		REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
		REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;
		break;

	default:
		LogMsg(0, "not support.....\n");
		break;
	}
}

void top_DDR_amble_config(unsigned DPHY0_BASE)
{
	unsigned data = 0;
	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x4);
	data &= 0xFFFF0FFF;
	data |= (1 << 11) | (1 << 13) | (1 << 15);
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x4) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x4) = data;

	return;
}

void top_DDR_phy_init(unsigned DDRC_BASE,unsigned fp)
{
	unsigned DPHY0_BASE = DDRC_BASE + 0x040000;
	unsigned data = 0;

	REG32(0xd4282800 + 0x3A4) &= 0xFFFF00FF;
	REG32(0xd4282800 + 0x3A4) |= (0xF << 8);
	REG32(0xd4282800 + 0x398) |= (0x3 << 10);
	REG32(DPHY0_BASE + COMMON_OFFSET) = 0x0;
	REG32(DPHY0_BASE + COMMON_OFFSET + subPHY_B_OFFSET) = 0x0;
	REG32(DPHY0_BASE + COMMON_OFFSET) = 0x1;
	REG32(DPHY0_BASE + COMMON_OFFSET + subPHY_B_OFFSET) = 0x1;
	REG32(DPHY0_BASE + 0x0064) = 0x4349;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET + 0x0064) = 0x4349;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 2 + 0x064) = 0x4349;
	REG32(DPHY0_BASE + FREQ_POINT_OFFSET * 3 + 0x064) = 0x4349;
	top_DDR_amble_config(DPHY0_BASE);
	top_DDR_wr_ds_odt_vref(DPHY0_BASE, 2);
	top_DDR_rx_ds_odt_vref(DPHY0_BASE, 2);

	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x14);
	data &= 0xFF9FFFEF;
	data |= (0x3 << 21);
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x14) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x14) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x14) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x14) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x14) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x14) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x14) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x14) = data;

	data = REG32(DPHY0_BASE + COMMON_OFFSET + 0x10);
	data |= 0x10000000;
	REG32(DPHY0_BASE + COMMON_OFFSET + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + 0x10) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET + 0x10) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 2 + 0x10) = data;
	REG32(DPHY0_BASE + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x10) = data;
	REG32(DPHY0_BASE + subPHY_B_OFFSET + COMMON_OFFSET + FREQ_POINT_OFFSET * 3 + 0x10) = data;

	REG32(DPHY0_BASE + COMMON_OFFSET + 0x30) = 0x1077;
	REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET + 0x24) = 0x0;
	REG32(DPHY0_BASE + OTHER_CONTROL_OFFSET) |= 0x1;

	return;
}

void top_Common_config(void)
{
	REG32(0xd4282800 + 0x39c) &= 0xFFFF00FF;
	REG32(0xd4282800 + 0x39c) |= (0x3B << 8);
	enable_PLL();
	mck6_sw_fc_top(BOOT_PP);
	REG32(0xd42828e8) &= 0xFFFFFFFC;
	REG32(0xd42828e8) |= 0x3;

	return;
}

void top_DDR_MC_Phy_Device_Init(unsigned int DDRC_BASE,unsigned int cs_val,unsigned int fp)
{
	unsigned DFI_PHY_USER_COMMAND_0 = DDRC_BASE + 0x13D0;
	__maybe_unused unsigned DPHY0_BASE = DDRC_BASE + 0x40000;
	unsigned read_data = 0;
	unsigned cs_num;

	if (cs_val == 1) {
		cs_num = 0x1;
	} else {
		cs_num = 0x3;
	}

	top_DDR_MC_init(DDRC_BASE, fp);
	top_DDR_phy_init(DDRC_BASE, fp);

	REG32(DFI_PHY_USER_COMMAND_0) = 0x13000001;
	read_data = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3fc);
	while ((read_data & 0x80000000) != 0x80000000) {
		read_data = REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x3fc);
	}
	LogMsg(0, "PHY INIT done \n");

	REG32(DFI_PHY_USER_COMMAND_0) = 0x13000100;
	REG32(DDRC_BASE + 0x20) = (0x10000001 | (cs_num << 24));

	LogMsg(0, "wait DRAM INIT \n");
	read_data = REG32(DDRC_BASE + 0x8);
	while ((read_data & 0x00000011) != 0x00011) {
		read_data = REG32(DDRC_BASE + 0x8);
	}
	LogMsg(0, "DRAM INIT done \n");

	REG32(DDRC_BASE + 0x24) = (0x10020001 | (cs_num << 24));
	REG32(DDRC_BASE + 0x24) = (0x10020002 | (cs_num << 24));
	REG32(DDRC_BASE + 0x24) = (0x1002000d | (cs_num << 24));
	REG32(DDRC_BASE + 0x24) = (0x10020003 | (cs_num << 24));
	REG32(DDRC_BASE + 0x24) = (0x10020016 | (cs_num << 24));

	REG32(DDRC_BASE + 0x20) = 0x11002000;
	REG32(DDRC_BASE + 0x20) = 0x11001000;
	if (cs_val != 1) {
		REG32(DDRC_BASE + 0x20) = 0x12002000;
		REG32(DDRC_BASE + 0x20) = 0x12001000;
	}

	REG32(DDRC_BASE + 0x24) = (0x1002000C | (cs_num << 24));
	REG32(DDRC_BASE + 0x24) = (0x1002000E | (cs_num << 24));
	REG32(DDRC_BASE + 0x24) = (0x1002000B | (cs_num << 24));
	REG32(DDRC_BASE + 0x24) = (0x10020017 | (cs_num << 24));
	LogMsg(0, "DRAM Mode register Init done.....\n");

	return;
}

void adjust_timing(u32 DDRC_BASE)
{
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0xF0800400;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110)= 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114)= 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01b0) = 0x221D0C1D;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01fc) = 0x000C005E;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0xA0800400;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0100) = 0x00000C1C;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110)= 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114)= 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01fc) = 0x000C005E;
	REG32(DDRC_BASE + MC_CH0_PHY_BASE + 0x03e4) = 0x13000802;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0x50800400;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110) = 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114) = 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01fc) = 0x000C005E;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0104) = 0x00800400;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0110) = 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x0114) = 0x40440000;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x01fc) = 0x000C005E;
}

void adjust_mapping(u32 DDRC_BASE, u32 cs_num, u32 size_mb, u32 mr8_value)
{
	u32 area_length_mb, area_length_cfg;
	u32 cs1_start_addr_l, cs1_start_addr_h;
	u32 io_width, density;
	u32 i, read_data;
	const struct addrmap_info *addrmap = &ddr_addrmap[13]; 

	area_length_mb = size_mb / cs_num;
	// area_length_mb = size_mb >> (cs_num -1);
	switch (area_length_mb) {
	case 1024: // 1GB
		area_length_cfg = 0xE;
		break;
	case 2048: // 2GB
		area_length_cfg = 0xF;
		break;
	case 4096: // 4GB
		area_length_cfg = 0x10;
		break;
	case 8192: // 8GB
		area_length_cfg = 0x11;
		break;
	case 16384: // 16GB
		area_length_cfg = 0x12;
		break;
	default:
		pr_err("do not support such area length =0x%x MB\n", area_length_mb);
		area_length_cfg = 0x10;
		break;
	}

	cs1_start_addr_l = ((area_length_mb >> 3) & 0x1FF);
	cs1_start_addr_h = ((area_length_mb >> 12) & 0xFFFFFFFF);

	io_width = (mr8_value >> 6);
	density = (mr8_value >> 2) & 0xf;

	for (i = 0; i < ARRAY_SIZE(ddr_addrmap); i++) {
		if ((io_width == ddr_addrmap[i].io_width_per_channel) && (density == ddr_addrmap[i].density_per_channel) ) {
			addrmap = &ddr_addrmap[i];
			break;
		}
	}

	read_data = REG32(DDRC_BASE + MC_CH0_BASE);
	read_data &= 0x0060FFFF;
	read_data |= (area_length_cfg << 16);
	REG32(DDRC_BASE + MC_CH0_BASE) = read_data;
	REG32(DDRC_BASE + MC_CH0_BASE + 0x4) = 0x0;

	read_data = REG32(DDRC_BASE + MC_CH0_BASE + 0x8);
	read_data &= 0x0060FFFF;
	read_data |= (cs1_start_addr_l << 23) |(area_length_cfg << 16);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x8) = read_data;
	REG32(DDRC_BASE + MC_CH0_BASE + 0xc) = cs1_start_addr_h;

	read_data = REG32(DDRC_BASE + MC_CH0_BASE + 0x20);
	read_data &= 0xFFFFF00C;
	read_data |= (addrmap->row_num << 8) | (addrmap->col_num << 4) | (addrmap->bank_num);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x20) = read_data;

	read_data = REG32(DDRC_BASE + MC_CH0_BASE + 0x24);
	read_data &= 0xFFFFF00C;
	read_data |= (addrmap->row_num << 8) | (addrmap->col_num << 4) | (addrmap->bank_num);
	REG32(DDRC_BASE + MC_CH0_BASE + 0x24) = read_data;

//     REG32(DDRC_BASE + MC_CH0_BASE) = 0xf0001;
//     REG32(DDRC_BASE + MC_CH0_BASE + 0x4) = 0x0;
//     REG32(DDRC_BASE + MC_CH0_BASE + 0x8) = 0x800f0001;
//     REG32(DDRC_BASE + MC_CH0_BASE + 0xc) = 0x0;
//     REG32(DDRC_BASE + MC_CH0_BASE + 0x20) = 0x05030632;//8 bank, 17 row, 10 column
//     REG32(DDRC_BASE + MC_CH0_BASE + 0x24) = 0x05030632;//8 bank, 17 row, 10 column

	LogMsg("DEBUG-ADDR[0x%x]:0x%x\n", (DDRC_BASE + MC_CH0_BASE), REG32(DDRC_BASE + MC_CH0_BASE));
	LogMsg("DEBUG-ADDR[0x%x]:0x%x\n", (DDRC_BASE + MC_CH0_BASE + 0x4), REG32(DDRC_BASE + MC_CH0_BASE + 0x4));
	LogMsg("DEBUG-ADDR[0x%x]:0x%x\n", (DDRC_BASE + MC_CH0_BASE + 0x8), REG32(DDRC_BASE + MC_CH0_BASE + 0x8));
	LogMsg("DEBUG-ADDR[0x%x]:0x%x\n", (DDRC_BASE + MC_CH0_BASE + 0xc), REG32(DDRC_BASE + MC_CH0_BASE + 0xc));
	LogMsg("DEBUG-ADDR[0x%x]:0x%x\n", (DDRC_BASE + MC_CH0_BASE + 0x20), REG32(DDRC_BASE + MC_CH0_BASE + 0x20));
	LogMsg("DEBUG-ADDR[0x%x]:0x%x\n", (DDRC_BASE + MC_CH0_BASE + 0x24), REG32(DDRC_BASE + MC_CH0_BASE + 0x24));
}
__maybe_unused static int printf_no_output(const char *fmt, ...)
{
        return 0;
}

static void top_training_fp_all(u32 ddr_base, u32 cs_num, u32 boot_pp, void *input)
{
	u64 to_traning_param[10];
	int (*func)(const char*, ...) = printf;
	void (*training)(void* param);
	unsigned long flush_lenth;

	#if !(LOGLEVEL > 0)
	func = printf_no_output;
	#endif

	to_traning_param[0] = ddr_base;
	to_traning_param[1] = cs_num;
	to_traning_param[2] = boot_pp;
	to_traning_param[3] = (u64)func;
	to_traning_param[4] = (u64)input;
	memcpy((void*)DDR_TRAINING_DATA_BASE, lpddr4_training_img, sizeof(lpddr4_training_img));
	flush_lenth = round_up(sizeof(lpddr4_training_img), CONFIG_RISCV_CBOM_BLOCK_SIZE);
	flush_dcache_range(DDR_TRAINING_DATA_BASE, DDR_TRAINING_DATA_BASE + flush_lenth);

	training = (void (*)(void * param))DDR_TRAINING_DATA_BASE;
	training(to_traning_param);
}

void lpddr4_silicon_init(u32 ddr_base, u32 data_rate)
{
	u32 fp=0;
	u32 size_mb, mr8_value, cs_num;;
	struct ddr_training_info_t *info;

	cs_num = ddr_cs_num;
	info = (struct ddr_training_info_t*)map_sysmem(DDR_TRAINING_INFO_BUFF, 0);
	top_Common_config();

	top_DDR_MC_Phy_Device_Init(ddr_base, cs_num, 0);

	size_mb = ddr_get_density();
	mr8_value = ddr_get_mr8();
	adjust_mapping(ddr_base, cs_num, size_mb, mr8_value);
	LogMsg(0,"ddr density: %u MB \n", size_mb);

	ddr_dfc_table_init(0xF0000000);
	init_table_mc_a0(0xF0000000);

	top_training_fp_all(ddr_base, cs_num, 0, info->para);

	fp=1;
	ddr_dfc(fp);
	top_training_fp_all(ddr_base, cs_num, fp, info->para);

	fp=2;
	ddr_dfc(fp);
	top_training_fp_all(ddr_base, cs_num, fp, info->para);

	/* change dram frequency */
	switch(data_rate) {
	case 1600:
		ddr_dfc(1);
		break;

	case 2400:
		ddr_dfc(2);
		break;

	case 1200:
	default:
		data_rate = 1200;
		ddr_dfc(0);
		break;
	}

	return;
}

#endif

