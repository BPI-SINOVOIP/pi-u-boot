// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <asm/global_data.h>
#include <mtd.h>
#include <linux/err.h>
#include <env.h>

#include <g_dnl.h>
#include <fastboot.h>
#include <net.h>
#include <usb.h>
#include <watchdog.h>
#include <linux/stringify.h>
#include <vsprintf.h>

static ulong spl_fastboot_load_read(struct spl_load_info *load, ulong sector,
			       ulong count, void *buf)
{
	ulong addr;

	pr_debug("%s: sector %lx, count %lx, buf %lx\n",
	      __func__, sector, count, (ulong)buf);

	addr = (ulong)CONFIG_SPL_LOAD_FIT_ADDRESS + sector;
	if (CONFIG_IS_ENABLED(IMAGE_PRE_LOAD))
		addr += image_load_offset;

	memcpy(buf, (void *)addr, count);

	return count;
}



static int run_fastboot_usb(void)
{
	int controller_index = 0;
	int ret;

	fastboot_init((void *)CONFIG_SPL_LOAD_FIT_ADDRESS, CONFIG_FASTBOOT_BUF_SIZE);
	ret = usb_gadget_initialize(controller_index);
	if (ret) {
		pr_err("USB init failed: %d\n", ret);
		return -1;
	}

	g_dnl_clear_detach();
	ret = g_dnl_register("usb_dnl_fastboot");
	if (ret)
		return ret;

	if (!g_dnl_board_usb_cable_connected()) {
		puts("\rUSB cable not detected.\n" \
		     "Command exit.\n");
		ret = -1;
		goto exit;
	}

	while (1) {
		if (g_dnl_detach())
			break;
		if (ctrlc())
			break;
		WATCHDOG_RESET();
		usb_gadget_handle_interrupts(controller_index);
	}

	ret = 0;

exit:
	g_dnl_unregister();
	g_dnl_clear_detach();
	usb_gadget_release(controller_index);

	return ret;
}


static int spl_fastboot_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	struct image_header *header;

	if (run_fastboot_usb()){
		pr_err("run fastboot fail\n");
		return -1;
	}

	header = (struct image_header *)CONFIG_SPL_LOAD_FIT_ADDRESS;
	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		pr_debug("Found FIT\n");
		load.bl_len = 1;
		load.read = spl_fastboot_load_read;
		spl_load_simple_fit(spl_image, &load, 0, header);
	}else{
		pr_debug("not support legacy image\n");
		return -1;
	}
	return 0;
}

SPL_LOAD_IMAGE_METHOD("FASTBOOT", 0, BOOT_DEVICE_BOARD, spl_fastboot_load_image);
