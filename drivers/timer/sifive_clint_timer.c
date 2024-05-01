// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020, Sean Anderson <seanga2@gmail.com>
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <timer.h>
#include <asm/io.h>
#include <asm/csr.h>
#include <dm/device-internal.h>
#include <linux/err.h>

/* mtime register */
#define MTIME_REG(base)			((ulong)(base) + 0xbff8)

static u64 notrace sifive_clint_get_count(struct udevice *dev)
{
	__maybe_unused u32 hi, lo;

	if (IS_ENABLED(CONFIG_64BIT))
		return csr_read(CSR_TIME);

	do {
		hi = csr_read(CSR_TIMEH);
		lo = csr_read(CSR_TIME);
	} while (hi != csr_read(CSR_TIMEH));

	return ((u64)hi << 32) | lo;
}

#if CONFIG_IS_ENABLED(RISCV_MMODE) && IS_ENABLED(CONFIG_TIMER_EARLY)
/**
 * timer_early_get_rate() - Get the timer rate before driver model
 */
unsigned long notrace timer_early_get_rate(void)
{
	return RISCV_MMODE_TIMER_FREQ;
}

/**
 * timer_early_get_count() - Get the timer count before driver model
 *
 */
u64 notrace timer_early_get_count(void)
{
	return sifive_clint_get_count(NULL);
}
#endif

static const struct timer_ops sifive_clint_ops = {
	.get_count = sifive_clint_get_count,
};

static int sifive_clint_probe(struct udevice *dev)
{
	dev_set_priv(dev, dev_read_addr_ptr(dev));
	if (!dev_get_priv(dev))
		return -EINVAL;

	return timer_timebase_fallback(dev);
}

static const struct udevice_id sifive_clint_ids[] = {
	{ .compatible = "riscv,clint0" },
	{ .compatible = "sifive,clint0" },
	{ }
};

U_BOOT_DRIVER(sifive_clint) = {
	.name		= "sifive_clint",
	.id		= UCLASS_TIMER,
	.of_match	= sifive_clint_ids,
	.probe		= sifive_clint_probe,
	.ops		= &sifive_clint_ops,
	.flags		= DM_FLAG_PRE_RELOC,
};
