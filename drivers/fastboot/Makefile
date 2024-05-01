# SPDX-License-Identifier: GPL-2.0+

obj-y += fb_common.o
obj-y += fb_getvar.o
obj-y += fb_command.o
obj-y += fb_spacemit.o
obj-$(CONFIG_$(SPL_)FASTBOOT_FLASH_MMC) += fb_mmc.o
obj-$(CONFIG_$(SPL_)FASTBOOT_FLASH_NAND) += fb_nand.o
obj-$(CONFIG_$(SPL_)FASTBOOT_FLASH_MTD) += fb_mtd.o

obj-$(CONFIG_$(SPL_)FASTBOOT_MULTI_FLASH_OPTION_MMC) += fb_mmc.o
obj-$(CONFIG_$(SPL_)FASTBOOT_MULTI_FLASH_OPTION_MTD) += fb_mtd.o
obj-$(CONFIG_$(SPL_)FASTBOOT_SUPPORT_BLOCK_DEV) += fb_blk.o
