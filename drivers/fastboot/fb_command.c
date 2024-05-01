// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2016 The Android Open Source Project
 */

#include <common.h>
#include <command.h>
#include <env.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <fb_mmc.h>
#include <fb_nand.h>
#include <fb_mtd.h>
#include <part.h>
#include <stdlib.h>
#include <spl.h>
#include <image.h>
#include <fb_spacemit.h>
#include <fb_mtd.h>
#include <fb_blk.h>
#include <dm.h>

/**
 * image_size - final fastboot image size
 */
static u32 image_size;

/**
 * fastboot_bytes_received - number of bytes received in the current download
 */
static u32 fastboot_bytes_received;

/**
 * fastboot_bytes_expected - number of bytes expected in the current download
 */
static u32 fastboot_bytes_expected;

static void okay(char *, char *);
static void getvar(char *, char *);
static void download(char *, char *);

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void flash(char *, char *);
static void erase(char *, char *);
#endif

#if !defined(CONFIG_SPL_BUILD)
static void upload(char *, char *);
static void reboot_bootloader(char *, char *);
static void reboot_fastbootd(char *, char *);
static void reboot_recovery(char *, char *);
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_FORMAT)
static void oem_format(char *, char *);
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_PARTCONF)
static void oem_partconf(char *, char *);
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_BOOTBUS)
static void oem_bootbus(char *, char *);
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_READ)
static void oem_read(char *cmd_parameter, char *response);
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_CONFIG_ACCESS)
static void oem_config(char *cmd_parameter, char *response);
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_ENV_ACCESS)
static void oem_env(char *cmd_parameter, char *response);
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_UUU_SUPPORT)
static void run_ucmd(char *, char *);
static void run_acmd(char *, char *);
#endif


static const struct {
	const char *command;
	void (*dispatch)(char *cmd_parameter, char *response);
} commands[FASTBOOT_COMMAND_COUNT] = {
	[FASTBOOT_COMMAND_GETVAR] = {
		.command = "getvar",
		.dispatch = getvar
	},
	[FASTBOOT_COMMAND_DOWNLOAD] = {
		.command = "download",
		.dispatch = download
	},
#if !defined(CONFIG_SPL_BUILD)
	[FASTBOOT_COMMAND_UPLOAD] = {
		.command = "upload",
		.dispatch = upload
	},
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
	[FASTBOOT_COMMAND_FLASH] =  {
		.command = "flash",
		.dispatch = flash
	},
	[FASTBOOT_COMMAND_ERASE] =  {
		.command = "erase",
		.dispatch = erase
	},
#endif
	[FASTBOOT_COMMAND_BOOT] =  {
		.command = "boot",
		.dispatch = okay
	},
#endif /*!defined(CONFIG_SPL_BUILD)*/
	[FASTBOOT_COMMAND_CONTINUE] =  {
		.command = "continue",
		.dispatch = okay
	},
#if !defined(CONFIG_SPL_BUILD)
	[FASTBOOT_COMMAND_REBOOT] =  {
		.command = "reboot",
		.dispatch = okay
	},
	[FASTBOOT_COMMAND_REBOOT_BOOTLOADER] =  {
		.command = "reboot-bootloader",
		.dispatch = reboot_bootloader
	},
	[FASTBOOT_COMMAND_REBOOT_FASTBOOTD] =  {
		.command = "reboot-fastboot",
		.dispatch = reboot_fastbootd
	},
	[FASTBOOT_COMMAND_REBOOT_RECOVERY] =  {
		.command = "reboot-recovery",
		.dispatch = reboot_recovery
	},
	[FASTBOOT_COMMAND_SET_ACTIVE] =  {
		.command = "set_active",
		.dispatch = okay
	},
#endif /*!defined(CONFIG_SPL_BUILD)*/
#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_FORMAT)
	[FASTBOOT_COMMAND_OEM_FORMAT] = {
		.command = "oem format",
		.dispatch = oem_format,
	},
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_PARTCONF)
	[FASTBOOT_COMMAND_OEM_PARTCONF] = {
		.command = "oem partconf",
		.dispatch = oem_partconf,
	},
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_BOOTBUS)
	[FASTBOOT_COMMAND_OEM_BOOTBUS] = {
		.command = "oem bootbus",
		.dispatch = oem_bootbus,
	},
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_READ)
	[FASTBOOT_COMMAND_OEM_READ] = {
		.command = "oem read",
		.dispatch = oem_read,
	},
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_CONFIG_ACCESS)
	[FASTBOOT_COMMAND_CONFIG_ACCESS] = {
		.command = "oem config",
		.dispatch = oem_config,
	},
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_ENV_ACCESS)
	[FASTBOOT_COMMAND_ENV_ACCESS] = {
		.command = "oem env",
		.dispatch = oem_env,
	},
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_UUU_SUPPORT)
	[FASTBOOT_COMMAND_UCMD] = {
		.command = "UCmd",
		.dispatch = run_ucmd,
	},
	[FASTBOOT_COMMAND_ACMD] = {
		.command = "ACmd",
		.dispatch = run_acmd,
	},
