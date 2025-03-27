// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */
#include <common.h>
#include <env.h>
#include <log.h>
#include <asm/global_data.h>
#include <net.h>
#include <tlv_eeprom.h>
#include <vsprintf.h>
#include <ctype.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <u-boot/crc.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <net.h>
#include <env_flags.h>
#include <mtd.h>
#include <jffs2/load_kernel.h>
#include <mtd.h>
#include <jffs2/load_kernel.h>
#include <vsprintf.h>
#include <ctype.h>
#include <dm.h>
#include <fb_spacemit.h>
#include <watchdog.h>
#include "nfs_env.h"

#define NFS_LOAD_ADDR  CONFIG_FASTBOOT_BUF_ADDR

int eth_parse_enetaddr(const char *addr, uint8_t *enetaddr)
{
	char *end;
	int i;

	if (strlen(addr) != 17) {
		pr_err("Invalid MAC address length\n");
		return -1;
	}

	for (i = 0; i < 6; i++) {
		enetaddr[i] = simple_strtoul(addr, &end, 16);
		if (i < 5 && *end != ':') {
			pr_err("Invalid MAC address format\n");
			return -1;
		}
		addr = end + 1;
	}

	pr_info("Successfully parsed MAC address\n");
	return 0;
}


static int get_mac_address(uint8_t *mac_addr)
{
	const char *mac_str = env_get("ethaddr");
	if (mac_str) {
		if (eth_validate_ethaddr_str(mac_str) == 0) {
			if (eth_parse_enetaddr(mac_str, mac_addr) == 0) {
				pr_info("Successfully obtained MAC address from environment\n");
				return 0;
			} else {
				pr_err("Failed to parse MAC address from environment\n");
			}
		} else {
			pr_err("Invalid MAC address in environment\n");
		}
	} else {
		pr_debug("MAC address not found in environment\n");
	}
	return -1;
}

static int read_mac_from_eeprom(uint8_t *mac_addr)
{
	struct tlvinfo_tlv *mac_base_tlv = NULL;
	read_from_eeprom(&mac_base_tlv, TLV_CODE_MAC_BASE);
	if (mac_base_tlv && mac_base_tlv->length == 6) {
		memcpy(mac_addr, mac_base_tlv->value, 6);
		pr_info("Successfully read MAC address from EEPROM: %02x:%02x:%02x:%02x:%02x:%02x\n",
				mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		return 0;
	}
	pr_err("Failed to read valid MAC address from EEPROM\n");
	return -1;
}

static int get_device_mac_address(uint8_t *mac_addr)
{
	// Try to get MAC address from environment first
	if (get_mac_address(mac_addr) == 0) {
		return 0;
	}

	// If not found in environment, try to read from EEPROM
	if (read_mac_from_eeprom(mac_addr) == 0) {
		char mac_str[18];
		sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
				mac_addr[0], mac_addr[1], mac_addr[2],
				mac_addr[3], mac_addr[4], mac_addr[5]);
		env_set("ethaddr", mac_str);
		return 0;
	}

	pr_err("Failed to get MAC address\n");
	return -1;
}

static int strcasecmp_colon(const char *s1, const char *s2, size_t n) {
	for (size_t i = 0; i < n; i++) {
		char c1 = tolower((unsigned char)s1[i]);
		char c2 = tolower((unsigned char)s2[i]);
		if (c1 != c2) {
			return c1 - c2;
		}
		if (c1 == '\0') {
			return 0;
		}
		if (c1 == ':') {
			continue;
		}
	}
	return 0;
}

