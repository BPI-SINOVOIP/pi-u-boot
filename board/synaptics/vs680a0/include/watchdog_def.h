/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Shaojun Feng <sfeng@synaptics.com>
 */

#ifndef _WATCHDOG_DEF_H_
#define _WATCHDOG_DEF_H_

/* Watch dog registers*/
#define SOC_SM_SYS_CTRL_REG_BASE	0xF7FE2000

#define DW_WDT_CR			0x00
#define DW_WDT_TORR			0x04
#define DW_WDT_CRR			0x0C

#define DW_WDT_CR_EN_OFFSET		0x00
#define DW_WDT_CR_RMOD_OFFSET		0x01
#define DW_WDT_CRR_RESTART_VAL		0x76
#define DW_WDT_CR_RPL_OFFSET		0x02
#define DW_WDT_CR_RPL_MASK		0x07
#define DW_WDT_CR_RPL_PCLK_8		0x02

#define RA_smSysCtl_SM_WDT_MASK 	0x003c
#endif
