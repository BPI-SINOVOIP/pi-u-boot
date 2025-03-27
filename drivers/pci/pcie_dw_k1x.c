// SPDX-License-Identifier: GPL-2.0+
/*
 * Spacemit k1x DesignWare based PCIe host controller driver
 *
 * Copyright (c) 2023, spacemit Corporation.
 *
 */
#include <common.h>
#include <dm.h>
#include <log.h>
#include <pci.h>
#include <generic-phy.h>
#include <power-domain.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <clk.h>
#include <reset.h>

#include "pcie_dw_common.h"

DECLARE_GLOBAL_DATA_PTR;

#define PCIE_VENDORID_MASK	GENMASK(15, 0)
#define PCIE_DEVICEID_SHIFT	16
#define K1X_PCIE_VENDOR_ID	0x201F
#define k1X_PCIE_DEVICE_ID	0x0001

#define PCIE_LINK_CAPABILITY		0x7c
#define PCIE_LINK_CTL_2			0xa0
#define TARGET_LINK_SPEED_MASK		0xf
#define LINK_SPEED_GEN_1		0x1
#define LINK_SPEED_GEN_2		0x2
#define LINK_SPEED_GEN_3		0x3

#define PCIE_MISC_CONTROL_1_OFF		0x8bc
#define PCIE_DBI_RO_WR_EN		BIT(0)

#define PLR_OFFSET			0x700
#define PCIE_PORT_DEBUG0		(PLR_OFFSET + 0x28)
#define PORT_LOGIC_LTSSM_STATE_MASK	0x1f
#define PORT_LOGIC_LTSSM_STATE_L0	0x11

#define PCIE_LINK_UP_TIMEOUT_MS		1000

/* Offsets from App base */
#define PCIE_CMD_STATUS			0x04
#define LTSSM_EN_VAL			BIT(0)



#define	PCIECTRL_K1X_CONF_DEVICE_CMD			0x0000
#define	LTSSM_EN					BIT(6)
/* Perst input value in ep mode */
#define	PCIE_PERST_IN					BIT(7)
/* Perst GPIO en in RC mode 1: perst# low, 0: perst# high */
#define	PCIE_RC_PERST					BIT(12)
/* Wake# GPIO in EP mode 1: Wake# low, 0: Wake# high */
#define	PCIE_EP_WAKE					BIT(13)
#define	APP_HOLD_PHY_RST				BIT(30)
/* BIT31 0: EP, 1: RC*/
#define	DEVICE_TYPE_RC					BIT(31)

#define	PCIE_CTRL_LOGIC					0x0004
#define	PCIE_IGNORE_PERSTN				BIT(2)

#define	K1X_PHY_AHB_LINK_STS				0x0004
#define	SMLH_LINK_UP					BIT(1)
#define	RDLH_LINK_UP					BIT(12)

/**
 *
 * @pci: The common PCIe DW structure
 * @app_base: The base address of application register space
 */
struct pcie_dw_k1x {
	/* Must be first member of the struct */
	struct pcie_dw dw;
	void __iomem		*base;		/* DT k1x_conf */
	void __iomem		*phy_ahb;		/* DT phy_ahb */
	void __iomem		*phy_addr;		/* DT phy_addr */
	void __iomem		*conf0_addr;		/* DT conf0_addr */
	void __iomem		*phy0_addr;		/* DT phy0_addr */
	int			port_id;

	/* reset, clock resources */
	struct clk clock;
	struct reset_ctl reset;
	struct gpio_desc pwr_on_gpio;
	int power_on_status;
};

enum dw_pcie_device_mode {
	DW_PCIE_UNKNOWN_TYPE,
	DW_PCIE_EP_TYPE,
	DW_PCIE_RC_TYPE,
};

static inline u32 k1x_pcie_readl(struct pcie_dw_k1x *pcie, u32 offset)
{
	return readl(pcie->base + offset);
}

static inline void k1x_pcie_writel(struct pcie_dw_k1x *pcie, u32 offset,
				      u32 value)
{
	writel(value, pcie->base + offset);
}

static inline u32 k1x_pcie_phy_ahb_readl(struct pcie_dw_k1x *pcie, u32 offset)
{
	return readl(pcie->phy_ahb + offset);
}

static inline void k1x_pcie_phy_ahb_writel(struct pcie_dw_k1x *pcie, u32 offset,
				      u32 value)
{
	writel(value, pcie->phy_ahb + offset);
}

static inline u32 k1x_pcie_phy_reg_readl(struct pcie_dw_k1x *pcie, u32 offset)
{
	return readl(pcie->phy_addr + offset);
}

