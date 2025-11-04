// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2021, Renesas Electronics Corporation. All rights reserved.
 */

#include <common.h>
#include <renesas/rzf-dev/rzf-dev_def.h>
#include <renesas/rzf-dev/rzf-dev_sys_regs.h>
#include "ddr_internal.h"

void ddr_ctrl_reten_en_n(uint8_t val)
{
	val &= 1;
		write_phy_reg(DDRPHY_R79, (val << 1));
}

char *ddr_get_version(void)
{
	return (((mmio_read_32(SYS_LSI_DEVID) >> 28) + 1 > 1) ? AN_VERSION : AN_VERSION_0);
}
