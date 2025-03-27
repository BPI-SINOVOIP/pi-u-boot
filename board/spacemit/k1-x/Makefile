# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2022-2023 Spacemit, Inc

obj-y	+= k1x.o
obj-$(CONFIG_SPL_BUILD)	+= spl.o k1x-i2c-eeprom.o k1x-tlvinfo.o
obj-$(CONFIG_SPLASH_SOURCE) +=splash.o
obj-$(CONFIG_ENV_IS_IN_NFS) +=nfs_env.o
obj-$(CONFIG_SPACEMIT_SHUTDOWN_CHARGE) += show_battery.o