static inline void k1x_pcie_phy_reg_writel(struct pcie_dw_k1x *pcie, u32 offset,
				      u32 value)
{
	writel(value, pcie->phy_addr + offset);
}

static inline u32 k1x_pcie_conf0_reg_readl(struct pcie_dw_k1x *pcie, u32 offset)
{
	return readl(pcie->conf0_addr + offset);
}

static inline void k1x_pcie_conf0_reg_writel(struct pcie_dw_k1x *pcie, u32 offset,
				      u32 value)
{
	writel(value, pcie->conf0_addr + offset);
}

static inline u32 k1x_pcie_phy0_reg_readl(struct pcie_dw_k1x *pcie, u32 offset)
{
	return readl(pcie->phy0_addr + offset);
}

static inline void k1x_pcie_phy0_reg_writel(struct pcie_dw_k1x *pcie, u32 offset,
				      u32 value)
{
	writel(value, pcie->phy0_addr + offset);
}

/**
 * pcie_dw_configure() - Configure link capabilities and speed
 *
 * @regs_base: A pointer to the PCIe controller registers
 * @cap_speed: The capabilities and speed to configure
 *
 * Configure the link capabilities and speed in the PCIe root complex.
 */
static void pcie_dw_configure(struct pcie_dw_k1x *pci, u32 cap_speed)
{
	u32 val;

	dw_pcie_dbi_write_enable(&pci->dw, true);

	val = readl(pci->dw.dbi_base + PCIE_LINK_CAPABILITY);
	val &= ~TARGET_LINK_SPEED_MASK;
	val |= cap_speed;
	writel(val, pci->dw.dbi_base + PCIE_LINK_CAPABILITY);

	val = readl(pci->dw.dbi_base + PCIE_LINK_CTL_2);
	val &= ~TARGET_LINK_SPEED_MASK;
	val |= cap_speed;
	writel(val, pci->dw.dbi_base + PCIE_LINK_CTL_2);

	dw_pcie_dbi_write_enable(&pci->dw, false);
}

/**
 * is_link_up() - Return the link state
 *
 * @regs_base: A pointer to the PCIe DBICS registers
 *
 * Return: 1 (true) for active line and 0 (false) for no link
 */
static int is_link_up(struct pcie_dw_k1x *pci)
{
	u32 val;

	val = readl(pci->dw.dbi_base + PCIE_PORT_DEBUG0);
	val &= PORT_LOGIC_LTSSM_STATE_MASK;

	return (val == PORT_LOGIC_LTSSM_STATE_L0);
}

/**
 * wait_link_up() - Wait for the link to come up
 *
 * @regs_base: A pointer to the PCIe controller registers
 *
 * Return: 1 (true) for active line and 0 (false) for no link (timeout)
 */
static int wait_link_up(struct pcie_dw_k1x *pci)
{
	unsigned long timeout;

	timeout = get_timer(0) + PCIE_LINK_UP_TIMEOUT_MS;
	while (!is_link_up(pci)) {
		if (get_timer(0) > timeout)
			return 0;
	};

	return 1;
}

static int pcie_dw_k1x_pcie_link_up(struct pcie_dw_k1x *pci, u32 cap_speed)
{
	u32 reg;

	if (is_link_up(pci)) {
		printf("PCI Link already up before configuration!\n");
		return 1;
	}

	/* DW pre link configurations */
	pcie_dw_configure(pci, cap_speed);

	/* Initiate link training */
	reg = k1x_pcie_readl(pci, PCIECTRL_K1X_CONF_DEVICE_CMD);
	reg |= LTSSM_EN;
	reg &= ~APP_HOLD_PHY_RST;
	k1x_pcie_writel(pci, PCIECTRL_K1X_CONF_DEVICE_CMD, reg);

	/* Check that link was established */
	if (!wait_link_up(pci))
		return 0;

	/*
	 * Link can be established in Gen 1. still need to wait
	 * till MAC nagaotiation is completed
	 */
	udelay(100);

	return 1;
}

static int pcie_set_mode(struct pcie_dw_k1x *pci,
			       enum dw_pcie_device_mode mode)
{
	u32 reg;

