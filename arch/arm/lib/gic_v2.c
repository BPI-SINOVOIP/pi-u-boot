/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2017
 * Xiaoming Lu <xmlu@marvell.com>
 *
 * (C) Copyright 2020 Synaptics Incorporated
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 */

#include <common.h>
#include <bitfield.h>
#include <asm/gic.h>
#include <asm/io.h>


static void __iomem *gicd_base;
static void __iomem *gicc_base;
static int gic_v2_irq_num = 0;

static inline u32 gicd_readl(u32 offset)
{
	return readl(gicd_base + offset);
}

static inline u32 gicc_readl(u32 offset)
{
	return readl(gicc_base + offset);
}

static inline void gicd_writel(u32 offset, u32 data)
{
	writel(data, gicd_base + offset);
}

static inline void gicc_writel(u32 offset, u32 data)
{
	writel(data, gicc_base + offset);
}

void gic_v2_init(void __iomem *gicd, void __iomem *gicc, int *irq_num)
{
	gicd_base = gicd;
	gicc_base = gicc;

	u32 val;
	val = gicd_readl(GICD_TYPER);
	val &= 0x1F;
	gic_v2_irq_num = 32 * (val + 1);
	*irq_num = gic_v2_irq_num;

	/* Disable interrupts for Distribution & CPU Interface */
	gicd_writel(GICD_CTLR, 0);
	gicc_writel(GICC_CTLR, 0);

	int i;
	for (i = 0; i < *irq_num; i += 32) {
		/* Disable all interrupt source */
		gicd_writel(GICD_ICENABLERn + (i / 32) * 4, 0xFFFFFFFF);
		/* Clear all pending interrupts */
		gicd_writel(GICD_ICPENDRn + (i / 32) * 4, 0xFFFFFFFF);

		/* in case interrupt is not acked, clear active interrupts */
		val = gicd_readl(GICD_ICACTIVERn + (i / 32) * 4);
		if (val)
			gicd_writel(GICD_ICACTIVERn + (i / 32) * 4, val);

		/* Set all interrrupts as Group 1, generate IRQ */
		gicd_writel(GICD_IGROUPRn + (i / 32) * 4, 0xFFFFFFFF);
	}

	for (i = 0; i < *irq_num; i += 4) {
		/* Set all interrupt priorities to high */
		gicd_writel(GICD_IPRIORITYRn + (i / 4) * 4, 0x80808080);

		/* Clear Target */
		gicd_writel(GICD_ITARGETSRn + (i / 4) * 4, 0);
	}

	for (i = 0; i < *irq_num; i += 16) {
		/* Set all interrupt source to be level-sensitive and 1-N model */
		gicd_writel(GICD_ICFGR + (i / 16) * 4, 0x55555555);
	}

	/* Enable all interrupt priorities except for 0xf */
	gicc_writel(GICC_PMR, 0xFF);

	/* Enable pre-emption on all interrupts */
	gicc_writel(GICC_BPR, 0x3);

	/* Enable Group 0 & Group 1 and ActCtl for CPU interface */
	gicc_writel(GICC_CTLR, 0x7);

	/* Enable Group 0 & Group 1 for Distribution interface */
	gicd_writel(GICD_CTLR, 0x3);
}

void gic_v2_enable_irq(int irq)
{
	gicd_writel(GICD_ISENABLERn + (irq / 32) * 4, BIT(irq % 32));
	u32 val = gicd_readl(GICD_ITARGETSRn + (irq / 4) * 4);
	gicd_writel(GICD_ITARGETSRn + (irq / 4) * 4, bitfield_replace(val, 8 * (irq % 4), 8, 0x01));
}

void gic_v2_disable_irq(int irq)
{
	gicd_writel(GICD_ICENABLERn + (irq / 32) * 4, BIT(irq % 32));
	u32 val = gicd_readl(GICD_ITARGETSRn + (irq / 4) * 4);
	gicd_writel(GICD_ITARGETSRn + (irq / 4) * 4, bitfield_replace(val, 8 * (irq % 4), 8, 0x00));
}

int gic_v2_get_irq_id(void)
{
	int irq = gicc_readl(GICC_IAR) & 0x3ff;
	if (irq >= 1020 && irq <= 1021) {		/*  Reserved */
		printf("Receive illegal interrupt %d.\n", irq);
		return -ENOSYS;
	} else if (irq >= 1022 && irq <= 1023) {	/*  Spurious Interrupt */
		debug("Receive spurious interrupt.\n");
		return -EINVAL;
	} else if (irq >= gic_v2_irq_num || irq < 0) {
		printf("Receive illegal interrupt %d, mustn't larger than %d.\n", irq, gic_v2_irq_num);
		return -EPERM;
	}
	return irq;
}

void gic_v2_inactive_irq(int irq)
{
	gicc_writel(GICC_EOIR, irq & 0x3ff);
}

void gic_v2_sgi_trig(int irq)
{
	/* Generate Software Generate Interrupt, only send to this CPU */
	gicd_writel(GICD_SGIR, 0x02008000 | (irq & 0xf));
}
