/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2008 - 2009 Windriver, <www.windriver.com>
 * Author: Tom Rix <Tom.Rix@windriver.com>
 *
 * (C) Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * Copyright (C) 2019 Synaptics Incorporated
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 *
 */

#include <common.h>
#include <command.h>
#include <console.h>
#include <dm.h>
#include <g_dnl.h>
#include <usb.h>
#include <version.h>
#include <lcd.h>
#include <video.h>
#include <video_console.h>
#include <iomux.h>
#include <mmc.h>
//#include <adc_key.h>
//#include <asm/arch/fastboot.h>
#include <net.h>
//#include <fastboot-internal.h>
#include <memalign.h>
#include <fastboot.h>
#include "fastboot_syna.h"

extern u8 __gzip_synaptics_logo_bmp_begin[];
extern u8 __gzip_synaptics_logo_bmp_end[];

enum {
	ORANGE = 0x0361ff,
	GREEN = 0x00ff00,
	RED = 0x0000ff,
};

struct oem_lock_flag {
	unsigned int magic;
	unsigned int reserve1[6];
	unsigned int lock_flag;
	unsigned int lock_critical_flag;
	unsigned int reserve2[119];	/* round to 512 bytes */
};

struct frp_lock_flag {
	unsigned char reserve[510];
	unsigned char lock_critical_flag;
	unsigned char lock_flag;
};

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_GUI_ENABLE)
static int show_logo(void)
{
	void *bmp_start;
	struct bmp_image *bmp;
	unsigned long len;
	struct udevice *dev;
	bmp = gunzip_bmp((uintptr_t)__gzip_synaptics_logo_bmp_begin, &len, &bmp_start);

	if (uclass_first_device_err(UCLASS_VIDEO, &dev))
		return -1;

	video_bmp_display(dev, (uintptr_t)bmp, 105, 105, 1);
	return 0;
}

static int console_puts(const char *vc_name, char *s)
{
	struct udevice *vc;
	if (uclass_get_device_by_name(UCLASS_VIDEO_CONSOLE, vc_name, &vc)) {
		printf("Can't find device named %s.\n", vc_name);
		return -1;
	}
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(vc);
	struct stdio_dev *sdev = &vc_priv->sdev;
	sdev->puts(sdev, s);
	return 0;
}

static int console_colour_puts(const char *vc_name, char *s, int colour)
{
	struct udevice *vc;
	if (uclass_get_device_by_name(UCLASS_VIDEO_CONSOLE, vc_name, &vc)) {
		printf("Can't find device named %s.\n", vc_name);
		return -1;
	}
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(vc);
	int colour_bak = vc_priv->colour_fg;
	vc_priv->colour_fg = colour;
	struct stdio_dev *sdev = &vc_priv->sdev;
	sdev->puts(sdev, s);
	vc_priv->colour_fg = colour_bak;
	return 0;
}

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_GUI_BUTTON)

#define CMD_NUM 3

typedef struct {
	char discription[32];
	enum fastboot_reboot_command cmd;
} fastboot_gui_cmd;

fastboot_gui_cmd fastboot_gui_cmd_list[CMD_NUM] = {
	{"\nNormal boot", 		FB_REBOOT_CMD_REBOOT},
	{"\nBoot to recovery mode", 	FB_REBOOT_CMD_RECOVERY},
	{"\nBoot to fastboot mode", 	FB_REBOOT_CMD_REBOOT_FB}
};

static int cur_cmd_i = 0;

static int console_set_bg(const char *vc_name, int colour)
{
	struct udevice *vc;
	if (uclass_get_device_by_name(UCLASS_VIDEO_CONSOLE, vc_name, &vc)) {
		printf("Can't find device named %s.\n", vc_name);
		return -1;
	}
	struct vidconsole_priv *vc_priv = dev_get_uclass_priv(vc);
	vc_priv->colour_bg = colour;
	return 0;
}
#endif

void update_gui_lock_state(unsigned int lock_flag, unsigned int lock_critical_flag)
{
	if (lock_flag)
		console_puts("vidconsole-lock", "\n  LOCKED");
	else
		console_colour_puts("vidconsole-lock", "\n  UNLOCKED", RED);
#if 0
	if (lock_critical_flag)
		console_puts("vidconsole-lock", "\n  CRITICAL-LOCKED");
	else
		console_colour_puts("vidconsole-lock", "\n  CRITICAL-UNLOCKED", RED);
#endif
}

