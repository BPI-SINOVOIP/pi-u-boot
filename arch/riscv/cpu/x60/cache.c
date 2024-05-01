// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <asm/cache.h>
#include <cache.h>
#include <asm/csr.h>

void icache_enable(void)
{
#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	/* csr:0x7c0 can be accessed in MMODE only */
	asm volatile("csrsi 0x7c0, 0x2 \n\t");
#endif
#endif
}

void icache_disable(void)
{
#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	/* csr:0x7c0 can be accessed in MMODE only */
	asm volatile("csrci 0x7c0, 0x2 \n\t");
#endif
#endif
}

int icache_status(void)
{
	int ret = 0;

#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
	/* 
	  if the I$ is enabled by configuration, 
	  set icache is enabled as default, it will be
	  updated with csr:0x7c0 if it can be accessed
	*/
	ret = 1;
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	asm volatile (
		"csrr t1, 0x7c0\n\t"
		"andi	%0, t1, 0x02\n\t"
		: "=r" (ret)
		:
		: "memory"
	);
#endif
#endif

	return ret;
}

void dcache_enable(void)
{
#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	/* csr:0x7c0 can be accessed in MMODE only */
	asm volatile("csrsi 0x7c0, 0x1 \n\t");
#endif
#endif
}

void dcache_disable(void)
{
#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	/* csr:0x7c0 can be accessed in MMODE only */
	asm volatile("csrci 0x7c0, 0x1 \n\t");
#endif
#endif
}

int dcache_status(void)
{
	int ret = 0;

#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
	/* 
	  if the I$ is enabled by configuration, 
	  set icache is enabled as default, it will be
	  updated with csr:0x7c0 if it can be accessed
	*/
	ret = 1;
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	asm volatile (
		"csrr t1, 0x7c0\n\t"
		"andi	%0, t1, 0x01\n\t"
		: "=r" (ret)
		:
		: "memory"
	);
#endif
#endif

	return ret;
}


void branch_predict_enable(void)
{

#if !CONFIG_IS_ENABLED(SYS_BRANCH_PREDICT_OFF)
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	/* csr:0x7c0 can be accessed in MMODE only */
	csr_set(0x7c0, 0x10);
#endif
#endif
}

void branch_predict_disable(void)
{
#if !CONFIG_IS_ENABLED(SYS_BRANCH_PREDICT_OFF)
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	/* csr:0x7c0 can be accessed in MMODE only */
	csr_clear(0x7c0, 0x10);
#endif
#endif
}

void prefetch_enable(void)
{

#if !CONFIG_IS_ENABLED(SYS_PREFETCH_OFF)
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	/* csr:0x7c0 can be accessed in MMODE only */
	csr_set(0x7c0, 0x20);
#endif
#endif
}

void prefetch_disable(void)
{
#if !CONFIG_IS_ENABLED(SYS_PREFETCH_OFF)
#if CONFIG_SPL_BUILD && CONFIG_SPL_RISCV_MMODE
	/* csr:0x7c0 can be accessed in MMODE only */
	csr_clear(0x7c0, 0x20);
#endif
#endif
}



int check_cache_range(unsigned long start, unsigned long end)
{
	int ok = 1;

	if (start & (CONFIG_RISCV_CBOM_BLOCK_SIZE - 1))
		ok = 0;

	if (end & (CONFIG_RISCV_CBOM_BLOCK_SIZE - 1))
		ok = 0;

	if (!ok) {
		warn_non_spl("CACHE: Misaligned operation at range [%08lx, %08lx]\n",
			start, end);
	}

	return ok;
}

void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	if (!check_cache_range(start, end))
		return;

	while (start < end) {
		cbo_invalid(start);
		start += CONFIG_RISCV_CBOM_BLOCK_SIZE;
	}
}

void flush_dcache_range(unsigned long start, unsigned long end)
{
	if (!check_cache_range(start, end))
		return;

	while (start < end) {
		cbo_flush(start);
		start += CONFIG_RISCV_CBOM_BLOCK_SIZE;
	}
}

void clean_dcache_range(unsigned long start, unsigned long end)
{
	if (!check_cache_range(start, end))
		return;

	while (start < end) {
		cbo_clean(start);
		start += CONFIG_RISCV_CBOM_BLOCK_SIZE;
	}
}
