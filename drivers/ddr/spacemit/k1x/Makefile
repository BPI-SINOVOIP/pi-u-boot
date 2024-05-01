# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2023 Spacemit
#

ifdef CONFIG_SPL_BUILD
obj-y += ddr_init.o lpddr4_silicon_init.o ddr_freq.o
else
obj-$(CONFIG_DYNAMIC_DDR_CLK_FREQ) += ddr_freq.o
endif
