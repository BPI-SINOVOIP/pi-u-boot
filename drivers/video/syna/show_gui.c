/*
 * Copyright 2008 - 2009 Windriver, <www.windriver.com>
 * Author: Tom Rix <Tom.Rix@windriver.com>
 *
 * (C) Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <g_dnl.h>
#include "io.h"

#include "show_gui.h"

static void show_gui_frame(char* frame_text);

static int do_show_gui(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = 0;
	char *frame_text = NULL;
	unsigned int i = 0, intr_counter = 0, instat = 0;
	char str[5][20] = {"Implementation", "Subscription", "Validate Items", "Random Update UI", "Test procedure"};

	printf("\033[32mEnter Show UI mode\033[00m\n");

	ret = MV_VPP_Init();
	if(ret != 0)
	{
		printf("MV_VPP_Init failed\n");
		return CMD_RET_FAILURE;
	}

	gr_init(1280, 720);

	frame_text = argv[1];
	if (frame_text != NULL) {
		memcpy(frame_text, argv[1], strlen(argv[1])+1);
		printf("Show GUI frame text = %s\n", frame_text);
		show_gui_frame(frame_text);
	} else {
		printf("Show checker bar Patten frame\n");
		MV_VPP_DisplayPatternFrame(0, 0, 1280, 720, 5120);
	}
	MV_VPP_Enable_IRQ();

	do{
		intr_counter = getFrmCount();
		if((intr_counter % 99) == 0)
		{
			i = i % 5;
			show_gui_frame(str[i]);
			++i;
		}
		if (ctrlc())
			break;
	}while(1);

	printf("Waiting for Ctrl+C\n");
	while (1) {
		if (ctrlc())
			break;
	}

	MV_VPP_Deinit();

	return CMD_RET_SUCCESS;
}

static void show_gui_frame(char* frame_text)
{
	unsigned int start_x, start_y;
	int r = 0, g = 0, b = 0;

	start_x = (gr_fb_width() - 650)/2;
	start_y = (gr_fb_height() - 400)/2;
	gr_buffer_init();
	gr_color(0, 0, 0, 255);
	gr_clear();
	gr_color(10, 10, 10, 255);
	gr_fill(start_x, start_y, start_x+650, start_y+400);

	r=255;b=255;g=0;
	start_x+=25;
	start_y+=25;
	gr_color(g, b, r, 255);
	gr_text(start_x, start_y, frame_text, 1);
	gr_flip();
}

U_BOOT_CMD(
	show_gui,	2,	1,	do_show_gui,
	"show_gui <frame_type> - Shows GUI frame",
	""
);
