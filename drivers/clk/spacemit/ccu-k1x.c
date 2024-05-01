// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit k1x clock controller driver
 *
 * Copyright (c) 2023, spacemit Corporation.
 *
 */

#include <dt-bindings/clock/spacemit-k1x-clock.h>
#include <linux/clk-provider.h>
#include <common.h>
#include <clk.h>
#include <clk-uclass.h>
#include <dm.h>
#include <log.h>
#include <asm/io.h>
#include "ccu-k1x.h"
#include "ccu_mix.h"
#include "ccu_pll.h"
#include "ccu_ddn.h"

/* APBS register offset */
//pll1
#define APB_SPARE1_REG          0x100
#define APB_SPARE2_REG          0x104
#define APB_SPARE3_REG          0x108
//pll2
#define APB_SPARE7_REG          0x118
#define APB_SPARE8_REG          0x11c
#define APB_SPARE9_REG          0x120
//pll3
#define APB_SPARE10_REG         0x124
#define APB_SPARE11_REG         0x128
#define APB_SPARE12_REG         0x12c
/* end of APBS register offset */

/* APBC register offset */
#define APBC_UART1_CLK_RST      0x0
#define APBC_UART2_CLK_RST      0x4
#define APBC_GPIO_CLK_RST       0x8
#define APBC_PWM0_CLK_RST       0xc
#define APBC_PWM1_CLK_RST       0x10
#define APBC_PWM2_CLK_RST       0x14
#define APBC_PWM3_CLK_RST       0x18
#define APBC_TWSI8_CLK_RST      0x20
#define APBC_UART3_CLK_RST      0x24
#define APBC_RTC_CLK_RST        0x28 //reserved
#define APBC_TWSI0_CLK_RST      0x2c
#define APBC_TWSI1_CLK_RST      0x30
#define APBC_TIMERS1_CLK_RST    0x34
#define APBC_TWSI2_CLK_RST      0x38
#define APBC_AIB_CLK_RST        0x3c
#define APBC_TWSI4_CLK_RST      0x40
#define APBC_TIMERS2_CLK_RST    0x44
#define APBC_ONEWIRE_CLK_RST    0x48
#define APBC_TWSI5_CLK_RST      0x4c
#define APBC_DRO_CLK_RST        0x58
#define APBC_IR_CLK_RST         0x5c
#define APBC_TWSI6_CLK_RST      0x60
#define APBC_COUNTER_CLK_SEL    0x64

#define APBC_TWSI7_CLK_RST      0x68
#define APBC_TSEN_CLK_RST       0x6c

#define APBC_UART4_CLK_RST      0x70
#define APBC_UART5_CLK_RST      0x74
#define APBC_UART6_CLK_RST      0x78
#define APBC_SSP3_CLK_RST       0x7c

#define APBC_SSPA0_CLK_RST      0x80
#define APBC_SSPA1_CLK_RST      0x84

#define APBC_IPC_AP2AUD_CLK_RST 0x90
#define APBC_UART7_CLK_RST      0x94
#define APBC_UART8_CLK_RST      0x98
#define APBC_UART9_CLK_RST      0x9c

#define APBC_CAN0_CLK_RST       0xa0
#define APBC_PWM4_CLK_RST       0xa8
#define APBC_PWM5_CLK_RST       0xac
#define APBC_PWM6_CLK_RST       0xb0
#define APBC_PWM7_CLK_RST       0xb4
#define APBC_PWM8_CLK_RST       0xb8
#define APBC_PWM9_CLK_RST       0xbc
#define APBC_PWM10_CLK_RST      0xc0
#define APBC_PWM11_CLK_RST      0xc4
#define APBC_PWM12_CLK_RST      0xc8
#define APBC_PWM13_CLK_RST      0xcc
#define APBC_PWM14_CLK_RST      0xd0
#define APBC_PWM15_CLK_RST      0xd4
#define APBC_PWM16_CLK_RST      0xd8
#define APBC_PWM17_CLK_RST      0xdc
#define APBC_PWM18_CLK_RST      0xe0
#define APBC_PWM19_CLK_RST      0xe4
/* end of APBC register offset */

/* MPMU register offset */
#define MPMU_POSR			0x10 //no define
#define POSR_PLL1_LOCK			BIT(27)
#define POSR_PLL2_LOCK			BIT(28)
#define POSR_PLL3_LOCK			BIT(29)

#define MPMU_VRCR			0x18 //no define
#define MPMU_VRCR_REQ_EN0		BIT(0)
#define MPMU_VRCR_REQ_EN2		BIT(2)
#define MPMU_VRCR_REQ_POL2		BIT(6)
#define MPMU_VRCR_VCXO_OUT_REQ_EN2	BIT(14)

#define MPMU_WDTPCR     0x200
#define MPMU_RIPCCR     0x210 //no define
#define MPMU_ACGR       0x1024
#define MPMU_SUCCR      0x14
#define MPMU_SUCCR_1	 0x10b0
#define MPMU_APBCSCR    0x1050

/* end of MPMU register offset */

/* APMU register offset */
#define APMU_JPG_CLK_RES_CTRL       0x20
#define APMU_CSI_CCIC2_CLK_RES_CTRL 0x24
#define APMU_ISP_CLK_RES_CTRL       0x38
#define APMU_LCD_CLK_RES_CTRL1      0x44
#define APMU_LCD_SPI_CLK_RES_CTRL   0x48
#define APMU_LCD_CLK_RES_CTRL2      0x4c
#define APMU_CCIC_CLK_RES_CTRL      0x50
#define APMU_SDH0_CLK_RES_CTRL      0x54
#define APMU_SDH1_CLK_RES_CTRL      0x58
#define APMU_USB_CLK_RES_CTRL       0x5c
#define APMU_QSPI_CLK_RES_CTRL      0x60
#define APMU_USB_CLK_RES_CTRL       0x5c
#define APMU_DMA_CLK_RES_CTRL       0x64
#define APMU_AES_CLK_RES_CTRL       0x68
#define APMU_VPU_CLK_RES_CTRL       0xa4
#define APMU_GPU_CLK_RES_CTRL       0xcc
#define APMU_SDH2_CLK_RES_CTRL      0xe0
#define APMU_PMUA_MC_CTRL           0xe8
#define APMU_PMU_CC2_AP             0x100
#define APMU_PMUA_EM_CLK_RES_CTRL   0x104

#define APMU_AUDIO_CLK_RES_CTRL     0x14c
#define APMU_HDMI_CLK_RES_CTRL      0x1B8
#define APMU_CCI550_CLK_CTRL        0x300
#define APMU_ACLK_CLK_CTRL          0x388
#define APMU_CPU_C0_CLK_CTRL        0x38C
#define APMU_CPU_C1_CLK_CTRL        0x390

#define APMU_PCIE_CLK_RES_CTRL_0    0x3cc
#define APMU_PCIE_CLK_RES_CTRL_1    0x3d4
#define APMU_PCIE_CLK_RES_CTRL_2    0x3dc

#define APMU_EMAC0_CLK_RES_CTRL     0x3e4
#define APMU_EMAC1_CLK_RES_CTRL     0x3ec
/* end of APMU register offset */

/* APBC2 register offset */
#define APBC2_UART1_CLK_RST		0x00
#define APBC2_SSP2_CLK_RST		0x04
#define APBC2_TWSI3_CLK_RST		0x08
#define APBC2_RTC_CLK_RST		0x0c
#define APBC2_TIMERS0_CLK_RST		0x10
#define APBC2_KPC_CLK_RST		0x14
#define APBC2_GPIO_CLK_RST		0x1c
/* end of APBC2 register offset */

struct spacemit_k1x_clk k1x_clock_controller;
struct clk vctcxo_24, vctcxo_3, vctcxo_1, pll1_vco, clk_32k, clk_dummy;

#ifdef CONFIG_SPL_BUILD
//apbs
static SPACEMIT_CCU_FACTOR(pll1_2457p6_vco, "pll1_2457p6_vco", "pll1_vco",
	1, 100);

//pll1
static SPACEMIT_CCU_GATE_FACTOR(pll1_d2, "pll1_d2", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(1), BIT(1), 0x0,
	2, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d4, "pll1_d4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(3), BIT(3), 0x0,
	4, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d6, "pll1_d6", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(5), BIT(5), 0x0,
	6, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d8, "pll1_d8", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(7), BIT(7), 0x0,
	8, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d23_106p8, "pll1_d23_106p8", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(20), BIT(20), 0x0,
	23, 1, 0);
//pll1_d6
static SPACEMIT_CCU_GATE_FACTOR(pll1_d12_204p8, "pll1_d12_204p8", "pll1_d6",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(5), BIT(5), 0x0,
	2, 1, 0);
static SPACEMIT_CCU_GATE(pll1_d6_409p6, "pll1_d6_409p6", "pll1_d6",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(0), BIT(0), 0x0,
	0);
//pll1_d4
static SPACEMIT_CCU_GATE_FACTOR(pll1_d78_31p5, "pll1_d78_31p5", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(6), BIT(6), 0x0,
	39, 2, 0);
//pll1_d2
static SPACEMIT_CCU_GATE(pll1_d2_1228p8, "pll1_d2_1228p8", "pll1_d2",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(16), BIT(16), 0x0,
	0);
//pll1_d8
static SPACEMIT_CCU_GATE_FACTOR(pll1_d24_102p4, "pll1_d24_102p4", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(12), BIT(12), 0x0,
	3, 1, 0);