	switch (mode) {
	case DW_PCIE_RC_TYPE:
		reg = k1x_pcie_readl(pci, PCIECTRL_K1X_CONF_DEVICE_CMD);
		reg |= DEVICE_TYPE_RC;
		k1x_pcie_writel(pci, PCIECTRL_K1X_CONF_DEVICE_CMD, reg);

		reg = k1x_pcie_readl(pci, PCIE_CTRL_LOGIC);
		reg |= PCIE_IGNORE_PERSTN;
		k1x_pcie_writel(pci, PCIE_CTRL_LOGIC, reg);
		break;
	case DW_PCIE_EP_TYPE:
		reg = k1x_pcie_readl(pci, PCIECTRL_K1X_CONF_DEVICE_CMD);
		reg &= ~DEVICE_TYPE_RC;
		k1x_pcie_writel(pci, PCIECTRL_K1X_CONF_DEVICE_CMD, reg);
		break;
	default:
		dev_err(pci->dw.dev, "INVALID device type %d\n", mode);
		return -EINVAL;
	}

	return 0;
}

static int k1x_pcie_host_init(struct pcie_dw_k1x *pci)
{
	u32 reg;

	mdelay(100);
	/* set Perst# gpio high state*/
	reg = k1x_pcie_readl(pci, PCIECTRL_K1X_CONF_DEVICE_CMD);
	reg &= ~PCIE_RC_PERST;
	k1x_pcie_writel(pci, PCIECTRL_K1X_CONF_DEVICE_CMD, reg);

	return 0;
}

