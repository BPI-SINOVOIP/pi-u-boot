#include <common.h>
#include <asm/io.h>

#include "Galois_memmap.h"
#include "cpu_wrp.h"

int timer_init(void)
{
	writel(1, (MEMMAP_CA7_REG_BASE + RA_CPU_WRP_TIMER_REG));
	return 0;
}

