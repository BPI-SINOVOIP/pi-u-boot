/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */
#include <common.h>
#include <command.h>

enum image_src_type{
	SRC_TFTP = 1,
	SRC_USBHOST,
	SRC_USBSLAVE,
};

#if defined(CONFIG_USB_BOOT)
static int get_img_usbslave(char * const img_name, void * addr)
{
	int ret = -1;
	int image_size = 0;
	char cmd[512];

	sprintf(cmd, "usbload %s 0x%x", img_name, (uint32_t)(uint64_t)addr);
	printf("%s\n", cmd);

	ret = run_command(cmd, 0);
	if (ret)
		return -1;

	image_size = env_get_hex("filesize", 0);

	return image_size;
}
#else
static int get_img_usbslave(char * const img_name, void * addr)
{
	printf("don't support usb slave!\n");
	return -1;
}
#endif

#if defined(CONFIG_CMD_USB) && defined(CONFIG_CMD_FAT)
static int get_img_usbhost(char * const img_name, void * addr)
{
	int ret = -1, i = 0, j = 0;
	int image_size = 0;
	char cmd[512], usbpath[16];
	char * colon_ptr = NULL;
	char * p_img_name = img_name;

	ret = run_command("usb start", 0);
	if (ret)
		return ret;

	colon_ptr = strchr(p_img_name, ':');

	if(!colon_ptr)
		sprintf(cmd, "fatload usb 0 0x%x %s", (uint32_t)(uint64_t)addr, img_name);
	else {
		for(i = 0, p_img_name = img_name; i < 16; i++, p_img_name++) {
			if(*p_img_name == ':')
				j++;
			if(j == 2) {
				usbpath[i] = '\0';
				p_img_name++;
				break;
			}
			usbpath[i] = *p_img_name;
		}
		sprintf(cmd, "fatload usb %s 0x%x %s", usbpath, (uint32_t)(uint64_t)addr, p_img_name);
	}
	printf("%s\n", cmd);

	ret = run_command(cmd, 0);
	if (ret)
		return -1;

	image_size = env_get_hex("filesize", 0);

	return image_size;
}
#else
static int get_img_usbhost(char * const img_name, void * addr)
{
	printf("don't support usb slave!\n");
	return -1;
}
#endif

#if defined(CONFIG_NET)
static int get_img_tftp(char * const img_name, void * addr)
{
	int ret = -1, i = 0;
	int image_size = 0;
	char cmd[512], serverip[32];
	char * colon_ptr = NULL;
	char *p_img_name = img_name;

	colon_ptr = strchr(p_img_name, ':');

	if (!colon_ptr) {
		//suppose "serverip" already set
		//FIXME: check env "serverip", if no, return error
	} else {
		for(i = 0, p_img_name = img_name; i < 32; i++, p_img_name++) {
			if(*p_img_name == ':') {
				serverip[i] = '\0';
				break;
			}
			serverip[i] = *p_img_name;
		}
		sprintf(cmd, "set serverip %s\n", serverip);
		//set serverip to env
		run_command(cmd, 0);
	}

	sprintf(cmd, "tftpboot 0x%x %s", (uint32_t)(uint64_t)addr, img_name);

	ret = run_command(cmd, 0);
	if (ret)
		return -1;

	image_size = env_get_hex("filesize", 0);

	return image_size;
}
#else
static int get_img_tftp(char * const img_name, void * addr)
{
	printf("don't support tftp!\n");
	return -1;
}
#endif



int get_img(unsigned int src, char * const img_name, void * addr)
{
	int ret = -1;
	switch(src) {
		case SRC_TFTP:
			ret = get_img_tftp(img_name, addr);
			break;
		case SRC_USBHOST:
			ret = get_img_usbhost(img_name, addr);
			break;
		case SRC_USBSLAVE:
			ret = get_img_usbslave(img_name, addr);
			break;
		default:
			printf("invalide src! can't download image %s to 0x%p\n", img_name, addr);
			break;
	}

	return ret;
}

static int do_imgload(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	unsigned int src = 0;
	unsigned int load_addr;

	if(argc < 4) {
		return -1;
	}

	if(strcmp(argv[1], "tftp") == 0)
		src = SRC_TFTP;

	if(strcmp(argv[1], "usbh") == 0)
		src = SRC_USBHOST;

	if(strcmp(argv[1], "usbs") == 0)
		src = SRC_USBSLAVE;

	if(strlen(argv[2]) > 255) {
		printf("%s, file name %s is too long.\n", __func__, argv[1]);
		return -1;
	}

	load_addr = simple_strtoull(argv[3], NULL, 16);

	ret = get_img(src, argv[2], (void *)(uint64_t)load_addr);

	if(ret > 0)
		return 0;

	return ret;
}


U_BOOT_CMD(
	imgload, 4, 0, do_imgload,
	"load image through tftp, usb host or usb slave",
	" [src: tftp, usbh, usbs] [image path] [load address]\n"
	"example:\n"
	"    imgload tftp 10.70.24.110:emmc/bootloader.subimg 0x1000000\n"
	"    imgload usbh 0:1:emmc/bootloader.subimg 0x1000000\n"
	"    imgload usbs emmc/bootloader.subimg 0x1000000\n"
);