#define PCIE_REF_CLK_OUTPUT
static int porta_init_done = 0;
// wait porta rterm done
void porta_rterm(struct pcie_dw_k1x *k1x)
{
	int rd_data;
	u32 val;

	//REG32(PMUA_REG_BASE + 0x3CC) = 0x4000003f;
	val = 0x4000003f;
	k1x_pcie_conf0_reg_writel(k1x, 0 , val);

	//REG32(PMUA_REG_BASE + 0x3CC) &= 0xbfffffff; // clear hold phy reset
	val = k1x_pcie_conf0_reg_readl(k1x, 0);
	val &= 0xbfffffff;
	k1x_pcie_conf0_reg_writel(k1x, 0 , val);

	// set refclk model
	//REG32(0xC0B10000 + (0x17 << 2)) |= (0x1 << 10);
	val = k1x_pcie_phy0_reg_readl(k1x, (0x17 << 2));
	val |= (0x1 << 10);
	k1x_pcie_phy0_reg_writel(k1x, (0x17 << 2), val);

	//REG32(0xC0B10000 + (0x17 << 2)) &= ~(0x3 << 8);
	val = k1x_pcie_phy0_reg_readl(k1x, (0x17 << 2));
	val &= ~(0x3 << 8);
	k1x_pcie_phy0_reg_writel(k1x, (0x17 << 2), val);


#ifndef PCIE_REF_CLK_OUTPUT
	// receiver mode
	//REG32(0xC0B10000 + (0x17 << 2)) |= 0x2 << 8;
	val = k1x_pcie_phy0_reg_readl(k1x, (0x17 << 2));
	val |= 0x2 << 8;
	k1x_pcie_phy0_reg_writel(k1x, (0x17 << 2), val);

	//REG32(0xC0B10000 + (0x8 << 2)) &= ~(0x1 << 29);
	val = k1x_pcie_phy0_reg_readl(k1x, (0x8 << 2));
	val &= ~(0x1 << 29);
	k1x_pcie_phy0_reg_writel(k1x, (0x8 << 2), val);
#ifdef PCIE_SEL_24M_REF_CLK
	//REG32(0xC0B10000 + (0x12 << 2)) &= 0xffff0fff;
	val = k1x_pcie_phy0_reg_readl(k1x, (0x12 << 2));
	val &= 0xffff0fff;
	k1x_pcie_phy0_reg_writel(k1x, (0x12 << 2), val);

	//REG32(0xC0B10000 + (0x12 << 2)) |= 0x00002000; // select 24Mhz refclock input pll_reg1[15:13]=2
	val = k1x_pcie_phy0_reg_readl(k1x, (0x12 << 2));
	val |= 0x00002000;
	k1x_pcie_phy0_reg_writel(k1x, (0x12 << 2), val);

	//REG32(0xC0B10000 + (0x8 << 2)) |= 0x3 << 29;	   // rc_cal_reg2 0x68
	val = k1x_pcie_phy0_reg_readl(k1x, (0x8 << 2));
	val |= 0x3 << 29;
	k1x_pcie_phy0_reg_writel(k1x, (0x8 << 2), val);
#elif PCIE_SEL_100M_REF_CLK
	//REG32(0xC0B10000 + (0x8 << 2)) |= 0x1 << 30; // rc_cal_reg2 0x48
	val = k1x_pcie_phy0_reg_readl(k1x, (0x8 << 2));
	val |= 0x1 << 30;
	k1x_pcie_phy0_reg_writel(k1x, (0x8 << 2), val);
#endif
	//REG32(0xC0B10000 + (0x14 << 2)) |= (0x1 << 3); // pll_reg9[3] en_rterm,only enable in receiver mode
	val = k1x_pcie_phy0_reg_readl(k1x, (0x14 << 2));
	val |= (0x1 << 3);
	k1x_pcie_phy0_reg_writel(k1x, (0x14 << 2), val);
#else
	// driver mode
	//REG32(0xC0B10000 + (0x17 << 2)) |= 0x1 << 8;
	val = k1x_pcie_phy0_reg_readl(k1x, (0x17 << 2));
	val |= 0x1 << 8;
	k1x_pcie_phy0_reg_writel(k1x, (0x17 << 2), val);

	//REG32(0xC0B10000 + 0x400 + (0x17 << 2)) |= 0x1 << 8;
	val = k1x_pcie_phy0_reg_readl(k1x, 0x400 + (0x17 << 2));
	val |= 0x1 << 8;
	k1x_pcie_phy0_reg_writel(k1x, 0x400 + (0x17 << 2), val);

	//REG32(0xC0B10000 + (0x12 << 2)) &= 0xffff0fff;
	val = k1x_pcie_phy0_reg_readl(k1x, (0x12 << 2));
	val &= 0xffff0fff;
	k1x_pcie_phy0_reg_writel(k1x, (0x12 << 2), val);

	//REG32(0xC0B10000 + (0x12 << 2)) |= 0x00002000; // select 24Mhz refclock input pll_reg1[15:13]=2
	val = k1x_pcie_phy0_reg_readl(k1x, (0x12 << 2));
	val |= 0x00002000;
	k1x_pcie_phy0_reg_writel(k1x, (0x12 << 2), val);

	//REG32(0xC0B10000 + (0x13 << 2)) |= (0x1 << 4); // pll_reg5[4] of lane0, enable refclk_100_n/p 100Mhz output
	val = k1x_pcie_phy0_reg_readl(k1x, (0x13 << 2));
	val |= (0x1 << 4);
	k1x_pcie_phy0_reg_writel(k1x, (0x13 << 2), val);

	//// REG32(0xC0B10000+(0x14<<2)) |= (0x1<<3);//pll_reg9[3] en_rterm,only enable in receiver mode
#endif

	//REG32(0xC0B10000 + (0x12 << 2)) &= 0xfff0ffff; // pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
	val = k1x_pcie_phy0_reg_readl(k1x, (0x12 << 2));
	val &= 0xfff0ffff;
	k1x_pcie_phy0_reg_writel(k1x, (0x12 << 2), val);

	//REG32(0xC0B10000 + (0x02 << 2)) = 0x00000B78; // PU_ADDR_CLK_CFG of lane0
	val = 0x00000B78;
	k1x_pcie_phy0_reg_writel(k1x, (0x02 << 2), val);

	//REG32(0xC0B10000 + (0x06 << 2)) = 0x00000400; // force rcv done
	val = 0x00000400;
	k1x_pcie_phy0_reg_writel(k1x, (0x06 << 2), val);
	printk("Now waiting portA resister tuning done...\n");

	// force PCIE mpu_u3/pu_rx_lfps
	//REG32(PCIE_PUPHY_REG_BASE + 0x6 * 4) |= (0x1 << 17) | (0x1 << 15);
	val = k1x_pcie_phy_reg_readl(k1x, (0x6 * 4));
	val |= ((0x1 << 17) | (0x1 << 15));
	k1x_pcie_phy_reg_writel(k1x, (0x6 * 4), val);

	// wait pm0 rterm done
	do
	{
		//rd_data = REG32(0xC0B10000 + 0x21 * 4);
		rd_data = k1x_pcie_phy0_reg_readl(k1x, (0x21 * 4));
		printk("porta redonly_reg2: %08x\n", rd_data);
	} while (((rd_data >> 10) & 0x1) == 0); // waiting PCIe portA readonly_reg2[2] r_tune_done==1
}

