/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Jie Zhang <Jie.Zhang@synaptics.com>
 */

#include <common.h>
#include <command.h>
#include "bm_generic.h"

//static unsigned char name_buffer[128] __attribute__((aligned(4)));
//be overwritten by next loading command.
static unsigned int img_size = 0;
static int do_bcm_disable_usb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int cmdid = BM_CMD_TURN_OFF_USB_DEVICE;
	unsigned ret = call_bm_process(BM_STDCALL(cmdid), 0, 0, 0, 0, 0, 0);
	return ret;
}

static uint32_t request_img_from_bm_by_name(char *image_name, uint32_t uboot_img_load_addr)
{
	uint32_t cmdid = BM_CMD_LOAD_USB_IMAGE;
	uint32_t arg0, arg1, arg2, arg3;
	arg0 = 0xff;
	arg1 = uboot_img_load_addr;
	arg2 = (uint32_t)(uint64_t)&image_name[0];
	arg3 = strlen(image_name) + 1;
	debug("---debug---, %d, arg0=0x%x, arg1=0x%x, arg2=0x%x, arg3=0x%x\n",
		__LINE__, arg0, arg1, arg2, arg3);
	unsigned ret = call_bm_process(BM_STDCALL(cmdid), arg0, arg1, arg2, arg3, 0, 0);
	return ret;
}

static int do_usbload(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned load_addr, ret;
	unsigned char buff[1024];
	char image_name[256] = "07_IMAGE";
	uint64_t buff_addr = 0;
	#define BM_LOAD_RERTY_INITIAL	3
	unsigned int retry_bm = BM_LOAD_RERTY_INITIAL;

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
	ret = request_img_from_bm_by_name(argv[1], load_addr);
	if (ret != 0){
		printf("can not load image from usb.\n");
		return ret;
	}

	ret = request_img_from_bm_by_name(image_name, (uint32_t)(uint64_t)buff);
	if (ret != 0){
		printf("can not get image size from usb.\n");
		return ret;
	}

	buff_addr = (uint64_t)buff;
	img_size = *(unsigned int *)buff_addr;
	while(!img_size && retry_bm) {
		mdelay(1);
		ret = request_img_from_bm_by_name(image_name, (uint32_t)(uint64_t)buff);
		if (ret != 0){
			printf("can not get image size from usb.\n");
			return ret;
		}
		buff_addr = (uint64_t)buff;
		img_size = *(unsigned int *)buff_addr;

		retry_bm--;
	}
	printf("usbload done, get size %d by retry %d.\n", img_size, (BM_LOAD_RERTY_INITIAL - retry_bm));

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