//apbc
static const char *twsi_parent_names[] = {
	"pll1_d78_31p5",
};
static SPACEMIT_CCU_MUX_GATE(twsi6_clk, "twsi6_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI6_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(twsi8_clk, "twsi8_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI8_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(sdh_axi_aclk, "sdh_axi_aclk", NULL,
	BASE_TYPE_APMU, APMU_SDH0_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const sdh01_parent_names[] = {
	"pll1_d6_409p6",
 };
static SPACEMIT_CCU_DIV_FC_MUX_GATE(sdh0_clk, "sdh0_clk", sdh01_parent_names,
	BASE_TYPE_APMU, APMU_SDH0_CLK_RES_CTRL,
	8, 3, BIT(11),
	5, 3, BIT(4), BIT(4), 0x0,
	0);
static const char * const sdh2_parent_names[] = {
	"pll1_d6_409p6",
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(sdh2_clk, "sdh2_clk", sdh2_parent_names,
	BASE_TYPE_APMU, APMU_SDH2_CLK_RES_CTRL,
	8, 3, BIT(11),
	5, 3, BIT(4), BIT(4), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(usb_axi_clk, "usb_axi_clk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(usb_p1_aclk, "usb_p1_aclk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(5), BIT(5), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(usb30_clk, "usb30_clk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(8), BIT(8), 0x0,
	0);
static const char * const qspi_parent_names[] = {"clk_dummy", "clk_dummy", "clk_dummy",
		"clk_dummy", "clk_dummy", "pll1_d23_106p8"};
static SPACEMIT_CCU_DIV_MFC_MUX_GATE(qspi_clk, "qspi_clk", qspi_parent_names,
	BASE_TYPE_APMU, APMU_QSPI_CLK_RES_CTRL,
	9, 3, BIT(12),
	6, 3, BIT(4), BIT(4), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(qspi_bus_clk, "qspi_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_QSPI_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const aes_parent_names[] = {
	"clk_dummy", "pll1_d24_102p4"
};
static SPACEMIT_CCU_MUX_GATE(aes_clk, "aes_clk", aes_parent_names,
	BASE_TYPE_APMU, APMU_AES_CLK_RES_CTRL,
	6, 1, BIT(5), BIT(5), 0x0,
	0);

static u32 transfer_to_spl_list[][2] = {
	{CLK_TWSI6, CLK_TWSI6_SPL},
	{CLK_TWSI8, CLK_TWSI8_SPL},
	{CLK_SDH_AXI, CLK_SDH_AXI_SPL},
	{CLK_SDH0, CLK_SDH0_SPL},
	{CLK_SDH2, CLK_SDH2_SPL},
	{CLK_USB_P1, CLK_USB_P1_SPL},
	{CLK_USB_AXI, CLK_USB_AXI_SPL},
	{CLK_USB30, CLK_USB30_SPL},
	{CLK_QSPI, CLK_QSPI_SPL},
	{CLK_QSPI_BUS, CLK_QSPI_BUS_SPL},
	{CLK_AES, CLK_AES_SPL},
};

static struct spacemit_clk_table spacemit_k1x_clks = {
	.clks	= {
		[CLK_PLL1_2457P6_SPL]    = &pll1_2457p6_vco.common.clk,
		[CLK_PLL1_D2_SPL]        = &pll1_d2.common.clk,
		[CLK_PLL1_D4_SPL]        = &pll1_d4.common.clk,
		[CLK_PLL1_D6_SPL]        = &pll1_d6.common.clk,
		[CLK_PLL1_D23_SPL]       = &pll1_d23_106p8.common.clk,
		[CLK_PLL1_409P6_SPL] = &pll1_d6_409p6.common.clk,
		[CLK_PLL1_D8_SPL]		= &pll1_d8.common.clk,
		[CLK_PLL1_31P5_SPL]      = &pll1_d78_31p5.common.clk,
		[CLK_PLL1_1228_SPL]      = &pll1_d2_1228p8.common.clk,
		[CLK_TWSI6_SPL]      = &twsi6_clk.common.clk,
		[CLK_TWSI8_SPL]      = &twsi8_clk.common.clk,
		[CLK_SDH_AXI_SPL]        = &sdh_axi_aclk.common.clk,
		[CLK_SDH0_SPL]       = &sdh0_clk.common.clk,
		[CLK_SDH2_SPL]       = &sdh2_clk.common.clk,
		[CLK_USB_P1_SPL]     = &usb_p1_aclk.common.clk,
		[CLK_USB_AXI_SPL]        = &usb_axi_clk.common.clk,
		[CLK_USB30_SPL]      = &usb30_clk.common.clk,
		[CLK_QSPI_SPL]       = &qspi_clk.common.clk,
		[CLK_QSPI_BUS_SPL]       = &qspi_bus_clk.common.clk,
		[CLK_PLL1_204P8_SPL]	= &pll1_d12_204p8.common.clk,
		[CLK_PLL1_102P4_SPL]	= &pll1_d24_102p4.common.clk,
		[CLK_AES_SPL]		= &aes_clk.common.clk,
	},
	.num = CLK_MAX_NO_SPL,
};

ulong transfer_clk_id_to_spl(ulong id)
{
	u32 listsize = ARRAY_SIZE(transfer_to_spl_list);

	for (int i = 0; i < listsize; i++){
		if (id == transfer_to_spl_list[i][0]){
			pr_info("id:%ld, %d,\n", id, transfer_to_spl_list[i][1]);
			return transfer_to_spl_list[i][1];
		}
	}
	return id;
}
#else

//apbs
static SPACEMIT_CCU_FACTOR(pll1_2457p6_vco, "pll1_2457p6_vco", "pll1_vco",
	1, 100);

static const struct ccu_pll_rate_tbl pll2_rate_tbl[] = {
	PLL_RATE(3000000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3f, 0xe00000),
	PLL_RATE(3200000000UL, 0x67, 0xdd, 0x50, 0x00, 0x43, 0xeaaaab),
	PLL_RATE(2457600000UL, 0x64, 0xdd, 0x50, 0x00, 0x33, 0x0ccccd),
	PLL_RATE(2800000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3a, 0x155555),
};

static const struct ccu_pll_rate_tbl pll3_rate_tbl[] = {
	PLL_RATE(3000000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3f, 0xe00000),
	PLL_RATE(3200000000UL, 0x67, 0xdd, 0x50, 0x00, 0x43, 0xeaaaab),
	PLL_RATE(2457600000UL, 0x64, 0xdd, 0x50, 0x00, 0x33, 0x0ccccd),
};

static SPACEMIT_CCU_PLL(pll2, "pll2", &pll2_rate_tbl, ARRAY_SIZE(pll2_rate_tbl),
	BASE_TYPE_APBS, APB_SPARE7_REG, APB_SPARE8_REG, APB_SPARE9_REG,
	MPMU_POSR, POSR_PLL2_LOCK, 1,
	0);

static SPACEMIT_CCU_PLL(pll3, "pll3", &pll3_rate_tbl, ARRAY_SIZE(pll3_rate_tbl),
	BASE_TYPE_APBS, APB_SPARE10_REG, APB_SPARE11_REG, APB_SPARE12_REG,
	MPMU_POSR, POSR_PLL3_LOCK, 1,
	0);

//pll1
static SPACEMIT_CCU_GATE_FACTOR(pll1_d2, "pll1_d2", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(1), BIT(1), 0x0,
	2, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d3, "pll1_d3", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(2), BIT(2), 0x0,
	3, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d4, "pll1_d4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(3), BIT(3), 0x0,
	4, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d5, "pll1_d5", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(4), BIT(4), 0x0,
	5, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d6, "pll1_d6", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(5), BIT(5), 0x0,
	6, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d7, "pll1_d7", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(6), BIT(6), 0x0,
	7, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d8, "pll1_d8", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(7), BIT(7), 0x0,
	8, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d11_223p4, "pll1_d11_223p4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(15), BIT(15), 0x0,
	11, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d13_189, "pll1_d13_189", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(16), BIT(16), 0x0,
	13, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d23_106p8, "pll1_d23_106p8", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(20), BIT(20), 0x0,
	23, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d64_38p4, "pll1_d64_38p4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(0), BIT(0), 0x0,
	64, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_aud_245p7, "pll1_aud_245p7", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(10), BIT(10), 0x0,
	10, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_aud_24p5, "pll1_aud_24p5", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(11), BIT(11), 0x0,
	100, 1, 0);

//pll2
static SPACEMIT_CCU_GATE_FACTOR(pll2_d1, "pll2_d1", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(0), BIT(0), 0x0,
	1, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll2_d2, "pll2_d2", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(1), BIT(1), 0x0,
	2, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll2_d3, "pll2_d3", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(2), BIT(2), 0x0,
	3, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll2_d4, "pll2_d4", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(3), BIT(3), 0x0,
	4, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll2_d5, "pll2_d5", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(4), BIT(4), 0x0,
	5, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll2_d6, "pll2_d6", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(5), BIT(5), 0x0,
	6, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll2_d7, "pll2_d7", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(6), BIT(6), 0x0,
	7, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll2_d8, "pll2_d8", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(7), BIT(7), 0x0,
	8, 1, 0);

//pll3
static SPACEMIT_CCU_GATE_FACTOR(pll3_d1, "pll3_d1", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(0), BIT(0), 0x0,
	1, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll3_d2, "pll3_d2", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(1), BIT(1), 0x0,
	2, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll3_d3, "pll3_d3", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(2), BIT(2), 0x0,
	3, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll3_d4, "pll3_d4", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(3), BIT(3), 0x0,
	4, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll3_d5, "pll3_d5", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(4), BIT(4), 0x0,
	5, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll3_d6, "pll3_d6", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(5), BIT(5), 0x0,
	6, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll3_d7, "pll3_d7", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(6), BIT(6), 0x0,
	7, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll3_d8, "pll3_d8", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(7), BIT(7), 0x0,
	8, 1, 0);

//pll1_d8
static SPACEMIT_CCU_GATE(pll1_d8_307p2, "pll1_d8_307p2", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(13), BIT(13), 0x0,
	0);
static SPACEMIT_CCU_FACTOR(pll1_d32_76p8, "pll1_d32_76p8", "pll1_d8_307p2",
	4, 1);
static SPACEMIT_CCU_FACTOR(pll1_d40_61p44, "pll1_d40_61p44", "pll1_d8_307p2",
	5, 1);
static SPACEMIT_CCU_FACTOR(pll1_d16_153p6, "pll1_d16_153p6", "pll1_d8",
	2, 1);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d24_102p4, "pll1_d24_102p4", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(12), BIT(12), 0x0,
	3, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d48_51p2, "pll1_d48_51p2", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(7), BIT(7), 0x0,
	6, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d48_51p2_ap, "pll1_d48_51p2_ap", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(11), BIT(11), 0x0,
	6, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_m3d128_57p6, "pll1_m3d128_57p6", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(8), BIT(8), 0x0,
	16, 3, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d96_25p6, "pll1_d96_25p6", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(4), BIT(4), 0x0,
	12, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d192_12p8, "pll1_d192_12p8", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(3), BIT(3), 0x0,
	24, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d192_12p8_wdt, "pll1_d192_12p8_wdt", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(19), BIT(19), 0x0,
	24, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d384_6p4, "pll1_d384_6p4", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(2), BIT(2), 0x0,
	48, 1, 0);
static SPACEMIT_CCU_FACTOR(pll1_d768_3p2, "pll1_d768_3p2", "pll1_d384_6p4",
	2, 1);
static SPACEMIT_CCU_FACTOR(pll1_d1536_1p6, "pll1_d1536_1p6", "pll1_d384_6p4",
	4, 1);
static SPACEMIT_CCU_FACTOR(pll1_d3072_0p8, "pll1_d3072_0p8", "pll1_d384_6p4",
	8, 1);
//pll1_d7
static SPACEMIT_CCU_FACTOR(pll1_d7_351p08, "pll1_d7_351p08", "pll1_d7",
	1, 1);
//pll1_d6
static SPACEMIT_CCU_GATE(pll1_d6_409p6, "pll1_d6_409p6", "pll1_d6",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(0), BIT(0), 0x0,
	0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d12_204p8, "pll1_d12_204p8", "pll1_d6",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(5), BIT(5), 0x0,
	2, 1, 0);
//pll1_d5
static SPACEMIT_CCU_GATE(pll1_d5_491p52, "pll1_d5_491p52", "pll1_d5",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(21), BIT(21), 0x0,
	0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d10_245p76, "pll1_d10_245p76", "pll1_d5",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(18), BIT(18), 0x0,
	2, 1, 0);
//pll1_d4
static SPACEMIT_CCU_GATE(pll1_d4_614p4, "pll1_d4_614p4", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(15), BIT(15), 0x0,
	0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d52_47p26, "pll1_d52_47p26", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(10), BIT(10), 0x0,
	13, 1, 0);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d78_31p5, "pll1_d78_31p5", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(6), BIT(6), 0x0,
	39, 2, 0);
//pll1_d3
static SPACEMIT_CCU_GATE(pll1_d3_819p2, "pll1_d3_819p2", "pll1_d3",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(14), BIT(14), 0x0,
	0);
//pll1_d2
static SPACEMIT_CCU_GATE(pll1_d2_1228p8, "pll1_d2_1228p8", "pll1_d2",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(16), BIT(16), 0x0,
	0);

//mpmu
static struct ccu_ddn_info uart_ddn_mask_info = {
	.factor = 2,
	.num_mask = 0x1fff,
	.den_mask = 0x1fff,
	.num_shift = 16,
	.den_shift = 0,
};
static struct ccu_ddn_tbl slow_uart1_tbl[] = {
	{.num = 125, .den = 24}, /*rate = parent_rate*24/125/2) */
};
static struct ccu_ddn_tbl slow_uart2_tbl[] = {
	{.num = 6144, .den = 960},/*rate = parent_rate*960/6144/2) */
};
static SPACEMIT_CCU_DDN_GATE(slow_uart1_14p74, "slow_uart1_14p74", "pll1_d16_153p6",
	&uart_ddn_mask_info, &slow_uart1_tbl, ARRAY_SIZE(slow_uart1_tbl),
	BASE_TYPE_MPMU, MPMU_SUCCR, MPMU_ACGR, BIT(1),
	0);
static SPACEMIT_CCU_DDN_GATE(slow_uart2_48, "slow_uart2_48", "pll1_d4_614p4",
	&uart_ddn_mask_info, &slow_uart2_tbl, ARRAY_SIZE(slow_uart2_tbl),
	BASE_TYPE_MPMU, MPMU_SUCCR_1, MPMU_ACGR, BIT(1),
	0);

//apbc
static const char * const uart_parent_names[] = {
	"pll1_m3d128_57p6", "slow_uart1_14p74", "slow_uart2_48"
};
static SPACEMIT_CCU_MUX_GATE(uart1_clk, "uart1_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(uart2_clk, "uart2_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(uart3_clk, "uart3_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART3_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(uart4_clk, "uart4_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART4_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(uart5_clk, "uart5_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART5_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(uart6_clk, "uart6_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART6_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(uart7_clk, "uart7_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART7_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(uart8_clk, "uart8_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART8_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(uart9_clk, "uart9_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART9_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_GATE(gpio_clk, "gpio_clk", "vctcxo_24",
	BASE_TYPE_APBC, APBC_GPIO_CLK_RST,
	0x3, 0x3, 0x0,
	0);
static const char *pwm_parent_names[] = {
	"pll1_d192_12p8", "clk_32k"
};
static SPACEMIT_CCU_MUX_GATE(pwm0_clk, "pwm0_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM0_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm1_clk, "pwm1_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm2_clk, "pwm2_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm3_clk, "pwm3_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM3_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm4_clk, "pwm4_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM4_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm5_clk, "pwm5_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM5_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm6_clk, "pwm6_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM6_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm7_clk, "pwm7_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM7_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm8_clk, "pwm8_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM8_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm9_clk, "pwm9_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM9_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm10_clk, "pwm10_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM10_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm11_clk, "pwm11_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM11_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm12_clk, "pwm12_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM12_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm13_clk, "pwm13_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM13_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm14_clk, "pwm14_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM14_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm15_clk, "pwm15_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM15_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm16_clk, "pwm16_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM16_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm17_clk, "pwm17_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM17_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm18_clk, "pwm18_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM18_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(pwm19_clk, "pwm19_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM19_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static const char *ssp_parent_names[] = { "pll1_d384_6p4", "pll1_d192_12p8", "pll1_d96_25p6",
	"pll1_d48_51p2", "pll1_d768_3p2", "pll1_d1536_1p6", "pll1_d3072_0p8"
};
static SPACEMIT_CCU_MUX_GATE(ssp3_clk, "ssp3_clk", ssp_parent_names,
	BASE_TYPE_APBC, APBC_SSP3_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_GATE(rtc_clk, "rtc_clk", "clk_32k",
	BASE_TYPE_APBC, APBC_RTC_CLK_RST,
	0x83, 0x83, 0x0, 0);
static const char *twsi_parent_names[] = {
	"pll1_d78_31p5", "pll1_d48_51p2", "pll1_d40_61p44"
};
static SPACEMIT_CCU_MUX_GATE(twsi0_clk, "twsi0_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI0_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(twsi1_clk, "twsi1_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(twsi2_clk, "twsi2_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(twsi4_clk, "twsi4_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI4_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(twsi5_clk, "twsi5_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI5_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(twsi6_clk, "twsi6_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI6_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(twsi7_clk, "twsi7_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI7_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(twsi8_clk, "twsi8_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI8_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static const char *timer_parent_names[] = {
	"pll1_d192_12p8", "clk_32k", "pll1_d384_6p4", "vctcxo_3", "vctcxo_1"
};
static SPACEMIT_CCU_MUX_GATE(timers1_clk, "timers1_clk", timer_parent_names,
	BASE_TYPE_APBC, APBC_TIMERS1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(timers2_clk, "timers2_clk", timer_parent_names,
	BASE_TYPE_APBC, APBC_TIMERS2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_GATE(aib_clk, "aib_clk", "vctcxo_24",
	BASE_TYPE_APBC, APBC_AIB_CLK_RST,
	0x3, 0x3, 0x0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(onewire_clk, "onewire_clk", NULL,
	BASE_TYPE_APBC, APBC_ONEWIRE_CLK_RST,
	0x3, 0x3, 0x0, 0);

static const char *sspa_parent_names[] = { "pll1_d384_6p4", "pll1_d192_12p8", "pll1_d96_25p6",
	"pll1_d48_51p2", "pll1_d768_3p2", "pll1_d1536_1p6", "pll1_d3072_0p8"
};
static SPACEMIT_CCU_MUX_GATE(sspa0_clk, "sspa0_clk", sspa_parent_names,
	BASE_TYPE_APBC, APBC_SSPA0_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(sspa1_clk, "sspa1_clk", sspa_parent_names,
	BASE_TYPE_APBC, APBC_SSPA1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(dro_clk, "dro_clk", NULL,
	BASE_TYPE_APBC, APBC_DRO_CLK_RST,
	0x1, 0x1, 0x0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ir_clk, "ir_clk", NULL,
	BASE_TYPE_APBC, APBC_IR_CLK_RST,
	0x1, 0x1, 0x0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(tsen_clk, "tsen_clk", NULL,
	BASE_TYPE_APBC, APBC_TSEN_CLK_RST,
	0x3, 0x3, 0x0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ipc_ap2aud_clk, "ipc_ap2aud_clk", NULL,
	BASE_TYPE_APBC, APBC_IPC_AP2AUD_CLK_RST,
	0x3, 0x3, 0x0, 0);
static const char *can_parent_names[] = {
	"pll1_m3d128_57p6", "slow_uart1_14p74", "slow_uart2_48"
};
static SPACEMIT_CCU_MUX_GATE(can0_clk, "can0_clk", can_parent_names,
	BASE_TYPE_APBC, APBC_CAN0_CLK_RST,
	4, 3, BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(can0_bus_clk, "can0_bus_clk", NULL,
	BASE_TYPE_APBC, APBC_CAN0_CLK_RST,
	BIT(0), BIT(0), 0x0, 0);

//mpmu
static SPACEMIT_CCU_GATE(wdt_clk, "wdt_clk", "pll1_d96_25p6",
	BASE_TYPE_MPMU, MPMU_WDTPCR,
	0x3, 0x3, 0x0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ripc_clk, "ripc_clk", NULL,
	BASE_TYPE_MPMU, MPMU_RIPCCR,
	0x3, 0x3, 0x0, 0);

//apmu
static const char * const jpg_parent_names[] = {
	 "pll1_d4_614p4", "pll1_d6_409p6", "pll1_d5_491p52", "pll1_d3_819p2",
	 "pll1_d2_1228p8", "pll2_d4", "pll2_d3"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(jpg_clk, "jpg_clk", jpg_parent_names,
	BASE_TYPE_APMU, APMU_JPG_CLK_RES_CTRL,
	5, 3, BIT(15),
	2, 3, BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(jpg_4kafbc_clk, "jpg_4kafbc_clk", NULL,
	BASE_TYPE_APMU, APMU_JPG_CLK_RES_CTRL,
	BIT(16), BIT(16), 0x0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(jpg_2kafbc_clk, "jpg_2kafbc_clk", NULL,
	BASE_TYPE_APMU, APMU_JPG_CLK_RES_CTRL,
	BIT(17), BIT(17), 0x0, 0);
static const char * const ccic2phy_parent_names[] = {
	"pll1_d24_102p4", "pll1_d48_51p2_ap"
};
static SPACEMIT_CCU_MUX_GATE(ccic2phy_clk, "ccic2phy_clk", ccic2phy_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	7, 1, BIT(5), BIT(5), 0x0,
	0);
static const char * const ccic3phy_parent_names[] = {
	"pll1_d24_102p4", "pll1_d48_51p2_ap"
};
static SPACEMIT_CCU_MUX_GATE(ccic3phy_clk, "ccic3phy_clk", ccic3phy_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	31, 1, BIT(30), BIT(30), 0x0, 0);
static const char * const csi_parent_names[] = {
	 "pll1_d5_491p52", "pll1_d6_409p6", "pll1_d4_614p4", "pll1_d3_819p2",
	 "pll2_d2", "pll2_d3", "pll2_d4", "pll1_d2_1228p8"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(csi_clk, "csi_clk", csi_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	20, 3, BIT(15),
	16, 3, BIT(4), BIT(4), 0x0,
	0);
static const char * const camm_parent_names[] = {
	"pll1_d8_307p2", "pll2_d5", "pll1_d6_409p6", "vctcxo_24"
};
static SPACEMIT_CCU_DIV_MUX_GATE(camm0_clk, "camm0_clk", camm_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	23, 4, 8, 2,
	BIT(28), BIT(28), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(camm1_clk, "camm1_clk", camm_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	23, 4, 8, 2,
	BIT(6), BIT(6), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(camm2_clk, "camm2_clk", camm_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	23, 4, 8, 2,
	BIT(27), BIT(27), 0x0,
	0);
static const char * const isp_cpp_parent_names[] = {
	 "pll1_d8_307p2", "pll1_d6_409p6"
};
static SPACEMIT_CCU_DIV_MUX_GATE(isp_cpp_clk, "isp_cpp_clk", isp_cpp_parent_names,
	BASE_TYPE_APMU, APMU_ISP_CLK_RES_CTRL,
	24, 2, 26, 1,
	BIT(28), BIT(28), 0x0,
	0);
static const char * const isp_bus_parent_names[] = {
	 "pll1_d6_409p6", "pll1_d5_491p52", "pll1_d8_307p2", "pll1_d10_245p76"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(isp_bus_clk, "isp_bus_clk", isp_bus_parent_names,
	BASE_TYPE_APMU, APMU_ISP_CLK_RES_CTRL,
	18, 3, BIT(23),
	21, 2, BIT(17), BIT(17), 0x0,
	0);
static const char * const isp_parent_names[] = {
	 "pll1_d6_409p6", "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d8_307p2"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(isp_clk, "isp_clk", isp_parent_names,
	BASE_TYPE_APMU, APMU_ISP_CLK_RES_CTRL,
	4, 3, BIT(7),
	8, 2, BIT(1), BIT(1), 0x0,
	0);
static const char * const dpumclk_parent_names[] = {
	"pll1_d6_409p6", "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d8_307p2"
};
static SPACEMIT_CCU_DIV2_FC_MUX_GATE(dpu_mclk, "dpu_mclk", dpumclk_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1, APMU_LCD_CLK_RES_CTRL2,
	1, 4, BIT(29),
	5, 3, BIT(0), BIT(0), 0x0,
	0);
static const char * const dpuesc_parent_names[] = {
	"pll1_d48_51p2_ap", "pll1_d52_47p26", "pll1_d96_25p6", "pll1_d32_76p8"
};
static SPACEMIT_CCU_MUX_GATE(dpu_esc_clk, "dpu_esc_clk", dpuesc_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1,
	0, 2, BIT(2), BIT(2), 0x0,
	0);
static const char * const dpubit_parent_names[] = { "pll1_d3_819p2", "pll2_d2", "pll2_d3",
	"pll1_d2_1228p8", "pll2_d4", "pll2_d5", "pll2_d8", "pll2_d8" //6 should be 429M?
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(dpu_bit_clk, "dpu_bit_clk", dpubit_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1,
	17, 3, BIT(31),
	20, 3, BIT(16), BIT(16), 0x0,
	0);
static const char * const dpupx_parent_names[] = {
	"pll1_d6_409p6", "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d8_307p2", "pll2_d7", "pll2_d8"
};
static SPACEMIT_CCU_DIV2_FC_MUX_GATE(dpu_pxclk, "dpu_pxclk", dpupx_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1, APMU_LCD_CLK_RES_CTRL2,
	17, 4, BIT(30),
	21, 3, BIT(16), BIT(16), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(dpu_hclk, "dpu_hclk", NULL,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1,
	BIT(5), BIT(5), 0x0,
	0);
static const char * const dpu_spi_parent_names[] = {
	 "pll1_d8_307p2", "pll1_d6_409p6", "pll1_d10_245p76", "pll1_d11_223p4",
	 "pll1_d13_189", "pll1_d23_106p8", "pll2_d3", "pll2_d5"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(dpu_spi_clk, "dpu_spi_clk", dpu_spi_parent_names,
	BASE_TYPE_APMU, APMU_LCD_SPI_CLK_RES_CTRL,
	8, 3, BIT(7),
	12, 3, BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(dpu_spi_hbus_clk, "dpu_spi_hbus_clk", NULL,
	BASE_TYPE_APMU, APMU_LCD_SPI_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(dpu_spi_bus_clk, "dpu_spi_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_LCD_SPI_CLK_RES_CTRL,
	BIT(5), BIT(5), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(dpu_spi_aclk, "dpu_spi_aclk", NULL,
	BASE_TYPE_APMU, APMU_LCD_SPI_CLK_RES_CTRL,
	BIT(6), BIT(6), 0x0,
	0);
static const char * const v2d_parent_names[] = {
	"pll1_d5_491p52", "pll1_d6_409p6", "pll1_d8_307p2", "pll1_d4_614p4",
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(v2d_clk, "v2d_clk", v2d_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1,
	9, 3, BIT(28),
	12, 2, BIT(8), BIT(8), 0x0,
	0);
static const char * const ccic_4x_parent_names[] = {
	 "pll1_d5_491p52", "pll1_d6_409p6", "pll1_d4_614p4", "pll1_d3_819p2",
	 "pll2_d2", "pll2_d3", "pll2_d4", "pll1_d2_1228p8"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(ccic_4x_clk, "ccic_4x_clk", ccic_4x_parent_names,
	BASE_TYPE_APMU, APMU_CCIC_CLK_RES_CTRL,
	18, 3, BIT(15),
	23, 2, BIT(4), BIT(4), 0x0,
	0);
static const char * const ccic1phy_parent_names[] = {
	"pll1_d24_102p4", "pll1_d48_51p2_ap"
};
static SPACEMIT_CCU_MUX_GATE(ccic1phy_clk, "ccic1phy_clk", ccic1phy_parent_names,
	BASE_TYPE_APMU, APMU_CCIC_CLK_RES_CTRL,
	7, 1, BIT(5), BIT(5), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(sdh_axi_aclk, "sdh_axi_aclk", NULL,
	BASE_TYPE_APMU, APMU_SDH0_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const sdh01_parent_names[] = {"pll1_d6_409p6",
	"pll1_d4_614p4", "pll2_d8", "pll2_d5", "pll1_d11_223p4", "pll1_d13_189", "pll1_d23_106p8" };

static SPACEMIT_CCU_DIV_FC_MUX_GATE(sdh0_clk, "sdh0_clk", sdh01_parent_names,
	BASE_TYPE_APMU, APMU_SDH0_CLK_RES_CTRL,
	8, 3, BIT(11),
	5, 3, BIT(4), BIT(4), 0x0,
	0);
static SPACEMIT_CCU_DIV_FC_MUX_GATE(sdh1_clk, "sdh1_clk", sdh01_parent_names,
	BASE_TYPE_APMU, APMU_SDH1_CLK_RES_CTRL,
	8, 3, BIT(11),
	5, 3, BIT(4), BIT(4), 0x0,
	0);
static const char * const sdh2_parent_names[] = {"pll1_d6_409p6",
	"pll1_d4_614p4", "pll2_d8", "pll1_d3_819p2", "pll1_d11_223p4", "pll1_d13_189", "pll1_d23_106p8" };

static SPACEMIT_CCU_DIV_FC_MUX_GATE(sdh2_clk, "sdh2_clk", sdh2_parent_names,
	BASE_TYPE_APMU, APMU_SDH2_CLK_RES_CTRL,
	8, 3, BIT(11),
	5, 3, BIT(4), BIT(4), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(usb_axi_clk, "usb_axi_clk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(usb_p1_aclk, "usb_p1_aclk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(5), BIT(5), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(usb30_clk, "usb30_clk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(8), BIT(8), 0x0,
	0);
static const char * const qspi_parent_names[] = {"pll1_d6_409p6", "pll2_d8", "pll1_d8_307p2",
		"pll1_d10_245p76", "pll1_d11_223p4", "pll1_d23_106p8", "pll1_d5_491p52", "pll1_d13_189"};
static SPACEMIT_CCU_DIV_MFC_MUX_GATE(qspi_clk, "qspi_clk", qspi_parent_names,
	BASE_TYPE_APMU, APMU_QSPI_CLK_RES_CTRL,
	9, 3, BIT(12),
	6, 3, BIT(4), BIT(4), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(qspi_bus_clk, "qspi_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_QSPI_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(dma_clk, "dma_clk", NULL,
	BASE_TYPE_APMU, APMU_DMA_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const aes_parent_names[] = {
	"pll1_d12_204p8", "pll1_d24_102p4"
};
static SPACEMIT_CCU_MUX_GATE(aes_clk, "aes_clk", aes_parent_names,
	BASE_TYPE_APMU, APMU_AES_CLK_RES_CTRL,
	6, 1, BIT(5), BIT(5), 0x0,
	0);
static const char * const vpu_parent_names[] = {
	"pll1_d4_614p4", "pll1_d5_491p52", "pll1_d3_819p2", "pll1_d6_409p6",
	"pll3_d6", "pll2_d3", "pll2_d4", "pll2_d5"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(vpu_clk, "vpu_clk", vpu_parent_names,
	BASE_TYPE_APMU, APMU_VPU_CLK_RES_CTRL,
	13, 3, BIT(21),
	10, 3,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const gpu_parent_names[] = {
	"pll1_d4_614p4", "pll1_d5_491p52", "pll1_d3_819p2", "pll1_d6_409p6",
	"pll3_d6", "pll2_d3", "pll2_d4", "pll2_d5"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(gpu_clk, "gpu_clk", gpu_parent_names,
	BASE_TYPE_APMU, APMU_GPU_CLK_RES_CTRL,
	12, 3, BIT(15),
	18, 3,
	BIT(4), BIT(4), 0x0,
	0);
static const char * const emmc_parent_names[] = {
	"pll1_d6_409p6", "pll1_d4_614p4", "pll1_d52_47p26", "pll1_d3_819p2"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(emmc_clk, "emmc_clk", emmc_parent_names,
	BASE_TYPE_APMU, APMU_PMUA_EM_CLK_RES_CTRL,
	8, 3, BIT(11),
	6, 2,
	0x18, 0x18, 0x0,
	0);
static SPACEMIT_CCU_DIV_GATE(emmc_x_clk, "emmc_x_clk", "pll1_d2_1228p8",
	BASE_TYPE_APMU, APMU_PMUA_EM_CLK_RES_CTRL,
	12, 3, BIT(15), BIT(15), 0x0,
	0);
static const char * const audio_parent_names[] = {
	 "pll1_aud_245p7", "pll1_d8_307p2", "pll1_d6_409p6"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(audio_clk, "audio_clk", audio_parent_names,
	BASE_TYPE_APMU, APMU_AUDIO_CLK_RES_CTRL,
	4, 3, BIT(15),
	7, 3,
	BIT(12), BIT(12), 0x0,
	0);
static const char * const hdmi_parent_names[] = {
	 "pll1_d6_409p6", "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d8_307p2"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(hdmi_mclk, "hdmi_mclk", hdmi_parent_names,
	BASE_TYPE_APMU, APMU_HDMI_CLK_RES_CTRL,
	1, 4, BIT(29),
	5, 3,
	BIT(0), BIT(0), 0x0,
	0);
static const char * const cci550_parent_names[] = {
	 "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d3_819p2", "pll2_d3"
};
static SPACEMIT_CCU_DIV_FC_MUX(cci550_clk, "cci550_clk", cci550_parent_names,
	BASE_TYPE_APMU, APMU_CCI550_CLK_CTRL,
	8, 3, BIT(12),
	0, 2,
	0);
static const char * const pmua_aclk_parent_names[] = {
	 "pll1_d10_245p76", "pll1_d8_307p2"
};
static SPACEMIT_CCU_DIV_FC_MUX(pmua_aclk, "pmua_aclk", pmua_aclk_parent_names,
	BASE_TYPE_APMU, APMU_ACLK_CLK_CTRL,
	1, 2, BIT(4),
	0, 1,
	0);
static const char * const cpu_c0_hi_parent_names[] = {
	 "pll3_d2", "pll3_d1"
};
static SPACEMIT_CCU_MUX(cpu_c0_hi_clk, "cpu_c0_hi_clk", cpu_c0_hi_parent_names,
	BASE_TYPE_APMU, APMU_CPU_C0_CLK_CTRL,
	13, 1,
	0);
static const char * const cpu_c0_parent_names[] = { "pll1_d4_614p4", "pll1_d3_819p2", "pll1_d6_409p6",
	"pll1_d5_491p52", "pll1_d2_1228p8", "pll3_d3", "pll2_d3", "cpu_c0_hi_clk"
};
static SPACEMIT_CCU_MUX_FC(cpu_c0_core_clk, "cpu_c0_core_clk", cpu_c0_parent_names,
	BASE_TYPE_APMU, APMU_CPU_C0_CLK_CTRL,
	BIT(12),
	0, 3,
	0);
static SPACEMIT_CCU_DIV(cpu_c0_ace_clk, "cpu_c0_ace_clk", "cpu_c0_core_clk",
	BASE_TYPE_APMU, APMU_CPU_C0_CLK_CTRL,
	6, 3,
	0);
static SPACEMIT_CCU_DIV(cpu_c0_tcm_clk, "cpu_c0_tcm_clk", "cpu_c0_core_clk",
	BASE_TYPE_APMU, APMU_CPU_C0_CLK_CTRL,
	9, 3,
	0);
static const char * const cpu_c1_hi_parent_names[] = {
	 "pll3_d2", "pll3_d1"
};
static SPACEMIT_CCU_MUX(cpu_c1_hi_clk, "cpu_c1_hi_clk", cpu_c1_hi_parent_names,
	BASE_TYPE_APMU, APMU_CPU_C1_CLK_CTRL,
	13, 1,
	0);
static const char * const cpu_c1_parent_names[] = { "pll1_d4_614p4", "pll1_d3_819p2", "pll1_d6_409p6",
	"pll1_d5_491p52", "pll1_d2_1228p8", "pll3_d3", "pll2_d3", "cpu_c1_hi_clk"
};
static SPACEMIT_CCU_MUX_FC(cpu_c1_pclk, "cpu_c1_pclk", cpu_c1_parent_names,
	BASE_TYPE_APMU, APMU_CPU_C1_CLK_CTRL,
	BIT(12),
	0, 3,
	0);
static SPACEMIT_CCU_DIV(cpu_c1_ace_clk, "cpu_c1_ace_clk", "cpu_c1_pclk",
	BASE_TYPE_APMU, APMU_CPU_C1_CLK_CTRL,
	6, 3,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(pcie0_clk, "pcie0_clk", NULL,
	BASE_TYPE_APMU, APMU_PCIE_CLK_RES_CTRL_0,
	0x7, 0x7, 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(pcie1_clk, "pcie1_clk", NULL,
	BASE_TYPE_APMU, APMU_PCIE_CLK_RES_CTRL_1,
	0x7, 0x7, 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(pcie2_clk, "pcie2_clk", NULL,
	BASE_TYPE_APMU, APMU_PCIE_CLK_RES_CTRL_2,
	0x7, 0x7, 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(emac0_bus_clk, "emac0_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_EMAC0_CLK_RES_CTRL,
	BIT(0), BIT(0), 0x0,
	0);
static SPACEMIT_CCU_GATE(emac0_ptp_clk, "emac0_ptp_clk", "pll1_d3_819p2",
	BASE_TYPE_APMU, APMU_EMAC0_CLK_RES_CTRL,
	BIT(15), BIT(15), 0x0,
	0);
static SPACEMIT_CCU_GATE_NO_PARENT(emac1_bus_clk, "emac1_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_EMAC1_CLK_RES_CTRL,
	BIT(0), BIT(0), 0x0,
	0);
static SPACEMIT_CCU_GATE(emac1_ptp_clk, "emac1_ptp_clk", "pll1_d3_819p2",
	BASE_TYPE_APMU, APMU_EMAC1_CLK_RES_CTRL,
	BIT(15), BIT(15), 0x0,
	0);

//apbc2
static const char * const uart1_sec_parent_names[] = {
	"pll1_m3d128_57p6", "slow_uart1_14p74", "slow_uart2_48"
};
static SPACEMIT_CCU_MUX_GATE(uart1_sec_clk, "uart1_sec_clk", uart1_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_UART1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);

static const char *ssp2_sec_parent_names[] = { "pll1_d384_6p4", "pll1_d192_12p8", "pll1_d96_25p6",
	"pll1_d48_51p2", "pll1_d768_3p2", "pll1_d1536_1p6", "pll1_d3072_0p8"
};
static SPACEMIT_CCU_MUX_GATE(ssp2_sec_clk, "ssp2_sec_clk", ssp2_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_SSP2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static const char *twsi3_sec_parent_names[] = {
	"pll1_d78_31p5", "pll1_d48_51p2", "pll1_d40_61p44"
};
static SPACEMIT_CCU_MUX_GATE(twsi3_sec_clk, "twsi3_sec_clk", twsi3_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_TWSI3_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_GATE(rtc_sec_clk, "rtc_sec_clk", "clk_32k",
	BASE_TYPE_APBC2, APBC2_RTC_CLK_RST,
	0x83, 0x83, 0x0, 0);
static const char *timer_sec_parent_names[] = {
	"pll1_d192_12p8", "clk_32k", "pll1_d384_6p4", "vctcxo_3", "vctcxo_1"
};
static SPACEMIT_CCU_MUX_GATE(timers0_sec_clk, "timers0_sec_clk", timer_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_TIMERS0_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static const char *kpc_sec_parent_names[] = {
	"pll1_d192_12p8", "clk_32k", "pll1_d384_6p4", "vctcxo_3", "vctcxo_1"
};
static SPACEMIT_CCU_MUX_GATE(kpc_sec_clk, "kpc_sec_clk", kpc_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_KPC_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static SPACEMIT_CCU_GATE(gpio_sec_clk, "gpio_sec_clk", "vctcxo_24",
	BASE_TYPE_APBC2, APBC2_GPIO_CLK_RST,
	0x3, 0x3, 0x0,
	0);

static const char * const apb_parent_names[] = {
	"pll1_d96_25p6", "pll1_d48_51p2", "pll1_d96_25p6", "pll1_d24_102p4"
};
static SPACEMIT_CCU_MUX(apb_clk, "apb_clk", apb_parent_names,
	BASE_TYPE_MPMU, MPMU_APBCSCR,
	0, 2, 0);

static struct spacemit_clk_table spacemit_k1x_clks = {
	.clks	= {
		[CLK_PLL1_2457P6]	= &pll1_2457p6_vco.common.clk,
		[CLK_PLL2]		= &pll2.common.clk,
		[CLK_PLL3]		= &pll3.common.clk,
		[CLK_PLL1_D2]		= &pll1_d2.common.clk,
		[CLK_PLL1_D3]		= &pll1_d3.common.clk,
		[CLK_PLL1_D4]		= &pll1_d4.common.clk,
		[CLK_PLL1_D5]		= &pll1_d5.common.clk,
		[CLK_PLL1_D6]		= &pll1_d6.common.clk,
		[CLK_PLL1_D7]		= &pll1_d7.common.clk,
		[CLK_PLL1_D8]		= &pll1_d8.common.clk,
		[CLK_PLL1_D11]		= &pll1_d11_223p4.common.clk,
		[CLK_PLL1_D13]		= &pll1_d13_189.common.clk,
		[CLK_PLL1_D23]		= &pll1_d23_106p8.common.clk,
		[CLK_PLL1_D64]		= &pll1_d64_38p4.common.clk,
		[CLK_PLL1_D10_AUD]	= &pll1_aud_245p7.common.clk,
		[CLK_PLL1_D100_AUD]	= &pll1_aud_24p5.common.clk,
		[CLK_PLL2_D1]		= &pll2_d1.common.clk,
		[CLK_PLL2_D2]		= &pll2_d2.common.clk,
		[CLK_PLL2_D3]		= &pll2_d3.common.clk,
		[CLK_PLL2_D4]		= &pll2_d4.common.clk,
		[CLK_PLL2_D5]		= &pll2_d5.common.clk,
		[CLK_PLL2_D6]		= &pll2_d6.common.clk,
		[CLK_PLL2_D7]		= &pll2_d7.common.clk,
		[CLK_PLL2_D8]		= &pll2_d8.common.clk,
		[CLK_PLL3_D1]		= &pll3_d1.common.clk,
		[CLK_PLL3_D2]		= &pll3_d2.common.clk,
		[CLK_PLL3_D3]		= &pll3_d3.common.clk,
		[CLK_PLL3_D4]		= &pll3_d4.common.clk,
		[CLK_PLL3_D5]		= &pll3_d5.common.clk,
		[CLK_PLL3_D6]		= &pll3_d6.common.clk,
		[CLK_PLL3_D7]		= &pll3_d7.common.clk,
		[CLK_PLL3_D8]		= &pll3_d8.common.clk,
		[CLK_PLL1_307P2]	= &pll1_d8_307p2.common.clk,
		[CLK_PLL1_76P8]		= &pll1_d32_76p8.common.clk,
		[CLK_PLL1_61P44]	= &pll1_d40_61p44.common.clk,
		[CLK_PLL1_153P6]	= &pll1_d16_153p6.common.clk,
		[CLK_PLL1_102P4]	= &pll1_d24_102p4.common.clk,
		[CLK_PLL1_51P2]		= &pll1_d48_51p2.common.clk,
		[CLK_PLL1_51P2_AP]	= &pll1_d48_51p2_ap.common.clk,
		[CLK_PLL1_57P6]		= &pll1_m3d128_57p6.common.clk,
		[CLK_PLL1_25P6]		= &pll1_d96_25p6.common.clk,
		[CLK_PLL1_12P8]		= &pll1_d192_12p8.common.clk,
		[CLK_PLL1_12P8_WDT]	= &pll1_d192_12p8_wdt.common.clk,
		[CLK_PLL1_6P4]		= &pll1_d384_6p4.common.clk,
		[CLK_PLL1_3P2]		= &pll1_d768_3p2.common.clk,
		[CLK_PLL1_1P6]		= &pll1_d1536_1p6.common.clk,
		[CLK_PLL1_0P8]		= &pll1_d3072_0p8.common.clk,
		[CLK_PLL1_351]		= &pll1_d7_351p08.common.clk,
		[CLK_PLL1_409P6]	= &pll1_d6_409p6.common.clk,
		[CLK_PLL1_204P8]	= &pll1_d12_204p8.common.clk,
		[CLK_PLL1_491]		= &pll1_d5_491p52.common.clk,
		[CLK_PLL1_245P76]	= &pll1_d10_245p76.common.clk,
		[CLK_PLL1_614]		= &pll1_d4_614p4.common.clk,
		[CLK_PLL1_47P26]	= &pll1_d52_47p26.common.clk,
		[CLK_PLL1_31P5]		= &pll1_d78_31p5.common.clk,
		[CLK_PLL1_819]		= &pll1_d3_819p2.common.clk,
		[CLK_PLL1_1228]		= &pll1_d2_1228p8.common.clk,
		[CLK_SLOW_UART1]	= &slow_uart1_14p74.common.clk,
		[CLK_SLOW_UART2]	= &slow_uart2_48.common.clk,
		[CLK_UART1]		= &uart1_clk.common.clk,
		[CLK_UART2]		= &uart2_clk.common.clk,
		[CLK_UART3]		= &uart3_clk.common.clk,
		[CLK_UART4]		= &uart4_clk.common.clk,
		[CLK_UART5]		= &uart5_clk.common.clk,
		[CLK_UART6]		= &uart6_clk.common.clk,
		[CLK_UART7]		= &uart7_clk.common.clk,
		[CLK_UART8]		= &uart8_clk.common.clk,
		[CLK_UART9]		= &uart9_clk.common.clk,
		[CLK_GPIO]		= &gpio_clk.common.clk,
		[CLK_PWM0]		= &pwm0_clk.common.clk,
		[CLK_PWM1]		= &pwm1_clk.common.clk,
		[CLK_PWM2]		= &pwm2_clk.common.clk,
		[CLK_PWM3]		= &pwm3_clk.common.clk,
		[CLK_PWM4]		= &pwm4_clk.common.clk,
		[CLK_PWM5]		= &pwm5_clk.common.clk,
		[CLK_PWM6]		= &pwm6_clk.common.clk,
		[CLK_PWM7]		= &pwm7_clk.common.clk,
		[CLK_PWM8]		= &pwm8_clk.common.clk,
		[CLK_PWM9]		= &pwm9_clk.common.clk,
		[CLK_PWM10]		= &pwm10_clk.common.clk,
		[CLK_PWM11]		= &pwm11_clk.common.clk,
		[CLK_PWM12]		= &pwm12_clk.common.clk,
		[CLK_PWM13]		= &pwm13_clk.common.clk,
		[CLK_PWM14]		= &pwm14_clk.common.clk,
		[CLK_PWM15]		= &pwm15_clk.common.clk,
		[CLK_PWM16]		= &pwm16_clk.common.clk,
		[CLK_PWM17]		= &pwm17_clk.common.clk,
		[CLK_PWM18]		= &pwm18_clk.common.clk,
		[CLK_PWM19]		= &pwm19_clk.common.clk,
		[CLK_SSP3]		= &ssp3_clk.common.clk,
		[CLK_RTC]		= &rtc_clk.common.clk,
		[CLK_TWSI0]		= &twsi0_clk.common.clk,
		[CLK_TWSI1]		= &twsi1_clk.common.clk,
		[CLK_TWSI2]		= &twsi2_clk.common.clk,
		[CLK_TWSI4]		= &twsi4_clk.common.clk,
		[CLK_TWSI5]		= &twsi5_clk.common.clk,
		[CLK_TWSI6]		= &twsi6_clk.common.clk,
		[CLK_TWSI7]		= &twsi7_clk.common.clk,
		[CLK_TWSI8]		= &twsi8_clk.common.clk,
		[CLK_TIMERS1]		= &timers1_clk.common.clk,
		[CLK_TIMERS2]		= &timers2_clk.common.clk,
		[CLK_AIB]		= &aib_clk.common.clk,
		[CLK_ONEWIRE]		= &onewire_clk.common.clk,
		[CLK_SSPA0]		= &sspa0_clk.common.clk,
		[CLK_SSPA1]		= &sspa1_clk.common.clk,
		[CLK_DRO]		= &dro_clk.common.clk,
		[CLK_IR]		= &ir_clk.common.clk,
		[CLK_TSEN]		= &tsen_clk.common.clk,
		[CLK_IPC_AP2AUD]	= &ipc_ap2aud_clk.common.clk,
		[CLK_CAN0]		= &can0_clk.common.clk,
		[CLK_CAN0_BUS]		= &can0_bus_clk.common.clk,
		[CLK_WDT] 		= &wdt_clk.common.clk,
		[CLK_RIPC] 		= &ripc_clk.common.clk,
		[CLK_JPG] 		= &jpg_clk.common.clk,
		[CLK_JPF_4KAFBC]	= &jpg_4kafbc_clk.common.clk,
		[CLK_JPF_2KAFBC]	= &jpg_2kafbc_clk.common.clk,
		[CLK_CCIC2PHY]		= &ccic2phy_clk.common.clk,
		[CLK_CCIC3PHY]		= &ccic3phy_clk.common.clk,
		[CLK_CSI]		= &csi_clk.common.clk,
		[CLK_CAMM0]		= &camm0_clk.common.clk,
		[CLK_CAMM1]		= &camm1_clk.common.clk,
		[CLK_CAMM2]		= &camm2_clk.common.clk,
		[CLK_ISP_CPP]		= &isp_cpp_clk.common.clk,
		[CLK_ISP_BUS]		= &isp_bus_clk.common.clk,
		[CLK_ISP]		= &isp_clk.common.clk,
		[CLK_DPU_MCLK]		= &dpu_mclk.common.clk,
		[CLK_DPU_ESC]		= &dpu_esc_clk.common.clk,
		[CLK_DPU_BIT]		= &dpu_bit_clk.common.clk,
		[CLK_DPU_PXCLK]		= &dpu_pxclk.common.clk,
		[CLK_DPU_HCLK]		= &dpu_hclk.common.clk,
		[CLK_DPU_SPI]		= &dpu_spi_clk.common.clk,
		[CLK_DPU_SPI_HBUS]	= &dpu_spi_hbus_clk.common.clk,
		[CLK_DPU_SPIBUS]	= &dpu_spi_bus_clk.common.clk,
		[CLK_SPU_SPI_ACLK]	= &dpu_spi_aclk.common.clk,
		[CLK_V2D]		= &v2d_clk.common.clk,
		[CLK_CCIC_4X]		= &ccic_4x_clk.common.clk,
		[CLK_CCIC1PHY]		= &ccic1phy_clk.common.clk,
		[CLK_SDH_AXI]		= &sdh_axi_aclk.common.clk,
		[CLK_SDH0] 		= &sdh0_clk.common.clk,
		[CLK_SDH1]		= &sdh1_clk.common.clk,
		[CLK_SDH2]		= &sdh2_clk.common.clk,
		[CLK_USB_P1]		= &usb_p1_aclk.common.clk,
		[CLK_USB_AXI]		= &usb_axi_clk.common.clk,
		[CLK_USB30]		= &usb30_clk.common.clk,
		[CLK_QSPI]		= &qspi_clk.common.clk,
		[CLK_QSPI_BUS]		= &qspi_bus_clk.common.clk,
		[CLK_DMA]		= &dma_clk.common.clk,
		[CLK_AES]		= &aes_clk.common.clk,
		[CLK_VPU]		= &vpu_clk.common.clk,
		[CLK_GPU]		= &gpu_clk.common.clk,
		[CLK_EMMC]		= &emmc_clk.common.clk,
		[CLK_EMMC_X]		= &emmc_x_clk.common.clk,
		[CLK_AUDIO]		= &audio_clk.common.clk,
		[CLK_HDMI]		= &hdmi_mclk.common.clk,
		[CLK_CCI550]		= &cci550_clk.common.clk,
		[CLK_PMUA_ACLK]		= &pmua_aclk.common.clk,
		[CLK_CPU_C0_HI]		= &cpu_c0_hi_clk.common.clk,
		[CLK_CPU_C0_CORE]	= &cpu_c0_core_clk.common.clk,
		[CLK_CPU_C0_ACE]	= &cpu_c0_ace_clk.common.clk,
		[CLK_CPU_C0_TCM]	= &cpu_c0_tcm_clk.common.clk,
		[CLK_CPU_C1_HI]		= &cpu_c1_hi_clk.common.clk,
		[CLK_CPU_C1_CORE]	= &cpu_c1_pclk.common.clk,
		[CLK_CPU_C1_ACE]	= &cpu_c1_ace_clk.common.clk,
		[CLK_PCIE0]		= &pcie0_clk.common.clk,
		[CLK_PCIE1]		= &pcie1_clk.common.clk,
		[CLK_PCIE2]		= &pcie2_clk.common.clk,
		[CLK_EMAC0_BUS]		= &emac0_bus_clk.common.clk,
		[CLK_EMAC0_PTP]		= &emac0_ptp_clk.common.clk,
		[CLK_EMAC1_BUS]		= &emac1_bus_clk.common.clk,
		[CLK_EMAC1_PTP]		= &emac1_ptp_clk.common.clk,
		[CLK_SEC_UART1]		= &uart1_sec_clk.common.clk,
		[CLK_SEC_SSP2]		= &ssp2_sec_clk.common.clk,
		[CLK_SEC_TWSI3]		= &twsi3_sec_clk.common.clk,
		[CLK_SEC_RTC]		= &rtc_sec_clk.common.clk,
		[CLK_SEC_TIMERS0]	= &timers0_sec_clk.common.clk,
		[CLK_SEC_KPC]		= &kpc_sec_clk.common.clk,
		[CLK_SEC_GPIO]		= &gpio_sec_clk.common.clk,
		[CLK_APB]		= &apb_clk.common.clk,
	},
	.num = CLK_MAX_NO,
};

#endif

struct spacemit_clk_init_rate init_rate_tbl[] = {
#ifdef CONFIG_SPL_BUILD
	{CLK_PMUA_ACLK_SPL, 307200000},
	{CLK_APB_SPL,	102400000},
#else
	{CLK_PMUA_ACLK, 307200000},
	{CLK_APB,	102400000},
	{CLK_SLOW_UART1, 14745600},
#endif
};

static inline const struct clk_ops *ccu_clk_dev_ops(struct udevice *dev)
{
	return (const struct clk_ops *)dev->driver->ops;
}

#ifndef CONFIG_SPL_BUILD
ulong ccu_clk_get_rate(struct clk *clk)
{
	const struct clk_ops *ops;
	struct clk *c = spacemit_k1x_clks.clks[clk->id];
	if (!clk_valid(c))
		return 0;
	ops = ccu_clk_dev_ops(c->dev);
	if(ops->get_rate)
		return ops->get_rate(c);
	return 0;
}

ulong ccu_clk_round_rate(struct clk *clk, unsigned long rate)
{
	const struct clk_ops *ops;
	struct clk *c = spacemit_k1x_clks.clks[clk->id];
	if (!clk_valid(c))
		return 0;
	ops = ccu_clk_dev_ops(c->dev);
	if(ops->round_rate)
		return ops->round_rate(c, rate);
	return 0;
}

int ccu_clk_set_parent(struct clk *clk, struct clk *parent)
{
	const struct clk_ops *ops;

	struct clk *c = spacemit_k1x_clks.clks[clk->id];
	struct clk *p = spacemit_k1x_clks.clks[parent->id];
	if (!clk_valid(c))
		return 0;
	ops = ccu_clk_dev_ops(c->dev);
	if(ops->set_parent)
		return ops->set_parent(c, p);
	return 0;
}
#endif

int ccu_clk_disable(struct clk *clk)
{
	const struct clk_ops *ops;

#ifdef CONFIG_SPL_BUILD
	clk->id = transfer_clk_id_to_spl(clk->id);
#endif
	struct clk *c = spacemit_k1x_clks.clks[clk->id];
	if (!clk_valid(c))
		return 0;
	ops = ccu_clk_dev_ops(c->dev);
	if(ops->disable)
		return ops->disable(c);
	return 0;
}

ulong ccu_clk_set_rate(struct clk *clk, unsigned long rate)
{
	const struct clk_ops *ops;

#ifdef CONFIG_SPL_BUILD
	clk->id = transfer_clk_id_to_spl(clk->id);
#endif
	struct clk *c = spacemit_k1x_clks.clks[clk->id];

	if (!clk_valid(c))
		return 0;
	ops = ccu_clk_dev_ops(c->dev);
	if(ops->set_rate)
		return ops->set_rate(c, rate);
	return 0;
}

int ccu_clk_enable(struct clk *clk)
{
	const struct clk_ops *ops;

#ifdef CONFIG_SPL_BUILD
	clk->id = transfer_clk_id_to_spl(clk->id);
#endif
	struct clk *c = spacemit_k1x_clks.clks[clk->id];

	if (!clk_valid(c))
		return 0;
	ops = ccu_clk_dev_ops(c->dev);
	if(ops->enable)
		return ops->enable(c);
	return 0;
}

const struct clk_ops ccu_clk_ops = {
#ifndef CONFIG_SPL_BUILD
	.get_rate = ccu_clk_get_rate,
	.round_rate = ccu_clk_round_rate,
	.set_parent = ccu_clk_set_parent,
#endif
	.disable = ccu_clk_disable,
	.set_rate = ccu_clk_set_rate,
	.enable = ccu_clk_enable,
};

int ccu_common_init(struct clk * clk, struct spacemit_k1x_clk *clk_info, struct spacemit_clk_table *clks)
{
	struct ccu_common *common = clk_to_ccu_common(clk);
	struct ccu_pll *pll = clk_to_ccu_pll(clk);
	int ret;

	if (!common)
		return -1;

	switch(common->base_type){
	case BASE_TYPE_MPMU:
		common->base = clk_info->mpmu_base;
		break;
	case BASE_TYPE_APMU:
		common->base = clk_info->apmu_base;
		break;
	case BASE_TYPE_APBC:
		common->base = clk_info->apbc_base;
		break;
	case BASE_TYPE_APBS:
		common->base = clk_info->apbs_base;
		break;
#ifndef CONFIG_SPL_BUILD
	case BASE_TYPE_CIU:
		common->base = clk_info->ciu_base;
		break;
	case BASE_TYPE_DCIU:
		common->base = clk_info->dciu_base;
		break;
	case BASE_TYPE_DDRC:
		common->base = clk_info->ddrc_base;
		break;
	case BASE_TYPE_AUDC:
		common->base = clk_info->audio_ctrl_base;
		break;
	case BASE_TYPE_APBC2:
		common->base = clk_info->apbc2_base;
		break;
#endif
	default:
		common->base = clk_info->apbc_base;
		break;

	}
	common->clk_tbl = clks;
	if(common->is_pll)
		pll->pll.lock_base = clk_info->mpmu_base;

	if(common->parent_name == NULL && common->parent_names != NULL)
		common->parent_name = common->parent_names[ccu_mix_get_parent(clk)];

	ret = clk_register(clk, common->driver_name, common->name, common->parent_name);

	return 0;
}

int spacemit_ccu_probe(struct spacemit_k1x_clk *clk_info,
		    struct spacemit_clk_table *clks)
{
	int i;

#ifdef CONFIG_SPL_BUILD
	for (i = CLK_PLL1_2457P6_SPL; i < clks->num ; i++) {
#else
	for (i = CLK_PLL1_2457P6; i < clks->num ; i++) {
#endif
		struct clk *clk = clks->clks[i];
		if (!clk)
			continue;

#ifdef CONFIG_SPL_BUILD
		if(clk->id >= CLK_VCTCXO_24_SPL)
#else
		if(clk->id >= CLK_VCTCXO_24)
#endif
			continue;

		clk->id = i;
		ccu_common_init(clk, clk_info, clks);
	}
#ifndef CONFIG_SPL_BUILD
	//init pll2 freq
	if (clk_info->pll2_freq) {
		struct clk *clk =clks->clks[CLK_PLL2];
		if (clk)
			clk_set_rate(clk, clk_info->pll2_freq);
	}
#endif
	//init clk default rate
	for (i = 0; i < ARRAY_SIZE(init_rate_tbl); i++) {
		struct clk *clk =clks->clks[init_rate_tbl[i].clk_id];
		if (!clk)
			continue;

		clk_set_rate(clk, init_rate_tbl[i].dft_rate);
	}

	return 0;
}

static inline void ccu_clk_dm(ulong id, struct clk *clk)
{
	if (!IS_ERR(clk)){

#ifdef CONFIG_SPL_BUILD
		id = transfer_clk_id_to_spl(id);
#endif
		clk->id = id;
		spacemit_k1x_clks.clks[id] = clk;
	}
}

static int spacemit_k1x_ccu_probe(struct udevice *dev)
{
	int ret = 0;
	struct spacemit_k1x_clk *clk_info = &k1x_clock_controller;
	struct spacemit_clk_table *clks = &spacemit_k1x_clks;

	pr_debug("init clock start \n");

	clk_info->mpmu_base = (void __iomem *)dev_remap_addr_index(dev, 0);
	if (!clk_info->mpmu_base) {
		pr_err("failed to map mpmu registers\n");
		goto out;
	}

	clk_info->apmu_base = (void __iomem *)dev_remap_addr_index(dev, 1);
	if (!clk_info->apmu_base) {
		pr_err("failed to map apmu registers\n");
		goto out;
	}

	clk_info->apbc_base = (void __iomem *)dev_remap_addr_index(dev, 2);
	if (!clk_info->apbc_base) {
		pr_err("failed to map apbc registers\n");
		goto out;
	}

	clk_info->apbs_base = (void __iomem *)dev_remap_addr_index(dev, 3);
	if (!clk_info->apbs_base) {
		pr_err("failed to map apbs registers\n");
		goto out;
	}

	clk_info->ciu_base = (void __iomem *)dev_remap_addr_index(dev, 4);
	if (!clk_info->ciu_base) {
		pr_err("failed to map ciu registers\n");
		goto out;
	}

	clk_info->dciu_base = (void __iomem *)dev_remap_addr_index(dev, 5);
	if (!clk_info->dciu_base) {
		pr_err("failed to map dragon ciu registers\n");
		goto out;
	}

	clk_info->ddrc_base = (void __iomem *)dev_remap_addr_index(dev, 6);
	if (!clk_info->ddrc_base) {
		pr_err("failed to map ddrc registers\n");
		goto out;
	}

	clk_info->apbc2_base = (void __iomem *)dev_remap_addr_index(dev, 7);
	if (!clk_info->apbc2_base) {
		pr_err("failed to map apbc2 registers\n");
		goto out;
	}

#ifdef CONFIG_SPL_BUILD
	clk_get_by_name(dev, "vctcxo_24", &vctcxo_24);
	ccu_clk_dm(CLK_VCTCXO_24_SPL, dev_get_clk_ptr(vctcxo_24.dev));
	clk_get_by_name(dev, "vctcxo_3", &vctcxo_3);
	ccu_clk_dm(CLK_VCTCXO_3_SPL, dev_get_clk_ptr(vctcxo_3.dev));
	clk_get_by_name(dev, "vctcxo_1", &vctcxo_1);
	ccu_clk_dm(CLK_VCTCXO_1_SPL, dev_get_clk_ptr(vctcxo_1.dev));
	clk_get_by_name(dev, "pll1_vco", &pll1_vco);
	ccu_clk_dm(CLK_PLL1_SPL, dev_get_clk_ptr(pll1_vco.dev));
	clk_get_by_name(dev, "clk_32k", &clk_32k);
	ccu_clk_dm(CLK_32K_SPL, dev_get_clk_ptr(clk_32k.dev));
	clk_get_by_name(dev, "clk_dummy", &clk_dummy);
	ccu_clk_dm(CLK_DUMMY_SPL, dev_get_clk_ptr(clk_dummy.dev));
#else
	clk_get_by_name(dev, "vctcxo_24", &vctcxo_24);
	ccu_clk_dm(CLK_VCTCXO_24, dev_get_clk_ptr(vctcxo_24.dev));
	clk_get_by_name(dev, "vctcxo_3", &vctcxo_3);
	ccu_clk_dm(CLK_VCTCXO_3, dev_get_clk_ptr(vctcxo_3.dev));
	clk_get_by_name(dev, "vctcxo_1", &vctcxo_1);
	ccu_clk_dm(CLK_VCTCXO_1, dev_get_clk_ptr(vctcxo_1.dev));
	clk_get_by_name(dev, "pll1_vco", &pll1_vco);
	ccu_clk_dm(CLK_PLL1, dev_get_clk_ptr(pll1_vco.dev));
	clk_get_by_name(dev, "clk_32k", &clk_32k);
	ccu_clk_dm(CLK_32K, dev_get_clk_ptr(clk_32k.dev));
	clk_get_by_name(dev, "clk_dummy", &clk_dummy);
	ccu_clk_dm(CLK_DUMMY, dev_get_clk_ptr(clk_dummy.dev));
#endif

	clk_info->pll2_freq = dev_read_u32_default(dev, "pll2-freq", 0);
	ret = spacemit_ccu_probe(clk_info, clks);
	pr_debug("init clock finish ret=%d \n", ret);
	if (!ret)
		return 0;
out:
	return -1;
}

static const struct udevice_id ccu_clk_ids[] = {
	{ .compatible = "spacemit,k1x-ccu" },
	{ },
};

U_BOOT_DRIVER(spacemit_k1x_ccu) = {
	.name = "k1x-ccu",
	.id = UCLASS_CLK,
	.of_match = ccu_clk_ids,
	.ops = &ccu_clk_ops,
	.probe = spacemit_k1x_ccu_probe,
};