#endif
};

/**
 * fastboot_handle_command - Handle fastboot command
 *
 * @cmd_string: Pointer to command string
 * @response: Pointer to fastboot response buffer
 *
 * Return: Executed command, or -1 if not recognized
 */
int fastboot_handle_command(char *cmd_string, char *response)
{
	int i;
	char *cmd_parameter;

	cmd_parameter = cmd_string;
	strsep(&cmd_parameter, ":");

	for (i = 0; i < FASTBOOT_COMMAND_COUNT; i++) {
		if (!strcmp(commands[i].command, cmd_string)) {
			if (commands[i].dispatch) {
				commands[i].dispatch(cmd_parameter,
							response);
				return i;
			} else {
				break;
			}
		}
	}

	pr_err("command %s not recognized.\n", cmd_string);
	fastboot_fail("unrecognized command", response);
	return -1;
}

/**
 * okay() - Send bare OKAY response
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 *
 * Send a bare OKAY fastboot response. This is used where the command is
 * valid, but all the work is done after the response has been sent (e.g.
 * boot, reboot etc.)
 */
static void okay(char *cmd_parameter, char *response)
{
	fastboot_okay(NULL, response);
}

/**
 * getvar() - Read a config/version variable
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void getvar(char *cmd_parameter, char *response)
{
	fastboot_getvar(cmd_parameter, response);
}

/**
 * fastboot_download() - Start a download transfer from the client
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void download(char *cmd_parameter, char *response)
{
	char *tmp;

	if (!cmd_parameter) {
		fastboot_fail("Expected command parameter", response);
		return;
	}
	fastboot_bytes_received = 0;
	fastboot_bytes_expected = hextoul(cmd_parameter, &tmp);
	if (fastboot_bytes_expected == 0) {
		fastboot_fail("Expected nonzero image size", response);
		return;
	}
	/*
	 * Nothing to download yet. Response is of the form:
	 * [DATA|FAIL]$cmd_parameter
	 *
	 * where cmd_parameter is an 8 digit hexadecimal number
	 */
	if (fastboot_bytes_expected > fastboot_buf_size) {
		fastboot_fail(cmd_parameter, response);
	} else {
		pr_info("Starting download of %d bytes\n",
		       fastboot_bytes_expected);
		fastboot_response("DATA", response, "%s", cmd_parameter);
	}
}

#if !defined(CONFIG_SPL_BUILD)
/**
 * fastboot_upload() - Start a upload transfer from the host
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void upload(char *cmd_parameter, char *response)
{
	/*fastboot_bytes_received would record had send byte*/
	fastboot_bytes_received = 0;

	if (fastboot_bytes_expected == 0) {
		fastboot_fail("Expected nonzero image size", response);
		return;
	}
	/*
	 * Nothing to upload yet. Response is of the form:
	 * [DATA|FAIL]$cmd_parameter
	 *
	 * where cmd_parameter is an 8 digit hexadecimal number
	 */
	if (fastboot_bytes_expected > fastboot_buf_size) {
		fastboot_fail(cmd_parameter, response);
	} else {
		pr_info("Starting upload of %d bytes\n",
		       fastboot_bytes_expected);
		fastboot_response("PUSH", response, "%08x", fastboot_bytes_expected);
	}
}
#endif

/**
 * fastboot_data_remaining() - return bytes remaining in current transfer
 *
 * Return: Number of bytes left in the current download
 */
u32 fastboot_data_remaining(void)
{
	return fastboot_bytes_expected - fastboot_bytes_received;
}

/**
 * fastboot_data_download() - Copy image data to fastboot_buf_addr.
 *
 * @fastboot_data: Pointer to received fastboot data
 * @fastboot_data_len: Length of received fastboot data
 * @response: Pointer to fastboot response buffer
 *
 * Copies image data from fastboot_data to fastboot_buf_addr. Writes to
 * response. fastboot_bytes_received is updated to indicate the number
 * of bytes that have been transferred.
 *
 * On completion sets image_size and ${filesize} to the total size of the
 * downloaded image.
 */