// force rterm value to porta/b/c
void rterm_force(struct pcie_dw_k1x *k1x, u32 pcie_rcal)
{
	int i, lane;
	u32 val = 0;

	if (k1x->port_id != 0x0) {
		lane = 2;
	} else {
		lane = 1;
	}

	printk("pcie_rcal = 0x%08x\n", pcie_rcal);
	printk("pcie port id = %d, lane num = %d\n", k1x->port_id, lane);

	// 2.write pma0 rterm value LSB[3:0](read0nly1[3:0]) to lane0/1 rx_reg1
	for (i = 0; i < lane; i++)
	{
		val = k1x_pcie_phy_reg_readl(k1x, ((0x14 << 2) + 0x400 * i));
		val |= ((pcie_rcal & 0xf) << 8);
		k1x_pcie_phy_reg_writel(k1x, ((0x14 << 2) + 0x400 * i), val);
	}
	// 3.set lane0/1 rx_reg4 bit5=0
	for (i = 0; i < lane; i++)
	{
		val = k1x_pcie_phy_reg_readl(k1x, ((0x15 << 2) + 0x400 * i));
		val &= ~(1 << 5);
		k1x_pcie_phy_reg_writel(k1x, ((0x15 << 2) + 0x400 * i), val);
	}

	// 4.write pma0 rterm value MSB[7:4](readonly1[7:4]) to lane0/1 tx_reg1[7:4]
	for (i = 0; i < lane; i++)
	{
		val = k1x_pcie_phy_reg_readl(k1x, ((0x19 << 2) + 0x400 * i));
		val |= ((pcie_rcal >> 4) & 0xf) << 12;
		k1x_pcie_phy_reg_writel(k1x, ((0x19 << 2) + 0x400 * i), val);
	}

	// 5.set lane0/1 tx_reg3 bit1=1
	for (i = 0; i < lane; i++)
	{
		val = k1x_pcie_phy_reg_readl(k1x, ((0x19 << 2) + 0x400 * i));
		val |= (1 << 25);
		k1x_pcie_phy_reg_writel(k1x, ((0x19 << 2) + 0x400 * i), val);
	}

	// 6.adjust rc calrefclk freq
#ifndef PCIE_REF_CLK_OUTPUT
	//REG32(PCIE_PUPHY_REG_BASE + (0x8 << 2)) &= ~(0x1 << 29);
	val = k1x_pcie_phy_reg_readl(k1x,  (0x8 << 2));
	val &= ~(0x1 << 29);
	k1x_pcie_phy_reg_writel(k1x,  (0x8 << 2), val);
#ifdef PCIE_SEL_24M_REF_CLK
	//REG32(PCIE_PUPHY_REG_BASE + (0x8 << 2)) |= 0x3 << 29; // rc_cal_reg2 0x68
	val = k1x_pcie_phy_reg_readl(k1x,  (0x8 << 2));
	val |= 0x3 << 29;
	k1x_pcie_phy_reg_writel(k1x,  (0x8 << 2), val);
#elif PCIE_SEL_100M_REF_CLK
	//REG32(PCIE_PUPHY_REG_BASE + (0x8 << 2)) |= 0x1 << 30; // rc_cal_reg2 0x48
	val = k1x_pcie_phy_reg_readl(k1x,  (0x8 << 2));
	val |= 0x1 << 30;
	k1x_pcie_phy_reg_writel(k1x,  (0x8 << 2), val);
#endif
#else
	//REG32(PCIE_PUPHY_REG_BASE + (0x8 << 2)) |= 0x3 << 29;
	val = k1x_pcie_phy_reg_readl(k1x,  (0x8 << 2));
	val |= 0x3 << 29;
	k1x_pcie_phy_reg_writel(k1x,  (0x8 << 2), val);
#endif

	// 7.set lane0/1 rc_cal_reg1[6]=1
	for (i = 0; i < lane; i++)
	{
		val = k1x_pcie_phy_reg_readl(k1x, ((0x8 << 2) + 0x400 * i));
		val &= ~(1 << 22);
		k1x_pcie_phy_reg_writel(k1x, ((0x8 << 2) + 0x400 * i), val);
	}
	for (i = 0; i < lane; i++)
	{
		val = k1x_pcie_phy_reg_readl(k1x, ((0x8 << 2) + 0x400 * i));
		val |= (1 << 22);
		k1x_pcie_phy_reg_writel(k1x, ((0x8 << 2) + 0x400 * i), val);
	}

	// release forc PCIE mpu_u3/pu_rx_lfps
	val = k1x_pcie_phy_reg_readl(k1x, 0x6 * 4);
	val &= 0xFFFD7FFF;
	k1x_pcie_phy_reg_writel(k1x, 0x6 * 4, val);
}