static int gui_init(void)
{

	if (show_logo() < 0) {
		printf("video device not find.\n");
		return -1;
	}

	console_colour_puts("vidconsole-info", "PRODUCT NAME\n  ", ORANGE);
	console_puts("vidconsole-info", env_get("product"));
	console_colour_puts("vidconsole-info", "\n\nFASTBOOT VERSION\n  ", ORANGE);
	console_puts("vidconsole-info", "0.4");
	console_colour_puts("vidconsole-info", "\n\nBOOTLOADER VERSION\n  ", ORANGE);
	console_puts("vidconsole-info", PLAIN_VERSION);
	console_colour_puts("vidconsole-info", "\n\nS/N\n  ", ORANGE);
	console_puts("vidconsole-info", env_get("serial#"));
	console_colour_puts("vidconsole-info", "\n\nDEVICE STATE", ORANGE);

	unsigned int lock_flag, lock_critical_flag;
	if (get_lock_flag(&lock_flag, &lock_critical_flag))
		return -1;
	update_gui_lock_state(lock_flag, lock_critical_flag);

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_GUI_BUTTON)
	console_colour_puts("vidconsole-usage", "\nQUICK PRESS hardware button", ORANGE);
	console_colour_puts("vidconsole-usage", "\nto change boot option", ORANGE);
	console_colour_puts("vidconsole-usage", "\nLONG PRESS hardware button", ORANGE);
	console_colour_puts("vidconsole-usage", "\nto select boot option", ORANGE);

	console_set_bg("vidconsole-sel", 0x111111);
	console_puts("vidconsole-sel", fastboot_gui_cmd_list[cur_cmd_i].discription);
#endif

	if (iomux_doenv(stdout, "serial,vidconsole")) {
		printf("do iomux failed.\n");
		return -1;
	}

	printf("Fastboot Version: 0.4\n");
	printf("U-Boot Version: "U_BOOT_VERSION_STRING"\n");

	return 0;
}
#endif

static int gui_close(void)
{
	if (iomux_doenv(stdout, "serial")) {
		printf("do iomux failed.\n");
		return -1;
	}

	return 0;
}

static int do_fastboot_udp(int argc, char *const argv[],
			   uintptr_t buf_addr, size_t buf_size)
{
#if CONFIG_IS_ENABLED(UDP_FUNCTION_FASTBOOT)
	if (run_command("dhcp", 0)){
		char *tmp = "10.70.52.123";
		env_set("ipaddr", tmp);
		net_ip = string_to_ip(tmp);
	}

	printf("enter fastboot mode\n");
	clear_bootloader_message_to_misc();

	int err = net_loop(FASTBOOT);
	if (err < 0) {
		printf("fastboot udp error: %d\n", err);
		printf("exit fastboot mode\n");
		do_reset(NULL, 0, 0, NULL);
		gui_close();
		return CMD_RET_FAILURE;
	}

	do_reset(NULL, 0, 0, NULL);
	return CMD_RET_SUCCESS;
#else
	pr_err("Fastboot UDP not enabled\n");
	do_reset(NULL, 0, 0, NULL);
	return CMD_RET_FAILURE;
#endif
}

