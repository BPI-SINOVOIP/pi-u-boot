# SPDX-License-Identifier:	GPL-2.0

obj-y	+= init.o mem.o reset.o timer.o misc.o
obj-y	+= driver/ utils/
obj-$(CONFIG_SYNA_SM) += sm.o
obj-$(CONFIG_SYNA_INCLUDE_SM_FW) += sm_fw.o
obj-$(CONFIG_SYNA_DISPLAY_TA) += display_ta.o
obj-$(CONFIG_CMD_SYNA_FASTBOOT_GUI) += synaptics_logo_bmp.o

PLATFORM_CPPFLAGS += $(if $(CONFIG_SYNA_INCLUDE_SM_FW), -I$(KBUILD_OUTPUT)/../output_sm/bin)