static int init_phy(struct pcie_dw_k1x *k1x)
{
	u32 rd_data, pcie_rcal;
	u32 val = 0;

	printk("Now init Rterm...\n");
	printk("pcie prot id = %d, porta_init_done = %d\n", k1x->port_id, porta_init_done);
	if (k1x->port_id != 0) {
	    if (porta_init_done == 0) {
			porta_rterm(k1x);
			//pcie_rcal = REG32(0xC0B10000 + (0x21 << 2));
			pcie_rcal = k1x_pcie_phy0_reg_readl(k1x,  (0x21 << 2));

			//REG32(PMUA_REG_BASE + 0x3CC) &= ~0x4000003f;
			val = k1x_pcie_conf0_reg_readl(k1x, 0);
			val &= ~0x4000003f;
			k1x_pcie_conf0_reg_writel(k1x, 0, val);
		} else {
			//pcie_rcal = REG32(0xC0B10000 + (0x21 << 2));
			pcie_rcal = k1x_pcie_phy0_reg_readl(k1x,  (0x21 << 2));
		}
	} else {
		do {
			//rd_data = REG32(0xC0B10000 + 0x21 * 4);
			rd_data = k1x_pcie_phy0_reg_readl(k1x,  (0x21 * 4));
		} while (((rd_data >> 10) & 0x1) == 0);
		//pcie_rcal = REG32(0xC0B10000 + (0x21 << 2));
		pcie_rcal = k1x_pcie_phy0_reg_readl(k1x,  (0x21 << 2));
	}

	rterm_force(k1x, pcie_rcal);

	printk("Now int init_puphy...\n");
	val = k1x_pcie_readl(k1x, PCIECTRL_K1X_CONF_DEVICE_CMD);
	val &= 0xbfffffff;
	k1x_pcie_writel(k1x, PCIECTRL_K1X_CONF_DEVICE_CMD, val);

	// set refclk model
	val = k1x_pcie_phy_reg_readl(k1x, (0x17 << 2));
	val |= (0x1 << 10);
	k1x_pcie_phy_reg_writel(k1x, (0x17 << 2), val);

	val = k1x_pcie_phy_reg_readl(k1x, (0x17 << 2));
	val &= ~(0x3 << 8);
	k1x_pcie_phy_reg_writel(k1x, (0x17 << 2), val);

	val = k1x_pcie_phy_reg_readl(k1x, 0x400 + (0x17 << 2));
	val |= (0x1 << 10);
	k1x_pcie_phy_reg_writel(k1x, 0x400 + (0x17 << 2), val);

	val = k1x_pcie_phy_reg_readl(k1x, 0x400 + (0x17 << 2));
	val &= ~(0x3 << 8);
	k1x_pcie_phy_reg_writel(k1x, 0x400+ (0x17 << 2), val);
#ifndef PCIE_REF_CLK_OUTPUT
	// receiver mode
	REG32(PCIE_PUPHY_REG_BASE + (0x17 << 2)) |= 0x2 << 8;
	REG32(PCIE_PUPHY_REG_BASE + 0x400 + (0x17 << 2)) |= 0x2 << 8;
#ifdef PCIE_SEL_24M_REF_CLK
	val = k1x_pcie_phy_reg_readl(k1x, (0x12 << 2));
	val &= 0xffff0fff;
	k1x_pcie_phy_reg_writel(k1x, (0x12 << 2), val);

	val = k1x_pcie_phy_reg_readl(k1x, (0x12 << 2));
	val |= 0x00002000;
	k1x_pcie_phy_reg_writel(k1x, (0x12 << 2), val);
#endif
#else
	// driver mode
	val = k1x_pcie_phy_reg_readl(k1x, (0x17 << 2));
	val |= 0x1 << 8;
	k1x_pcie_phy_reg_writel(k1x, (0x17 << 2), val);

	val = k1x_pcie_phy_reg_readl(k1x, 0x400 + (0x17 << 2));
	val |= 0x1 << 8;
	k1x_pcie_phy_reg_writel(k1x, 0x400 + (0x17 << 2), val);

	val = k1x_pcie_phy_reg_readl(k1x, (0x12 << 2));
	val &= 0xffff0fff;
	k1x_pcie_phy_reg_writel(k1x, (0x12 << 2), val);

	val = k1x_pcie_phy_reg_readl(k1x, (0x12 << 2));
	val |= 0x00002000;
	k1x_pcie_phy_reg_writel(k1x, (0x12 << 2), val);

	val = k1x_pcie_phy_reg_readl(k1x, (0x13 << 2));
	val |= (0x1 << 4);
	k1x_pcie_phy_reg_writel(k1x, (0x13 << 2), val);

	if (k1x->port_id == 0x0) {
		//REG32(0xC0B10000+(0x14<<2)) |= (0x1<<3);//pll_reg9[3] en_rterm,only enable in receiver mode
		val = k1x_pcie_phy0_reg_readl(k1x,  (0x14 << 2));
		val |= (0x1 << 3);
		k1x_pcie_phy0_reg_writel(k1x,  (0x14 << 2), val);
	}
#endif

	// pll_reg1 of lane0, disable ssc pll_reg4[3:0]=4'h0
	val = k1x_pcie_phy_reg_readl(k1x, (0x12 << 2));
	val &= 0xfff0ffff;
	k1x_pcie_phy_reg_writel(k1x, (0x12 << 2), val);

	// PU_ADDR_CLK_CFG of lane0
	val = 0x00000B78;
	k1x_pcie_phy_reg_writel(k1x, (0x02 << 2), val);

	 // PU_ADDR_CLK_CFG of lane1
	val = 0x00000B78;
	k1x_pcie_phy_reg_writel(k1x, 0x400 + (0x02 << 2), val);

	// force rcv done
	val = 0x00000400;
	k1x_pcie_phy_reg_writel(k1x, (0x06 << 2), val);

	// waiting pll lock
	printk("waiting pll lock...\n");
	do
	{
		rd_data = k1x_pcie_phy_reg_readl(k1x, 0x8);
	} while ((rd_data & 0x1) == 0);

	if (k1x->port_id == 0)
		porta_init_done = 0x1;
	printk("Now finish init_puphy....\n");
	return 0;
}