static int do_fastboot_usb(int argc, char *const argv[],
			   uintptr_t buf_addr, size_t buf_size)
{
#if CONFIG_IS_ENABLED(USB_FUNCTION_FASTBOOT)
	int controller_index;
	char *usb_controller;
	char *endp;
	int ret;

	if (argc < 2) {
		do_reset(NULL, 0, 0, NULL);
		return CMD_RET_USAGE;
	}

	usb_controller = argv[1];
	controller_index = simple_strtoul(usb_controller, &endp, 0);
	if (*endp != '\0') {
		pr_err("Error: Wrong USB controller index format\n");
		do_reset(NULL, 0, 0, NULL);
		return CMD_RET_FAILURE;
	}

	ret = usb_gadget_initialize(controller_index);
	if (ret) {
		pr_err("USB init failed: %d\n", ret);
		do_reset(NULL, 0, 0, NULL);
		return CMD_RET_FAILURE;
	}

	g_dnl_clear_detach();
	ret = g_dnl_register("usb_dnl_fastboot");
	if (ret) {
		do_reset(NULL, 0, 0, NULL);
		return ret;
	}

	if (!g_dnl_board_usb_cable_connected()) {
		puts("\rUSB cable not detected.\n" \
		     "Command exit.\n");
		do_reset(NULL, 0, 0, NULL);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	printf("enter fastboot mode\n");
	clear_bootloader_message_to_misc();

	while (1) {
		if (g_dnl_detach())
			break;
#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_GUI_BUTTON)
		switch (check_adc_key("smadc_key")) {
		case SHORT_PRESS:
			if (++cur_cmd_i == CMD_NUM)
				cur_cmd_i = 0;
			console_puts("vidconsole-sel", fastboot_gui_cmd_list[cur_cmd_i].discription);
			break;
		case LONG_PRESS:
			berlin_reboot(fastboot_gui_cmd_list[cur_cmd_i].cmd, 0, 0);
			break;
		default:
			break;
		}
#endif
		usb_gadget_handle_interrupts(controller_index);
	}

	do_reset(NULL, 0, 0, NULL);
	ret = CMD_RET_SUCCESS;

exit:
	printf("exit fastboot mode\n");
	gui_close();
	g_dnl_unregister();
	g_dnl_clear_detach();
	usb_gadget_release(controller_index);

	return ret;
#else
	pr_err("Fastboot USB not enabled\n");
	do_reset(NULL, 0, 0, NULL);
	return CMD_RET_FAILURE;
#endif
}

/* test if ctrl-d was pressed */
int ctrld(void)
{
	if (tstc()) {
		switch (getc()) {
		case 0x04:		/* ^D - Control D */
			return 1;
		default:
			break;
		}
	}

	return 0;
}

extern unsigned int get_max_malloc_size(void);
static int do_fastboot_gui(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	uintptr_t buf_size = get_max_malloc_size();
	void * buff = malloc_cache_aligned(buf_size);
	size_t buf_addr = (size_t)(unsigned long)buff;

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_GUI_ENABLE)
#if CONFIG_IS_ENABLED(SYNA_DISPLAY_TA)
	extern int reg_preload_tas(void);
	int ret = reg_preload_tas();
	if(ret) {
		printf("register ta failed: %d", ret);
		do_reset(NULL, 0, 0, NULL);
		return CMD_RET_FAILURE;
	}
#endif

	if (gui_init()) {
		do_reset(NULL, 0, 0, NULL);
		return CMD_RET_FAILURE;
	}
#endif

	if (argc < 2) {
		do_reset(NULL, 0, 0, NULL);
		return CMD_RET_USAGE;
	}

	while (argc > 1 && **(argv + 1) == '-') {
		char *arg = *++argv;

		--argc;
		while (*++arg) {
			switch (*arg) {
			case 'l':
				if (--argc <= 0) {
					do_reset(NULL, 0, 0, NULL);
					return CMD_RET_USAGE;
				}
				buf_addr = simple_strtoul(*++argv, NULL, 16);
				goto NXTARG;

			case 's':
				if (--argc <= 0) {
					do_reset(NULL, 0, 0, NULL);
					return CMD_RET_USAGE;
				}
				buf_size = simple_strtoul(*++argv, NULL, 16);
				goto NXTARG;

			default:
				do_reset(NULL, 0, 0, NULL);
				return CMD_RET_USAGE;
			}
		}
NXTARG:
		;
	}

	/* Handle case when USB controller param is just '-' */
	if (argc == 1) {
		pr_err("Error: Incorrect USB controller index\n");
		do_reset(NULL, 0, 0, NULL);
		return CMD_RET_USAGE;
	}

	fastboot_init((void *)buf_addr, buf_size);

	if (ctrld() || !strcmp(argv[1], "udp"))
		return do_fastboot_udp(argc, argv, buf_addr, buf_size);

	if (!strcmp(argv[1], "usb")) {
		argv++;
		argc--;
	}

	return do_fastboot_usb(argc, argv, buf_addr, buf_size);
}

static char fastboot_help_text[] =
	"[-l addr] [-s size] usb <controller> | udp\n"
	"\taddr - address of buffer used during data transfers ("
	__stringify(CONFIG_FASTBOOT_BUF_ADDR) ")\n"
	"\tsize - size of buffer used during data transfers ("
	__stringify(CONFIG_FASTBOOT_BUF_SIZE) ")"
	;

U_BOOT_CMD(
	fastboot_gui, CONFIG_SYS_MAXARGS, 1, do_fastboot_gui,
	"run as a fastboot usb or udp device", fastboot_help_text
);