static int load_mac_mapping(struct mac_mapping *mapping, const char *mac_mapping_file)
{
	char cmd[128] = {"\0"};
	char *buf = (char *)NFS_LOAD_ADDR;
	char *line, *next_line;
	int ret = -1;
	uint8_t mac_addr[6];
	char mac_str[18];
	size_t mac_str_len;
	const char *server_ip;
	bool found_match = false;

	// Run DHCP
	sprintf(cmd, "dhcp");
	ret = run_command(cmd, 0);
	if (ret != 0) {
		pr_err("DHCP failed, return value: %d\n", ret);
		return -1;
	}

	// Get server IP address
	server_ip = env_get("serverip");
	if (!server_ip) {
		pr_err("serverip not set in environment\n");
		return -1;
	}

	// Get device MAC address
	if (get_device_mac_address(mac_addr) != 0) {
		pr_err("Failed to get device MAC address\n");
		return -1;
	}

	// Convert MAC address to string format
	sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac_addr[0], mac_addr[1], mac_addr[2],
			mac_addr[3], mac_addr[4], mac_addr[5]);
	mac_str_len = strlen(mac_str);

	// Construct NFS command
	sprintf(cmd, "nfs %lx %s:%s", (ulong)buf, server_ip, mac_mapping_file);

	// Execute NFS command
	ret = run_command(cmd, 0);

	if (ret != 0) {
		pr_err("Failed to load %s from NFS\n", mac_mapping_file);
		return -1;
	}

	// Parse file content
	line = buf;
	while (line && *line) {
		next_line = strchr(line, '\n');
		if (next_line) {
			*next_line = '\0';
		}

		if (strcasecmp_colon(line, mac_str, mac_str_len) == 0) {
			char *bootfs_start = strchr(line, ',');
			if (bootfs_start) {
				bootfs_start++;
				char *rootfs_start = strchr(bootfs_start, ',');
				if (rootfs_start) {
					*rootfs_start = '\0';
					rootfs_start++;

					// Copy bootfs path
					strncpy(mapping->bootfs_path, bootfs_start, sizeof(mapping->bootfs_path) - 1);
					mapping->bootfs_path[sizeof(mapping->bootfs_path) - 1] = '\0';

					// Copy rootfs path
					strncpy(mapping->rootfs_path, rootfs_start, sizeof(mapping->rootfs_path) - 1);
					mapping->rootfs_path[sizeof(mapping->rootfs_path) - 1] = '\0';

					found_match = true;
					ret = 0;
					break;
				}
			}
		}

		if (next_line) {
			line = next_line + 1;
		} else {
			break;
		}
	}

	if (!found_match) {
		pr_err("Failed to find matching MAC address in %s\n", mac_mapping_file);
		ret = -1;
	}

	return ret;
}

static int check_nand_bootfs(void)
{
	char cmd[128];
	const char *bootfs_name = BOOTFS_NAME;

	if (!bootfs_name) {
		return -1;
	}

	snprintf(cmd, sizeof(cmd), "ubi part %s", parse_mtdparts_and_find_bootfs());
	if (run_command(cmd, 0) != 0) {
		return -1;
	}

	snprintf(cmd, sizeof(cmd), "ubifsmount %s", bootfs_name);
	if (run_command(cmd, 0) != 0) {
		return -1;
	}

	return 0;
}

static int check_nor_bootfs(void)
{
	int part, blk_index;
	char *blk_name;
	char devpart_str[16];

	if (get_available_boot_blk_dev(&blk_name, &blk_index)) {
		pr_err("Cannot get available block device\n");
		return -1;
	}

	part = detect_blk_dev_or_partition_exist(blk_name, blk_index, BOOTFS_NAME);
	if (part < 0) {
		pr_err("Failed to detect partition %s on %s:%d\n", BOOTFS_NAME, blk_name, blk_index);
		return -1;
	}

	snprintf(devpart_str, sizeof(devpart_str), "%d:%d", blk_index, part);

	if (!strcmp("mmc", blk_name)) {
		pr_info("Found bootfs partition on eMMC device %s\n", devpart_str);
	} else if (!strcmp("nvme", blk_name)) {
		pr_info("Found bootfs partition on NVMe device %s\n", devpart_str);
	} else {
		pr_info("Not found bootfs partition on %s\n", blk_name);
		return -1;
	}

	return 0;
}

static int check_mmc_bootfs(int dev)
{
	struct mmc *mmc;
	struct blk_desc *desc;
	struct disk_partition info;
	int part;

	mmc = find_mmc_device(dev);
	if (!mmc) {
		return -1;
	}

	if (mmc_init(mmc)) {
		return -1;
	}

	desc = mmc_get_blk_desc(mmc);
	for (part = 1; part <= MAX_SEARCH_PARTITIONS; part++) {
		if (part_get_info(desc, part, &info) != 0) {
			continue;
		}
		if (strcmp(info.name, BOOTFS_NAME) == 0) {
			return 0;
		}
	}

	return -1;
}

int check_bootfs_exists(void)
{
	int ret;
	u32 boot_mode = get_boot_mode();

	switch (boot_mode) {
	case BOOT_MODE_NAND:
		ret = check_nand_bootfs();
		break;
	case BOOT_MODE_NOR:
		ret = check_nor_bootfs();
		break;
	case BOOT_MODE_EMMC:
		ret = check_mmc_bootfs(MMC_DEV_EMMC);
		break;
	case BOOT_MODE_SD:
		ret = check_mmc_bootfs(MMC_DEV_SD);
		break;
	default:
		pr_info("Unsupported boot mode for checking bootfs\n");
		return -1;
	}

	return ret;
}

#ifdef CONFIG_CMD_NET
static int initr_net(void)
{
	printf("Net:   ");
	eth_initialize();
#if defined(CONFIG_RESET_PHY_R)
	debug("Reset Ethernet PHY\n");
	reset_phy();
#endif
	return 0;
}
#endif