static int pcie_dw_init_id(struct pcie_dw_k1x *pci)
{
	dw_pcie_dbi_write_enable(&pci->dw, true);
	writew(K1X_PCIE_VENDOR_ID, pci->dw.dbi_base + PCI_VENDOR_ID);
	writew(k1X_PCIE_DEVICE_ID, pci->dw.dbi_base + PCI_DEVICE_ID);
	dw_pcie_dbi_write_enable(&pci->dw, false);

	return 0;
}

static int k1x_power_on(struct pcie_dw_k1x *pci, int on)
{
	struct pcie_dw *dwp = &pci->dw;
	int gpio_val = 0;

	if (on) {
		if (pci->power_on_status) {
			gpio_val = 1;
		} else {
			gpio_val = 0;
		}
	} else {
		if (pci->power_on_status) {
			gpio_val = 0;
		} else {
			gpio_val = 1;
		}
	}

	if (dm_gpio_is_valid(&pci->pwr_on_gpio)) {
		dev_info(dwp->dev, "PCIe interface power %s, set gpio %d to %d\n", \
			on ? "on": "off", pci->pwr_on_gpio.offset, gpio_val);
		dm_gpio_set_value(&pci->pwr_on_gpio, gpio_val);
	}

	return 0;
}

/**
 * pcie_dw_k1x_probe() - Probe the PCIe bus for active link
 *
 * @dev: A pointer to the device being operated on
 *
 * Probe for an active link on the PCIe bus and configure the controller
 * to enable this port.
 *
 * Return: 0 on success, else -ENODEV
 */
static int pcie_dw_k1x_probe(struct udevice *dev)
{
	struct pcie_dw_k1x *pci = dev_get_priv(dev);
	struct udevice *ctlr = pci_get_controller(dev);
	struct pci_controller *hose = dev_get_uclass_priv(ctlr);
	struct phy phy0, phy1;
	int ret;
	u32 reg;

	/* enable pcie clk */
	clk_enable(&pci->clock);
	reset_deassert(&pci->reset);

	reg = k1x_pcie_readl(pci, PCIECTRL_K1X_CONF_DEVICE_CMD);
	reg &= ~LTSSM_EN;
	k1x_pcie_writel(pci, PCIECTRL_K1X_CONF_DEVICE_CMD, reg);

	/* set Perst# (fundamental reset) gpio low state*/
	reg = k1x_pcie_readl(pci, PCIECTRL_K1X_CONF_DEVICE_CMD);
	reg |= PCIE_RC_PERST;
	k1x_pcie_writel(pci, PCIECTRL_K1X_CONF_DEVICE_CMD, reg);

	init_phy(pci);

	ret = generic_phy_get_by_name(dev,  "pcie-phy0", &phy0);
	if (ret) {
		dev_err(dev, "Unable to get phy0\n");
	} else {
		generic_phy_reset(&phy0);
		generic_phy_init(&phy0);
		generic_phy_power_on(&phy0);
	}

	ret = generic_phy_get_by_name(dev,  "pcie-phy1", &phy1);
	if (ret) {
		dev_err(dev, "Unable to get phy1\n");
	} else {
		generic_phy_reset(&phy1);
		generic_phy_init(&phy1);
		generic_phy_power_on(&phy1);
	}

	pci->dw.first_busno = dev_seq(dev);
	pci->dw.dev = dev;

	pcie_set_mode(pci, DW_PCIE_RC_TYPE);

	/* power on the interface */
	k1x_power_on(pci, 1);

	k1x_pcie_host_init(pci);
	pcie_dw_setup_host(&pci->dw);
	pcie_dw_init_id(pci);

	if (!pcie_dw_k1x_pcie_link_up(pci, LINK_SPEED_GEN_2)) {
		printf("PCIE-%d: Link down\n", dev_seq(dev));
		return -ENODEV;
	}

	printf("PCIE-%d: Link up (Gen%d-x%d, Bus%d)\n", dev_seq(dev),
	       pcie_dw_get_link_speed(&pci->dw),
	       pcie_dw_get_link_width(&pci->dw),
	       hose->first_busno);

	ret = pcie_dw_prog_outbound_atu_unroll(&pci->dw, PCIE_ATU_REGION_INDEX0,
					 PCIE_ATU_TYPE_MEM,
					 pci->dw.mem.phys_start,
					 pci->dw.mem.bus_start, pci->dw.mem.size);
	return 0;
}