void fastboot_data_download(const void *fastboot_data,
			    unsigned int fastboot_data_len,
			    char *response)
{
#define BYTES_PER_DOT	0x20000
	u32 pre_dot_num, now_dot_num;

	if (fastboot_data_len == 0 ||
	    (fastboot_bytes_received + fastboot_data_len) >
	    fastboot_bytes_expected) {
		fastboot_fail("Received invalid data length",
			      response);
		return;
	}
	/* Download data to fastboot_buf_addr */
	memcpy(fastboot_buf_addr + fastboot_bytes_received,
	       fastboot_data, fastboot_data_len);

	pre_dot_num = fastboot_bytes_received / BYTES_PER_DOT;
	fastboot_bytes_received += fastboot_data_len;
	now_dot_num = fastboot_bytes_received / BYTES_PER_DOT;

	if (pre_dot_num != now_dot_num) {
		putc('.');
		if (!(now_dot_num % 74))
			putc('\n');
	}
	*response = '\0';
}


/**
 * fastboot_data_upload() - Copy image data to fastboot_buf_addr.
 *
 * @fastboot_data: Pointer to received fastboot data
 * @fastboot_data_len: Length of received fastboot data
 * @response: Pointer to fastboot response buffer
 *
 * Copies image data from fastboot_buf_addr to fastboot_data. Writes to
 * response. fastboot_bytes_received is updated to indicate the number
 * of bytes that have been transferred.
 */
void fastboot_data_upload(const void *fastboot_data,
			    unsigned int fastboot_data_len,
			    char *response)
{
#define BYTES_PER_DOT	0x20000
	u32 pre_dot_num, now_dot_num;

	if (fastboot_data_len == 0 ||
	    (fastboot_bytes_received + fastboot_data_len) >
	    fastboot_bytes_expected) {
		fastboot_fail("Received invalid data length",
			      response);
		return;
	}

	/* copy data to buffer */
	memcpy((void *)fastboot_data,
	       fastboot_buf_addr + fastboot_bytes_received, fastboot_data_len);

	pre_dot_num = fastboot_bytes_received / BYTES_PER_DOT;
	fastboot_bytes_received += fastboot_data_len;
	now_dot_num = fastboot_bytes_received / BYTES_PER_DOT;

	if (pre_dot_num != now_dot_num) {
		putc('.');
		if (!(now_dot_num % 74))
			putc('\n');
	}
	*response = '\0';
}


/**
 * fastboot_data_complete() - Mark current transfer complete
 *
 * @response: Pointer to fastboot response buffer
 *
 * Set image_size and ${filesize} to the total size of the downloaded image.
 */
void fastboot_data_complete(char *response)
{
	/* Download complete. Respond with "OKAY" */
	fastboot_okay(NULL, response);
	pr_info("\ndownloading/uploading of %d bytes finished\n", fastboot_bytes_received);
	image_size = fastboot_bytes_received;
	env_set_hex("filesize", image_size);
	fastboot_bytes_expected = 0;
	fastboot_bytes_received = 0;
}

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
/**
 * flash() - write the downloaded image to the indicated partition.
 *
 * @cmd_parameter: Pointer to partition name
 * @response: Pointer to fastboot response buffer
 *
 * Writes the previously downloaded image to the partition indicated by
 * cmd_parameter. Writes to response.
 */
static void flash(char *cmd_parameter, char *response)
{
	u32 boot_mode = get_boot_pin_select();

	switch(boot_mode){
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MTD) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MTD)
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
		static bool mtd_flash = false;
		if (!strncmp("mtd", cmd_parameter, 3))
			mtd_flash = true;
		if (!strncmp("gpt", cmd_parameter, 3))
			mtd_flash = false;

		if (mtd_flash){
			fastboot_mtd_flash_write(cmd_parameter, fastboot_buf_addr, image_size,
						response);
		}else{
			/* flash blk dev */
			fastboot_blk_flash_write(cmd_parameter, fastboot_buf_addr, image_size, response);
		}

		return;
#endif
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
		fastboot_mmc_flash_write(cmd_parameter, fastboot_buf_addr, image_size,
					response);
		return;
#endif
	}

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_NAND)
	fastboot_nand_flash_write(cmd_parameter, fastboot_buf_addr, image_size,
				  response);
