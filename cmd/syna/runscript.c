/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */
 #include <common.h>
#include <command.h>
#include <linux/ctype.h>
#include <memalign.h>

#define MAX_SIZE_SCRIPT (16 << 10)   // 16KB

static int execute_cmd_from_script(void * buff, unsigned int len)
{
	char cmd[512];
	unsigned int i = 0;
	int cmd_len = 0, rc = -1;
	char *p = buff;

	do {
		if((p[i] == '\n') || (p[i] == '\r') || (p[i] == '\0') || (p[i] == ';')) {
			goto runcommand;
		} else if(!isprint(p[i])) {
			// just ignore? or return error?
			printf("non-printable character %x.\n", p[i]);
			goto next;
		} else {
			cmd[cmd_len] = p[i];
			cmd_len++;
			goto next;
		}

runcommand:
		if(cmd_len > 0) {
			cmd[cmd_len] = '\0';
			if(cmd[0] != '#') {
				rc = run_command (cmd, 0);
				if (rc < 0) {
					printf("run %s ret %d\n",cmd, rc);
					return -1;
				}
				printf("run command: %s done\n", cmd);
				memset(cmd, 0, 512);
				cmd_len = 0;
			} else {
				if(strncmp(cmd, "#skip", 5) == 0)
					return 0;
			}
		}

next:
		i++;
		if(i == len)
			goto runcommand;
	} while(i < len);

	return 0;
}

static int do_runscript(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	char cmd[512];
	void * buff = NULL;
	unsigned int script_size = 0;

	if(argc < 3) {
		return -1;
	}

	buff = malloc_cache_aligned(MAX_SIZE_SCRIPT);

	memset(buff, 0, MAX_SIZE_SCRIPT);

	sprintf(cmd, "imgload %s %s 0x%p", argv[1], argv[2], buff);

	ret = run_command(cmd, 0);

	if(ret) {
		free(buff);
		return -1;
	}

	script_size = env_get_hex("filesize", 0);
	if(script_size > MAX_SIZE_SCRIPT) {
		printf("too big script(size = 0x%x)\n", script_size);
		return -1;
	}

	if(0 == execute_cmd_from_script(buff, script_size))
		printf("run script %s done!\n", argv[2]);
	else
		printf("fail to run script %s!\n", argv[2]);

	free(buff);
	return 0;
}

U_BOOT_CMD(
	runscript, 3, 0, do_runscript,
	"run command lists from a script",
	"runscript [src: tftp, usbh, usbs] [script path]\n"
	"- example:\n"
	"-   runscript tftp 10.70.24.110:u-boot-auto-cmd.txt\n"
	"-   runscript usbh 0:1:u-boot-auto-cmd.txt\n"
	"-   runscript usbs u-boot-auto-cmd.txt\n"
);