/**
 * pcie_dw_k1x_of_to_plat() - Translate from DT to device state
 *
 * @dev: A pointer to the device being operated on
 *
 * Translate relevant data from the device tree pertaining to device @dev into
 * state that the driver will later make use of. This state is stored in the
 * device's private data structure.
 *
 * Return: 0 on success, else -EINVAL
 */
static int pcie_dw_k1x_of_to_plat(struct udevice *dev)
{
	int ret = 0;
	struct pcie_dw_k1x *pcie = dev_get_priv(dev);

	/* Get the controller base address */
	pcie->dw.dbi_base = (void *)dev_read_addr_name(dev, "dbi");
	if ((fdt_addr_t)pcie->dw.dbi_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the config space base address and size */
	pcie->dw.cfg_base = (void *)dev_read_addr_size_name(dev, "config",
							 &pcie->dw.cfg_size);
	if ((fdt_addr_t)pcie->dw.cfg_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the iATU base address and size */
	pcie->dw.atu_base = (void *)dev_read_addr_name(dev, "atu");
	if ((fdt_addr_t)pcie->dw.atu_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the app base address */
	pcie->phy_ahb = (void *)dev_read_addr_name(dev, "phy_ahb");
	if ((fdt_addr_t)pcie->phy_ahb == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the PMU base address */
	pcie->base = (void *)dev_read_addr_name(dev, "k1x_conf");
	if ((fdt_addr_t)pcie->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the phy base address and size */
	pcie->phy_addr = (void *)dev_read_addr_name(dev, "phy_addr");
	if ((fdt_addr_t)pcie->phy_addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the pcie0 conf base address */
	pcie->conf0_addr = (void *)dev_read_addr_name(dev, "conf0_addr");
	if ((fdt_addr_t)pcie->conf0_addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* Get the pcie0 phy base address and size */
	pcie->phy0_addr = (void *)dev_read_addr_name(dev, "phy0_addr");
	if ((fdt_addr_t)pcie->phy0_addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	ret = dev_read_u32(dev, "k1x,pcie-port", &pcie->port_id);
	if (ret)
		return ret;

	ret = clk_get_by_index(dev, 0, &pcie->clock);
	if (ret) {
		dev_warn(dev, "It has no clk: %d\n", ret);
		return -EINVAL;
	}

	ret = reset_get_by_index(dev, 0, &pcie->reset);
	if (ret) {
		dev_warn(dev, "It has no reset: %d\n", ret);
		return -EINVAL;
	}

	gpio_request_by_name(dev, "k1x,pwr_on", 0, &pcie->pwr_on_gpio,
			     GPIOD_IS_OUT);
	if (!dm_gpio_is_valid(&pcie->pwr_on_gpio)) {
		dev_info(dev, "has no power on gpio.\n");
	}

	ret = dev_read_u32(dev, "power-on-status", &pcie->power_on_status);
	if (ret) {
		dev_info(dev, "has no power-on-status flag, use default.\n");
		pcie->power_on_status = 1;
	}

	return 0;
}

static const struct dm_pci_ops pcie_dw_k1x_ops = {
	.read_config	= pcie_dw_read_config,
	.write_config	= pcie_dw_write_config,
};

static const struct udevice_id pcie_dw_k1x_ids[] = {
	{ .compatible = "k1x,dwc-pcie" },
	{ }
};

U_BOOT_DRIVER(pcie_dw_k1x) = {
	.name			= "pcie_dw_k1x",
	.id			= UCLASS_PCI,
	.of_match		= pcie_dw_k1x_ids,
	.ops			= &pcie_dw_k1x_ops,
	.of_to_plat	= pcie_dw_k1x_of_to_plat,
	.probe			= pcie_dw_k1x_probe,
	.priv_auto	= sizeof(struct pcie_dw_k1x),
};