#endif
}

/**
 * erase() - erase the indicated partition.
 *
 * @cmd_parameter: Pointer to partition name
 * @response: Pointer to fastboot response buffer
 *
 * Erases the partition indicated by cmd_parameter (clear to 0x00s). Writes
 * to response.
 */
static void erase(char *cmd_parameter, char *response)
{
	u32 boot_mode = get_boot_pin_select();

	switch(boot_mode){
#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
		static bool mtd_flash = false;
		if (!strncmp("mtd", cmd_parameter, 3))
			mtd_flash = true;
		if (!strncmp("gpt", cmd_parameter, 3))
			mtd_flash = false;

		if (mtd_flash){
			fastboot_mtd_flash_erase(cmd_parameter, response);

			if (!strncmp("OKAY", response, 4))
				return;
		}

		/* erase blk dev */
		fastboot_blk_erase(cmd_parameter, response);
		return;
#endif
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
		fastboot_mmc_erase(cmd_parameter, response);
		return;
#endif
	}

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_NAND)
	fastboot_nand_erase(cmd_parameter, response);
#endif
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_UUU_SUPPORT)
/**
 * run_ucmd() - Execute the UCmd command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void run_ucmd(char *cmd_parameter, char *response)
{
	if (!cmd_parameter) {
		pr_err("missing slot suffix\n");
		fastboot_fail("missing command", response);
		return;
	}

	if (run_command(cmd_parameter, 0))
		fastboot_fail("", response);
	else
		fastboot_okay(NULL, response);
}

static char g_a_cmd_buff[64];

void fastboot_acmd_complete(void)
{
	run_command(g_a_cmd_buff, 0);
}

/**
 * run_acmd() - Execute the ACmd command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void run_acmd(char *cmd_parameter, char *response)
{
	if (!cmd_parameter) {
		pr_err("missing slot suffix\n");
		fastboot_fail("missing command", response);
		return;
	}

	if (strlen(cmd_parameter) > sizeof(g_a_cmd_buff)) {
		pr_err("too long command\n");
		fastboot_fail("too long command", response);
		return;
	}

	strcpy(g_a_cmd_buff, cmd_parameter);
	fastboot_okay(NULL, response);
}
#endif

#if !defined(CONFIG_SPL_BUILD)
/**
 * reboot_bootloader() - Sets reboot bootloader flag.
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void reboot_bootloader(char *cmd_parameter, char *response)
{
	if (fastboot_set_reboot_flag(FASTBOOT_REBOOT_REASON_BOOTLOADER))
		fastboot_fail("Cannot set reboot flag", response);
	else
		fastboot_okay(NULL, response);
}

/**
 * reboot_fastbootd() - Sets reboot fastboot flag.
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void reboot_fastbootd(char *cmd_parameter, char *response)
{
	if (fastboot_set_reboot_flag(FASTBOOT_REBOOT_REASON_FASTBOOTD))
		fastboot_fail("Cannot set fastboot flag", response);
	else
		fastboot_okay(NULL, response);
}

/**
 * reboot_recovery() - Sets reboot recovery flag.
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void reboot_recovery(char *cmd_parameter, char *response)
{
	if (fastboot_set_reboot_flag(FASTBOOT_REBOOT_REASON_RECOVERY))
		fastboot_fail("Cannot set recovery flag", response);
	else
		fastboot_okay(NULL, response);
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_FORMAT)
/**
 * oem_format() - Execute the OEM format command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void oem_format(char *cmd_parameter, char *response)
{
	char cmdbuf[32];

	if (!env_get("partitions")) {
		fastboot_fail("partitions not set", response);
	} else {
		sprintf(cmdbuf, "gpt write mmc %x $partitions",
			CONFIG_FASTBOOT_FLASH_MMC_DEV);
		if (run_command(cmdbuf, 0))
			fastboot_fail("", response);
		else
			fastboot_okay(NULL, response);
	}
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_PARTCONF)
/**
 * oem_partconf() - Execute the OEM partconf command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void oem_partconf(char *cmd_parameter, char *response)
{
	char cmdbuf[32];

	if (!cmd_parameter) {
		fastboot_fail("Expected command parameter", response);
		return;
	}

	/* execute 'mmc partconfg' command with cmd_parameter arguments*/
	snprintf(cmdbuf, sizeof(cmdbuf), "mmc partconf %x %s 0",
		 CONFIG_FASTBOOT_FLASH_MMC_DEV, cmd_parameter);
	pr_info("Execute: %s\n", cmdbuf);
	if (run_command(cmdbuf, 0))
		fastboot_fail("Cannot set oem partconf", response);
	else
		fastboot_okay(NULL, response);
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_BOOTBUS)
/**
 * oem_bootbus() - Execute the OEM bootbus command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void oem_bootbus(char *cmd_parameter, char *response)
{
	char cmdbuf[32];

	if (!cmd_parameter) {
		fastboot_fail("Expected command parameter", response);
		return;
	}

	/* execute 'mmc bootbus' command with cmd_parameter arguments*/
	snprintf(cmdbuf, sizeof(cmdbuf), "mmc bootbus %x %s",
		 CONFIG_FASTBOOT_FLASH_MMC_DEV, cmd_parameter);
	pr_info("Execute: %s\n", cmdbuf);
	if (run_command(cmdbuf, 0))
		fastboot_fail("Cannot set oem bootbus", response);
	else
		fastboot_okay(NULL, response);
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_READ)
/**
 * read_console_log() - Read data from console log buffer
 *
 * @fb_buf: Pointer to buffer where data will be copied
 *
 * @return: Actual size of data read
 */
