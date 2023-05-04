/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 */
#include <common.h>
#include <command.h>
#include "bcm_mailbox.h"

#define EROM_USB_LOAD_IMAGE  0x0006
#define EROM_USB_TURN_OFF  0x0007

//for function run_command only return 0 or 1, we have to define
//this variable to record the image size loaded this time. it will
//be overwritten by next loading command.
static unsigned int img_size = 0;
static int do_bcm_disable_usb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	BCM_MAILBOX_COMMAND cmd;
	bcm_clear_command(&cmd);
	cmd.command = EROM_USB_TURN_OFF;
	bcm_usb_boot_func(&cmd);
	return 0;
}

static unsigned request_img_from_bcm_by_name(char *image_name, unsigned uboot_img_load_addr)
{
	BCM_MAILBOX_COMMAND cmd;
	bcm_clear_command(&cmd);
	cmd.arg0 = 0xff;
	cmd.arg1 = uboot_img_load_addr;
	cmd.arg2 = (uint32_t)(uint64_t)&image_name[0];
	cmd.arg3 = strlen(image_name) + 1;
	cmd.command = EROM_USB_LOAD_IMAGE;
	int ret = bcm_usb_boot_func(&cmd);
	return ret;
}

static int do_usbload(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned load_addr, ret;
	unsigned char buff[1024];
	uint64_t buff_addr = 0;
	#define BCM_LOAD_RERTY_INITIAL	3
	unsigned int retry_bcm = BCM_LOAD_RERTY_INITIAL;

	if (argc < 3) {
		return -1;
	}

	if (strlen(argv[1]) > 255) {
		printf("%s, file name %s is too long.\n", __func__, argv[1]);
		return -1;
	}
	load_addr = simple_strtoull(argv[2], NULL, 16);
	flush_dcache_all();
	printf("%s, loading image %s to addr %x.\n", __func__, argv[1], load_addr);
	ret = request_img_from_bcm_by_name(argv[1] , load_addr);
        if (ret != 0){
                printf("can not load image from usb.\n");
                return ret;
        }

	ret = request_img_from_bcm_by_name("07_IMAGE", (uint32_t)(uint64_t)buff);
        if (ret != 0){
                printf("can not get image size from usb.\n");
                return ret;
        }
	buff_addr = (uint64_t)buff;
	img_size = *(unsigned int *)buff_addr;
	while(!img_size && retry_bcm) {
		mdelay(1);
		ret = request_img_from_bcm_by_name("07_IMAGE", (uint32_t)(uint64_t)buff);
		if (ret != 0){
			printf("can not get image size from usb.\n");
			return ret;
		}
		buff_addr = (uint64_t)buff;
		img_size = *(unsigned int *)buff_addr;

		retry_bcm--;
	}
	printf("usbload done, get size %d by retry %d.\n", img_size, (BCM_LOAD_RERTY_INITIAL - retry_bcm));

	// we can't get any parameter from the return of command,
	// we have to set the needed parameters to env
	env_set_hex("fileaddr", load_addr);
	env_set_hex("filesize", img_size);

	return 0;
}

static char usbload_help[] =\
"Download image from host over USB connection.";

static char usbload_usage[] =\
"\nexamples:\n"
"    usbload uNAND_full.img 0x7000000 to load image $(IMG_DIR_IN_HOST)/uNAND_full.img to addr 0x7000000\n"
;

U_BOOT_CMD(usbload, 3, 0, do_usbload, usbload_help, usbload_usage);

U_BOOT_CMD(
	bcmusboff, 1, 0, do_bcm_disable_usb,
	"bcm disable usb device",
	"- bcm disable usb device"
);
