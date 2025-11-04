// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2021, Renesas Electronics Corporation. All rights reserved.
 */

#include <common.h>
#include "ddr_internal.h"

const uint32_t mc_odt_pins_tbl[4] = {
	1,
	0,
	0,
	0
};

const uint32_t mc_mr1_tbl[2] = {
	0x0706,
	0x0100
};

const uint32_t mc_mr2_tbl[2] = {
	0x0E00,
	0x0000
};

const uint32_t mc_mr5_tbl[2] = {
	0x01C0,
	0x01C0
};

const uint32_t mc_mr6_tbl[2] = {
	0x007F,
	0x000F
};

const uint32_t mc_phy_settings_tbl[MC_PHYSET_NUM][2] = {
	{DDRMC_R040,	0x57630BB8},
	{DDRMC_R041,	0x00002828},
	{DDRMC_R042,	0x00003C22},
	{DDRMC_R043,	0x00102611},
};

const uint32_t swizzle_mc_tbl[SWIZZLE_MC_NUM][2] = {
	{DDRMC_R030,	0x00000000},
	{DDRMC_R031,	0x00000000},
	{DDRMC_R032,	0x1C191E13},
	{DDRMC_R033,	0x12000A18},
	{DDRMC_R034,	0x100B1D00},
	{DDRMC_R035,	0x1F0C1B09},
	{DDRMC_R036,	0x151A1614},
	{DDRMC_R037,	0x000D0E17},
	{DDRMC_R038,	0x00000000}
};

const uint32_t swizzle_phy_tbl[SIZZLE_PHY_NUM][2] = {
	{DDRPHY_R29,	0x00000000},
	{DDRPHY_R11,	0x0A070E16},
	{DDRPHY_R29,	0x00000001},
	{DDRPHY_R11,	0x18140B08},
	{DDRPHY_R29,	0x00000002},
	{DDRPHY_R11,	0x00170409},
	{DDRPHY_R29,	0x00000003},
	{DDRPHY_R11,	0x0F030619},
	{DDRPHY_R29,	0x00000004},
	{DDRPHY_R11,	0x13121110},
	{DDRPHY_R29,	0x00000005},
	{DDRPHY_R11,	0x05020C0D},
	{DDRPHY_R29,	0x00000006},
	{DDRPHY_R11,	0x1B1A1501},
	{DDRPHY_R29,	0x00000007},
	{DDRPHY_R11,	0x001E1D1C}
};