static u32 read_console_log(char *fb_buf) {
	char *log_start = gd->console_log.buffer;
	char *log_end = log_start + LOG_BUFFER_SIZE;
	char *log_ptr = gd->console_log.read_ptr;
	u32 read_size = 0;

	while (log_ptr != gd->console_log.write_ptr) {
		u32 copy_size = (log_ptr < gd->console_log.write_ptr) ?
						(gd->console_log.write_ptr - log_ptr) :
						(log_end - log_ptr);

		memcpy(fb_buf + read_size, log_ptr, copy_size);
		read_size += copy_size;
		log_ptr += copy_size;

		if (log_ptr == log_end) {
			log_ptr = log_start;
		}
	}

	gd->console_log.read_ptr = log_ptr;

	return read_size;
}

/**
 * oem_read() - Execute the OEM read command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void oem_read(char *cmd_parameter, char *response)
{
	char *part, *offset_str, *cmd_str;
	u32 off, boot_mode;

	cmd_str = cmd_parameter;
	part = strsep(&cmd_str, " ");
	if (!part){
		fastboot_fail("miss part, send command:\
			fastboot oem read:part [offset]", response);
		return;
	}

	if (strcmp(part, "console") == 0) {
		char *fb_buf = (char *)fastboot_buf_addr;
		u32 read_size = read_console_log(fb_buf);

		fastboot_bytes_expected = read_size;
		fastboot_response("OKAY", response, "%08x", read_size);
		return;
	}

	offset_str = strsep(&cmd_str, " ");
	if (!offset_str){
		pr_info("miss offset, would set offset to 0\n");
		off = 0;
	}else{
		off = simple_strtoul(offset_str, NULL, 0);
	}

	debug("get part:%s, offset:%x\n", part, off);

	boot_mode = get_boot_pin_select();
	switch(boot_mode){
#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
		/*mtd read part not support read raw data*/
		fastboot_bytes_expected = fastboot_mtd_flash_read(part, off, fastboot_buf_addr, response);

		/* if read data from mtd partition success, it would not try to read from blk dev*/
		if (fastboot_bytes_expected > 0)
			return;
		pr_info("read data from blk dev\n");
		fastboot_bytes_expected = fastboot_blk_read(part, off, fastboot_buf_addr, response);

		return;
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
		fastboot_bytes_expected = fastboot_mmc_read(part, off, fastboot_buf_addr, response);
		return;
#endif
	}

	fastboot_okay(NULL, response);
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_CONFIG_ACCESS)
void fastboot_config_access(char *operation, char *config, char *response);
/**
 * oem_config() - Execute the OEM config command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void oem_config(char *cmd_parameter, char *response)
{
    char *cmd_str, *operation;

	cmd_str = cmd_parameter;
	operation = strsep(&cmd_str, " ");

    fastboot_config_access(operation, cmd_str, response);
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_ENV_ACCESS)
void fastboot_env_access(char *operation, char *env, char *response);
/**
 * oem_env() - Execute the OEM env operation command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void oem_env(char *cmd_parameter, char *response)
{
    char *cmd_str, *operation;

	cmd_str = cmd_parameter;
	operation = strsep(&cmd_str, " ");

    fastboot_env_access(operation, cmd_str, response);
}
#endif