int run_net_flash_command(void)
{
#ifdef CONFIG_CMD_NET
	char cmd[128];
	char *ethaddr;
	char *tftp_load_addr = (char *)NFS_LOAD_ADDR;
	const char *flash_cmd = "flash_image net";
	char *filesize_str;
	ulong filesize = 0;
	int ret;
	char *net_flash_mode;

	// Check if net flash mode is enabled
	net_flash_mode = env_get("net_flash_protocol");
	if (!net_flash_mode || strcmp(net_flash_mode, "spacemit_tftp") != 0) {
		pr_info("Net flash mode not enabled\n");
		return 1;
	}

	ret = initr_net();
	if (ret != 0) {
		pr_err("Failed to initialize network\n");
		goto flash_fail;
	}

	// Run DHCP
	sprintf(cmd, "dhcp");
	ret = run_command(cmd, 0);
	if (ret != 0) {
		pr_err("DHCP failed, return value: %d\n", ret);
		goto flash_fail;
	}

	// Get mac address after DHCP
	ethaddr = env_get("ethaddr");
	if (!ethaddr) {
		pr_err("mac address not set in environment\n");
		goto flash_fail;
	}

	// Construct TFTP command
	snprintf(cmd, sizeof(cmd), "tftpboot %lx %s", (ulong)tftp_load_addr, ethaddr);
	if (run_command(cmd, 0) != 0) {
		pr_err("Failed to execute TFTP command\n");
		goto flash_fail;
	}

	// Get downloaded file size from environment
	filesize_str = env_get("filesize");
	if (!filesize_str) {
		pr_err("Failed to get filesize from environment\n");
		goto flash_fail;
	}

	filesize = simple_strtoul(filesize_str, NULL, 16);
	pr_info("Downloaded size: %lu bytes\n", filesize);
	pr_info("Received content: '");
	for (int i = 0; i < filesize && i < 64; i++) {
		printf("%c", tftp_load_addr[i]);
	}
	printf("'\n");

	// Check for flash command prefix
	char *path_start = strstr(tftp_load_addr, "net_data_path=");
	if (path_start) {
		char path[128];
		char server_ip[16];
		int len = 0;
		path_start += 13; // Skip "net_data_path="

		// Find IP and path separator
		char *colon = strchr(path_start, ':');
		if (colon) {
			// Extract server IP address
			len = colon - path_start;
			if (len < sizeof(server_ip)) {
				strncpy(server_ip, path_start, len);
				server_ip[len] = '\0';
				env_set("serverip", server_ip);
				pr_info("Set serverip=%s\n", server_ip);

				// Extract path portion after colon
				path_start = colon + 1;  // Skip colon
				len = 0;
				while (*path_start && len < sizeof(path) - 1) {
					path[len++] = *path_start++;
				}
				path[len] = '\0';

				env_set("net_data_path", path);
				pr_info("Set net_data_path=%s\n", path);

				// Execute flash command
				if (run_command(flash_cmd, 0) != 0) {
					pr_err("Failed to execute flash command\n");
					goto flash_fail;
				}
				return 0;
			}
		}
		pr_err("Invalid format: IP:path not found\n");
		goto flash_fail;
	}

	pr_info("Continuing with NFS boot\n");
	return 1;

flash_fail:
	printf(FAIL_BANNER);
	while(1) {
		/* do not return while flashing over! */
		WATCHDOG_RESET();
	}
#else
	return 0;
#endif
}

int load_env_from_nfs(void)
{
	const char *mac_mapping_file;
	struct mac_mapping mapping;
	char cmd[128] = {"\0"};
	int ret;

	if (env_get("boot_override")) {
		env_set("boot_override", NULL);
	}

	mac_mapping_file = env_get("mac_mapping_file");
	if (!mac_mapping_file) {
		pr_err("MAC mapping file path not set in environment\n");
		return -1;
	}

	ret = load_mac_mapping(&mapping, mac_mapping_file);
	if (ret == 0) {
		env_set("bootfs_path", mapping.bootfs_path);
		env_set("rootfs_path", mapping.rootfs_path);
		env_set("boot_override", "nfs");

		printf("bootfs_path: %s\n", env_get("bootfs_path"));
		printf("rootfs_path: %s\n", env_get("rootfs_path"));
		printf("boot_override: %s\n", env_get("boot_override"));

		sprintf(cmd, "nfs %x %s/env_%s.txt", 
				NFS_LOAD_ADDR, 
				mapping.bootfs_path, 
				CONFIG_SYS_CONFIG_NAME);

		if (run_command(cmd, 0) == 0) {
			sprintf(cmd, "env import -t %x", NFS_LOAD_ADDR);
			if (run_command(cmd, 0) == 0) {
				pr_info("Successfully loaded environment from NFS\n");
				return 0;
			} else {
				pr_err("Failed to import environment\n");
			}
		} else {
			pr_err("Failed to load environment file from NFS\n");
		}
	} else {
		pr_err("Failed to load MAC mapping from NFS. Return code: %d\n", ret);
	}

	return -1;
}
