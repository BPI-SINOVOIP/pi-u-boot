# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2021, Renesas Electronics Corporation. All rights reserved.

ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_TARGET_RZF_DEV)		:= ddr.o ddr_rzf.o
obj-$(CONFIG_TARGET_SMARC_RZF)		:= ddr.o ddr_rzf.o

obj-$(CONFIG_RZF_DDR_SWIZZLE_T1BC)	+= param_swizzle_T1bc.o
obj-$(CONFIG_RZF_DDR_SWIZZLE_T1C)	+= param_swizzle_T1c.o
obj-$(CONFIG_RZF_DDR_SWIZZLE_T2C)	+= param_swizzle_T2c.o
obj-$(CONFIG_RZF_DDR_SWIZZLE_T3BC)	+= param_swizzle_T3bc.o
obj-$(CONFIG_RZF_DDR_SWIZZLE_T3BCUD)	+= param_swizzle_T3bcud.o
obj-$(CONFIG_RZF_DDR_SWIZZLE_T3BCUD2)	+= param_swizzle_T3bcud2.o
obj-$(CONFIG_RZF_DDR_SWIZZLE_T3BCUL)	+= param_swizzle_T3bcul.o
obj-$(CONFIG_RZF_DDR_SWIZZLE_T3CL)	+= param_swizzle_T3cl.o

endif
