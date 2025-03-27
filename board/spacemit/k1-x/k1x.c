// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <common.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <dm/lists.h>
#include <env.h>
#include <fdtdec.h>
#include <image.h>
#include <log.h>
#include <mapmem.h>
#include <spl.h>
#include <init.h>
#include <virtio_types.h>
#include <virtio.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <stdlib.h>
#include <linux/io.h>
#include <asm/global_data.h>
#include <part.h>
#include <env_internal.h>
#include <asm/arch/ddr.h>
#include <power/regulator.h>
#include <fb_spacemit.h>
#include <net.h>
#include <i2c.h>
#include <linux/delay.h>
#include <tlv_eeprom.h>
#include <u-boot/crc.h>
#include <fb_mtd.h>
#include <power/pmic.h>
#include <dm/device.h>
#include <dm/device-internal.h>
#include <g_dnl.h>
#include <fdt_simplefb.h>
#include <mtd_node.h>
#include <misc.h>
#ifdef CONFIG_ENV_IS_IN_NFS
#include "nfs_env.h"
#endif
#ifdef CONFIG_BUTTON
#include <button.h>
struct fastboot_key_config {
	const char **key_names;
	int key_count;
	u32 press_time;
};
#endif

DECLARE_GLOBAL_DATA_PTR;
static char found_partition[64] = {0};
extern u32 ddr_cs_num;
bool is_video_connected = false;
uint32_t reboot_config;
void refresh_config_info(u8 *eeprom_data);
void read_from_eeprom(struct tlvinfo_tlv **tlv_data, u8 tcode);
int mac_read_from_buffer(u8 *eeprom_data);

void set_boot_mode(enum board_boot_mode boot_mode)
{
	writel(boot_mode, (void *)BOOT_DEV_FLAG_REG);
}

enum board_boot_mode get_boot_pin_select(void)
{
	/*if not set boot mode, try to return boot pin select*/
	u32 boot_select = readl((void *)BOOT_PIN_SELECT) & BOOT_STRAP_BIT_STORAGE_MASK;
	boot_select = boot_select >> BOOT_STRAP_BIT_OFFSET;
	pr_debug("boot_select:%x\n", boot_select);

	/*select spl boot device:
		 b'(bit1)(bit0)
	emmc:b'00, //BOOT_STRAP_BIT_EMMC
	nor :b'10, //BOOT_STRAP_BIT_NOR
	nand:b'01, //BOOT_STRAP_BIT_NAND
	sd  :b'11, //BOOT_STRAP_BIT_SD
*/
	switch (boot_select) {
	case BOOT_STRAP_BIT_EMMC:
		return BOOT_MODE_EMMC;
	case BOOT_STRAP_BIT_NAND:
		return BOOT_MODE_NAND;
	case BOOT_STRAP_BIT_NOR:
		return BOOT_MODE_NOR;
	case BOOT_STRAP_BIT_SD:
	default:
		return BOOT_MODE_SD;
	}
}

enum board_boot_mode get_boot_mode(void)
{
	/*if usb boot or has set boot mode, return boot mode*/
	u32 boot_mode = readl((void *)BOOT_DEV_FLAG_REG);
	pr_debug("%s, boot_mode:%x\n", __func__, boot_mode);

	switch (boot_mode) {
	case BOOT_MODE_USB:
		return BOOT_MODE_USB;
	case BOOT_MODE_EMMC:
		return BOOT_MODE_EMMC;
	case BOOT_MODE_NAND:
		return BOOT_MODE_NAND;
	case BOOT_MODE_NOR:
		return BOOT_MODE_NOR;
	case BOOT_MODE_SD:
		return BOOT_MODE_SD;
	case BOOT_MODE_SHELL:
		return BOOT_MODE_SHELL;
	}

	/*else return boot pin select*/
	return get_boot_pin_select();
}

void set_serialnumber_based_on_boot_mode(void)
{
	const char *s = env_get("serial#");
	enum board_boot_mode boot_mode = get_boot_mode();

	if (boot_mode != BOOT_MODE_USB && s) {
		g_dnl_set_serialnumber((char *)s);
	}
}

enum board_boot_mode get_boot_storage(void)
{
	enum board_boot_mode boot_storage = get_boot_mode();

	// save to card only when boot from card
	if (BOOT_MODE_SD != boot_storage)
		boot_storage =  get_boot_pin_select();

	return boot_storage;
}

int mmc_get_env_dev(void)
{
	u32 boot_mode = 0;
	boot_mode = get_boot_mode();
	pr_debug("%s, uboot boot_mode:%x\n", __func__, boot_mode);

	if (boot_mode == BOOT_MODE_EMMC)
		return MMC_DEV_EMMC;
	else
		return MMC_DEV_SD;
}

static bool write_boot_storage_emmc(ulong byte_addr, ulong byte_size, void *buff)
{
	struct blk_desc *dev_desc = blk_get_dev("mmc", MMC_DEV_EMMC);

	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return false;
	}

	blk_dselect_hwpart(dev_desc, 0);
	pr_info("write %ldbyte to emmc address %ld\n", byte_size, byte_addr);
	blk_dwrite(dev_desc,
		byte_addr / dev_desc->blksz,
		byte_size / dev_desc->blksz, buff);
	return true;
}

static bool write_boot_storage_sdcard(ulong byte_addr, ulong byte_size, void *buff)
{
	struct blk_desc *dev_desc = blk_get_dev("mmc", MMC_DEV_SD);

	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid sd device\n");
		return false;
	}

	pr_info("write %ldbyte to sdcard address %ld\n", byte_size, byte_addr);
	blk_dwrite(dev_desc,
		byte_addr / dev_desc->blksz,
		byte_size / dev_desc->blksz, buff);
	return true;
}

static bool write_boot_storage_spinor(ulong byte_addr, ulong byte_size, void *buff)
{
	struct mtd_info *mtd;
	const char* part = "private";

	mtd_probe_devices();
	mtd = get_mtd_device_nm(part);
	if ((NULL != mtd) && (0 == _fb_mtd_erase(mtd, byte_size))
		&& (0 == _fb_mtd_write(mtd, buff, byte_addr, byte_size, NULL))) {
		pr_info("write %ldbyte to spinor partition %s @offset %ld\n", byte_size, part, byte_addr);
		return true;
	}
	else
		return false;
}

static const struct boot_storage_op storage_write[] = {
	{BOOT_MODE_EMMC, 0x10000, NULL, write_boot_storage_emmc},
	{BOOT_MODE_SD, 0x10000, NULL, write_boot_storage_sdcard},
	{BOOT_MODE_NOR, 0, NULL, write_boot_storage_spinor},
};

static bool write_training_info(void *buff, ulong byte_size)
{
	int i;
	// save data to boot storage
	enum board_boot_mode boot_storage = get_boot_storage();

	for (i = 0; i < ARRAY_SIZE(storage_write); i++) {
		if (boot_storage == storage_write[i].boot_storage)
			return storage_write[i].write(storage_write[i].address, byte_size, buff);
	}

	return false;
}

void save_ddr_training_info(void)
{
	struct ddr_training_info_t *info;
	info = (struct ddr_training_info_t*)map_sysmem(DDR_TRAINING_INFO_BUFF, 0);

	if ((DDR_TRAINING_INFO_MAGIC == info->magic) &&
		(info->crc32 == crc32(0, (const uchar *)&info->chipid, sizeof(*info) - 8))) {
		// save DDR training info to boot storage
		write_training_info(info, sizeof(*info));
	}
}

void get_ddr_config_info(void)
{
	struct ddr_training_info_t *info;
	info = (struct ddr_training_info_t*)map_sysmem(DDR_TRAINING_INFO_BUFF, 0);

	if ((DDR_TRAINING_INFO_MAGIC == info->magic) &&
		(info->crc32 == crc32(0, (const uchar *)&info->chipid, sizeof(*info) - 8))) {
		// get DDR cs number that is update in spl stage
		ddr_cs_num = info->cs_num;
	}
	else
		ddr_cs_num = DDR_CS_NUM;
}

u32 get_reboot_config(void)
{
	int ret;
	struct udevice *dev;
	u32 flag = 0;
	uint8_t value;

	if (reboot_config)
		return reboot_config;

	/* K1 has non-reset register(BOOT_CIU_DEBUG_REG0) to save boot config
	   before system reboot, but it will be clear when K1 power is down,
	   then boot config will be save in P1.
	*/
	flag = readl((void *)BOOT_CIU_DEBUG_REG0);
	if ((BOOT_MODE_SHELL == flag) || (BOOT_MODE_USB == flag)) {
		/* reset  */
		writel(0, (void *)BOOT_CIU_DEBUG_REG0);
		reboot_config = flag;
	}
	else {
		// select boot mode from boot strap pin
		reboot_config = BOOT_MODE_BOOTSTRAP;
		ret = uclass_get_device_by_driver(UCLASS_PMIC,
				DM_DRIVER_GET(spacemit_pm8xx), &dev);
		if (ret) {
			pr_err("PMIC init failed: %d\n", ret);
			return 0;
		}
		pmic_read(dev, P1_NON_RESET_REG, &value, 1);
		pr_info("Read PMIC reg %x value %x\n", P1_NON_RESET_REG, value);
		if (1 == (value & 3)) {
			reboot_config = BOOT_MODE_USB;
			value &= ~3;
			pmic_write(dev, P1_NON_RESET_REG, &value, 1);
		}
		else if (2 == (value & 3)) {
			reboot_config = BOOT_MODE_SHELL;
			value &= ~3;
			pmic_write(dev, P1_NON_RESET_REG, &value, 1);
		}
	}

	return reboot_config;
}


#ifdef CONFIG_BUTTON
static int button_get_state_by_label(struct udevice *dev, const char *label)
{
	struct udevice *child;
	struct button_uc_plat *plat;
	int state;
	bool invert_state = false;

	pr_debug("Searching for button with label '%s'\n", label);
	for (device_find_first_child(dev, &child);
			child;
			device_find_next_child(&child)) {
		plat = dev_get_uclass_plat(child);
		if (plat->label && !strcmp(plat->label, label)) {
			invert_state = ofnode_read_bool(dev_ofnode(child), "invert-state");

			state = button_get_state(child);
			pr_debug("Button '%s' found, raw state: %d, invert: %d\n", label, state, invert_state);

			if (invert_state) {
				state = (state == BUTTON_ON) ? BUTTON_OFF : BUTTON_ON;
			}
			pr_debug("Button '%s' final state: %d\n", label, state);
			return state;
		}
	}

	pr_err("Button '%s' not found\n", label);
	return -ENOENT;
}

static int get_fastboot_key_config(struct fastboot_key_config *config)
{
	ofnode node;
	int ret;

	node = ofnode_path("/gpio_keys");
	if (!ofnode_valid(node))
		return -ENODEV;

	ret = ofnode_read_string_list(node, "fastboot-key-combo", &config->key_names);
	if (ret < 0)
		return ret;
	config->key_count = ret;

	ret = ofnode_read_u32(node, "fastboot-key-press-time", &config->press_time);
	if (ret < 0)
		return ret;

	return 0;
}

static bool check_fastboot_keys(void)
{
	struct udevice *dev;
	struct fastboot_key_config config;
	int *key_states;
	ulong press_start = 0;
	int i, ret;
	bool all_pressed = true;

	ret = get_fastboot_key_config(&config);
	if (ret < 0) {
		pr_err("Failed to get fastboot key config: %d\n", ret);
		return false;
	}

	pr_debug("Fastboot key config: count=%d, press_time=%u\n", config.key_count, config.press_time);

	ret = uclass_get_device_by_name(UCLASS_BUTTON, "gpio_keys", &dev);
	if (ret) {
		pr_err("Failed to get device for gpio_keys\n");
		return false;
	}

	key_states = calloc(config.key_count, sizeof(int));
	if (!key_states) {
		pr_err("Failed to allocate memory for key_states\n");
		return false;
	}

	press_start = get_timer(0);

	while (get_timer(press_start) < config.press_time) {
		all_pressed = true;
		for (i = 0; i < config.key_count; i++) {
			key_states[i] = button_get_state_by_label(dev, config.key_names[i]);
			if (key_states[i] < 0 || key_states[i] != BUTTON_ON) {
				all_pressed = false;
				break;
			}
		}

		if (!all_pressed) {
			/* Key released within the specified time, normal boot */
			free(key_states);
			return false;
		}

		mdelay(10);
	}

	/* Keys held down longer than specified time, enter Fastboot mode */
	free(key_states);
	pr_info("Fastboot key combination detected! Duration: %u ms\n", config.press_time);
	return true;
}
#endif


void run_fastboot_command(void)
{
	u32 boot_mode = get_boot_mode();

	if (boot_mode == BOOT_MODE_USB || BOOT_MODE_USB == get_reboot_config()
#ifdef CONFIG_BUTTON
		|| check_fastboot_keys()
#endif
	) {
		/* show flash log*/
		env_set("stdout", env_get("stdout_flash"));

		char *cmd_para = "fastboot 0";
		run_command(cmd_para, 0);

		/*read from eeprom and update info to env*/
		refresh_config_info(NULL);
	}
}


int run_uboot_shell(void)
{
	u32 boot_mode = get_boot_mode();

	if (boot_mode == BOOT_MODE_SHELL || BOOT_MODE_SHELL == get_reboot_config()) {
		return 0;
	}
	return 1;
}

void _load_env_from_blk(struct blk_desc *dev_desc, const char *dev_name, int dev)
{
	int err;
	u32 part;
	char cmd[128];
	struct disk_partition info;

	for (part = 1; part <= MAX_SEARCH_PARTITIONS; part++) {
		err = part_get_info(dev_desc, part, &info);
		if (err)
			continue;
		if (!strcmp(BOOTFS_NAME, info.name)){
			pr_debug("match info.name:%s\n", info.name);
			break;
		}
	}
	if (part > MAX_SEARCH_PARTITIONS)
		return;

	env_set("bootfs_part", simple_itoa(part));
	env_set("bootfs_devname", dev_name);

	/*load env.txt and import to uboot*/
	memset((void *)CONFIG_SPL_LOAD_FIT_ADDRESS, 0, CONFIG_ENV_SIZE);
	sprintf(cmd, "load %s %d:%d 0x%x env_%s.txt", dev_name,
			dev, part, CONFIG_SPL_LOAD_FIT_ADDRESS, CONFIG_SYS_CONFIG_NAME);
	pr_debug("cmd:%s\n", cmd);
	if (run_command(cmd, 0))
		return;

	memset(cmd, '\0', 128);
	sprintf(cmd, "env import -t 0x%x", CONFIG_SPL_LOAD_FIT_ADDRESS);
	pr_debug("cmd:%s\n", cmd);
	if (!run_command(cmd, 0)){
		pr_info("load env_%s.txt from bootfs successful\n", CONFIG_SYS_CONFIG_NAME);
	}
}

char* parse_mtdparts_and_find_bootfs(void) {
	const char *mtdparts = env_get("mtdparts");
	char cmd_buf[256];

	if (!mtdparts) {
		pr_debug("mtdparts not set\n");
		return NULL;
	}

	/* Find the last partition */
	const char *last_part_start = strrchr(mtdparts, '(');
	if (last_part_start) {
		last_part_start++; /* Skip the left parenthesis */
		const char *end = strchr(last_part_start, ')');
		if (end && (end - last_part_start < sizeof(found_partition))) {
			int len = end - last_part_start;
			strncpy(found_partition, last_part_start, len);
			found_partition[len] = '\0';

			snprintf(cmd_buf, sizeof(cmd_buf), "ubi part %s", found_partition);
			if (run_command(cmd_buf, 0) == 0) {
				/* Check if the bootfs volume exists */
				snprintf(cmd_buf, sizeof(cmd_buf), "ubi check %s", BOOTFS_NAME);
				if (run_command(cmd_buf, 0) == 0) {
					pr_info("Found bootfs in partition: %s\n", found_partition);
					return found_partition;
				}
			}
		}
	}

	pr_debug("bootfs not found in any partition\n");
	return NULL;
}

void import_env_from_bootfs(void)
{
	u32 boot_mode = get_boot_mode();

#ifdef CONFIG_ENV_IS_IN_NFS
	// Check if local bootfs exists
	if ((BOOT_MODE_USB != boot_mode) && check_bootfs_exists() != 0) {
		#ifdef CONFIG_CMD_NET
			eth_initialize();
		#endif
		// Local bootfs not found, try to load from NFS
		if (load_env_from_nfs() == 0) {
			return;
		}
	}
#endif

	switch (boot_mode) {
	case BOOT_MODE_NAND:
#if CONFIG_IS_ENABLED(ENV_IS_IN_MTD)
		/*load env from nand bootfs*/
		const char *bootfs_name = BOOTFS_NAME ;
		char cmd[128];

		if (!bootfs_name) {
			pr_err("bootfs not set\n");
			return;
		}

		/* Parse mtdparts to find the partition containing the BOOTFS_NAME volume */
		char *mtd_partition   = parse_mtdparts_and_find_bootfs();
		if (!mtd_partition  ) {
			pr_err("Bootfs not found in any partition\n");
			return;
		}

		sprintf(cmd, "ubifsmount ubi0:%s", bootfs_name);
		if (run_command(cmd, 0)) {
			pr_err("Cannot mount ubifs partition '%s'\n", bootfs_name);
			return;
		}

		memset((void *)CONFIG_SPL_LOAD_FIT_ADDRESS, 0, CONFIG_ENV_SIZE);
		sprintf(cmd, "ubifsload 0x%x env_%s.txt", CONFIG_SPL_LOAD_FIT_ADDRESS, CONFIG_SYS_CONFIG_NAME);
		if (run_command(cmd, 0)) {
			pr_err("Failed to load env_%s.txt from bootfs\n", CONFIG_SYS_CONFIG_NAME);
			return;
		}

		memset(cmd, '\0', 128);
		sprintf(cmd, "env import -t 0x%x", CONFIG_SPL_LOAD_FIT_ADDRESS);
		if (!run_command(cmd, 0)) {
			pr_err("Imported environment from 'env_k1-x.txt'\n");
		}
#endif
		break;
	case BOOT_MODE_NOR:
		struct blk_desc *dev_desc;
		char *blk_name;
		int blk_index;

		if (get_available_boot_blk_dev(&blk_name, &blk_index)){
			printf("can not get available blk dev\n");
			return;
		}

		dev_desc = blk_get_dev(blk_name, blk_index);
		if (dev_desc)
			_load_env_from_blk(dev_desc, blk_name, blk_index);
		break;
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
#ifdef CONFIG_MMC
		int dev;
		struct mmc *mmc;

		dev = mmc_get_env_dev();
		mmc = find_mmc_device(dev);
		if (!mmc) {
			pr_err("Cannot find mmc device\n");
			return;
		}
		if (mmc_init(mmc)){
			return;
		}

		_load_env_from_blk(mmc_get_blk_desc(mmc), "mmc", dev);
		break;
#endif
	default:
		break;
	}
	return;
}

void run_cardfirmware_flash_command(void)
{
	struct mmc *mmc;
	struct disk_partition info;
	int part_dev, err;
	char cmd[128] = {"\0"};

#ifdef CONFIG_MMC
	mmc = find_mmc_device(MMC_DEV_SD);
	if (!mmc)
		return;
	if (mmc_init(mmc))
		return;

	for (part_dev = 1; part_dev <= MAX_SEARCH_PARTITIONS; part_dev++) {
		err = part_get_info(mmc_get_blk_desc(mmc), part_dev, &info);
		if (err)
			continue;
		if (!strcmp(BOOTFS_NAME, info.name))
			break;

	}

	if (part_dev > MAX_SEARCH_PARTITIONS)
		return;

	/*check if flash config file is in sd card*/
	sprintf(cmd, "fatsize mmc %d:%d %s", MMC_DEV_SD, part_dev, FLASH_CONFIG_FILE_NAME);
	pr_debug("cmd:%s\n", cmd);
	if (!run_command(cmd, 0)){
		/* show flash log*/
		env_set("stdout", env_get("stdout_flash"));
		run_command("flash_image mmc", 0);
	}
#endif
	return;
}

void setenv_boot_mode(void)
{
#ifdef CONFIG_ENV_IS_IN_NFS
	const char *boot_override = env_get("boot_override");

	if (boot_override) {
		env_set("boot_device", boot_override);
		env_set("boot_override", NULL);
		return;
	}
#endif

	u32 boot_mode = get_boot_mode();
	switch (boot_mode) {
	case BOOT_MODE_NAND:
		env_set("boot_device", "nand");
		break;
	case BOOT_MODE_NOR:
		char *blk_name;
		int blk_index;

		if (get_available_boot_blk_dev(&blk_name, &blk_index)){
			printf("can not get available blk dev\n");
			return;
		}

		env_set("boot_device", "nor");
		env_set("boot_devnum", simple_itoa(blk_index));
		break;
	case BOOT_MODE_EMMC:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", simple_itoa(MMC_DEV_EMMC));
		break;
	case BOOT_MODE_SD:
		env_set("boot_device", "mmc");
		env_set("boot_devnum", simple_itoa(MMC_DEV_SD));
		break;
	case BOOT_MODE_USB:
		// for fastboot image download and run test
		env_set("bootcmd", CONFIG_BOOTCOMMAND);
		break;
	default:
		env_set("boot_device", "");
		break;
	}
}

void read_from_eeprom(struct tlvinfo_tlv **tlv_data, u8 tcode)
{
	static u8 eeprom_data[256];
	struct tlvinfo_header *tlv_hdr = NULL;
	struct tlvinfo_tlv *tlv_entry;
	unsigned int tlv_offset, tlv_len;
	int ret = 0;

	ret = read_tlvinfo_tlv_eeprom(eeprom_data, &tlv_hdr, &tlv_entry, 0);
	if (ret < 0) {
		pr_err("read tlvinfo from eeprom failed!\n");
		return;
	}

	tlv_offset = sizeof(struct tlvinfo_header);
	tlv_len = sizeof(struct tlvinfo_header) + be16_to_cpu(tlv_hdr->totallen);
	while (tlv_offset < tlv_len) {
		tlv_entry = (struct tlvinfo_tlv *)&eeprom_data[tlv_offset];
		if (tlv_entry->type == tcode) {
			*tlv_data = tlv_entry;
			return;
		}

		tlv_offset += sizeof(struct tlvinfo_tlv) + tlv_entry->length;
	}

	*tlv_data = NULL;
	return;
}

struct tlvinfo_tlv *find_tlv_in_buffer(u8 *eeprom_data, u8 tcode)
{
	struct tlvinfo_header *hdr = (struct tlvinfo_header *)eeprom_data;
	int total_length = be16_to_cpu(hdr->totallen);
	u8 *tlv_end = eeprom_data + sizeof(struct tlvinfo_header) + total_length;
	u8 *ptr = eeprom_data + sizeof(struct tlvinfo_header);

	while (ptr < tlv_end) {
		struct tlvinfo_tlv *tlv = (struct tlvinfo_tlv *)ptr;

		if (tlv->type == tcode) {
			return tlv;
		}

		ptr += sizeof(struct tlvinfo_tlv) + tlv->length;
	}

	return NULL;
}

int mac_read_from_buffer(u8 *eeprom_data) {
	unsigned int i;
	struct tlvinfo_tlv *mac_size_tlv;
	struct tlvinfo_tlv *mac_base_tlv;
	int maccount;
	u8 macbase[6];
	struct tlvinfo_header *eeprom_hdr = (struct tlvinfo_header *)eeprom_data;

	pr_info("EEPROM: ");

	mac_size_tlv = find_tlv_in_buffer(eeprom_data, TLV_CODE_MAC_SIZE);
	maccount = 1;
	if (mac_size_tlv) {
		maccount = (mac_size_tlv->value[0] << 8) | mac_size_tlv->value[1];
	}

	mac_base_tlv = find_tlv_in_buffer(eeprom_data, TLV_CODE_MAC_BASE);
	if (mac_base_tlv) {
		memcpy(macbase, mac_base_tlv->value, 6);
	} else {
		memset(macbase, 0, sizeof(macbase));
	}

	for (i = 0; i < maccount; i++) {
		if (is_valid_ethaddr(macbase)) {
			char ethaddr[18];
			char enetvar[11];

			sprintf(ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
				macbase[0], macbase[1], macbase[2],
				macbase[3], macbase[4], macbase[5]);
			sprintf(enetvar, i ? "eth%daddr" : "ethaddr", i);
			/* Only initialize environment variables that are blank
			 * (i.e. have not yet been set)
			 */
			if (!env_get(enetvar))
				env_set(enetvar, ethaddr);

			macbase[5]++;
			if (macbase[5] == 0) {
				macbase[4]++;
				if (macbase[4] == 0) {
					macbase[3]++;
					if (macbase[3] == 0) {
						macbase[0] = 0;
						macbase[1] = 0;
						macbase[2] = 0;
					}
				}
			}
		}
	}

	printf("%s v%u len=%u\n", eeprom_hdr->signature, eeprom_hdr->version,
	       be16_to_cpu(eeprom_hdr->totallen));

	return 0;
}

void set_env_ethaddr(u8 *eeprom_data) {
	int ethaddr_valid = 0, eth1addr_valid = 0;
	uint8_t mac_addr[6], mac1_addr[6];
	char cmd_str[128] = {0};

	/* Determine source of MAC address and attempt to read it */
	if (eeprom_data != NULL) {
		// Attempt to read MAC address from buffer

		if (mac_read_from_buffer(eeprom_data) < 0) {
			pr_err("Failed to set MAC addresses from EEPROM buffer.\n");
			return;
		}
	} else {
		// Attempt to read MAC address from EEPROM
		if (mac_read_from_eeprom() < 0) {
			pr_err("Read MAC address from EEPROM failed!\n");
			return;
		}
	}

	/* check ethaddr valid */
	ethaddr_valid = eth_env_get_enetaddr("ethaddr", mac_addr);
	eth1addr_valid = eth_env_get_enetaddr("eth1addr", mac1_addr);
	if (ethaddr_valid && eth1addr_valid) {
		pr_info("valid ethaddr: %02x:%02x:%02x:%02x:%02x:%02x\n",
			mac_addr[0], mac_addr[1], mac_addr[2],
			mac_addr[3], mac_addr[4], mac_addr[5]);
		return ;
	}

	/*create random ethaddr*/
	net_random_ethaddr(mac_addr);
	mac_addr[0] = 0xfe;
	mac_addr[1] = 0xfe;
	mac_addr[2] = 0xfe;

	memcpy(mac1_addr, mac_addr, sizeof(mac1_addr));
	mac1_addr[5] = mac_addr[5] + 1;

	/* write to env ethaddr and eth1addr */
	eth_env_set_enetaddr("ethaddr", mac_addr);
	eth_env_set_enetaddr("eth1addr", mac1_addr);

	/*must read before set/write to eeprom using tlv_eeprom command*/
	run_command("tlv_eeprom", 0);

	/* save mac address to eeprom */
	snprintf(cmd_str, (sizeof(cmd_str) - 1), "tlv_eeprom set 0x24 %02x:%02x:%02x:%02x:%02x:%02x", \
			mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	run_command(cmd_str, 0);

	memset(cmd_str, 0, sizeof(cmd_str));
	snprintf(cmd_str, (sizeof(cmd_str) - 1), "tlv_eeprom set 0x2A 2");
	run_command(cmd_str, 0);

	memset(cmd_str, 0, sizeof(cmd_str));
	snprintf(cmd_str, (sizeof(cmd_str) - 1), "tlv_eeprom write");
	run_command(cmd_str, 0);
}

void set_dev_serial_no(uint8_t *eeprom_data)
{
	struct tlvinfo_tlv *tlv_entry = NULL;
	char *strval;

	// Decide where to read the serial number from
	if (eeprom_data != NULL) {
		tlv_entry = find_tlv_in_buffer(eeprom_data, TLV_CODE_SERIAL_NUMBER);
	} else {
		read_from_eeprom(&tlv_entry, TLV_CODE_SERIAL_NUMBER);
	}

	if (tlv_entry && (0 < tlv_entry->length) && (tlv_entry->length <= 32)) {
		pr_info("Serial number is valid.\n");
		strval = malloc(tlv_entry->length + 1);
		memcpy(strval, tlv_entry->value, tlv_entry->length);
		strval[tlv_entry->length] = 0;
		env_set("serial#", strval);
		free(strval);
	}
}

struct code_desc_info {
	u8	m_code;
	char	*m_name;
};

void refresh_config_info(u8 *eeprom_data)
{
	struct tlvinfo_tlv *tlv_info = NULL;
	char *strval;
	int i;

	const struct code_desc_info {
		u8    m_code;
		u8    is_data;
		char *m_name;
	} info[] = {
		{ TLV_CODE_PRODUCT_NAME,   false, "product_name"},
		{ TLV_CODE_PART_NUMBER,    false, "part#"},
		{ TLV_CODE_SERIAL_NUMBER,  false, "serial#"},
		{ TLV_CODE_MANUF_DATE,     false, "manufacture_date"},
		{ TLV_CODE_MANUF_NAME,     false, "manufacturer"},
		{ TLV_CODE_WIFI_MAC_ADDR,  false, "wifi_addr"},
		{ TLV_CODE_BLUETOOTH_ADDR, false, "bt_addr"},
		{ TLV_CODE_DEVICE_VERSION, true,  "device_version"},
		{ TLV_CODE_SDK_VERSION,    true,  "sdk_version"},
	};

	for (i = 0; i < ARRAY_SIZE(info); i++) {
		if (eeprom_data != NULL) {
			tlv_info = find_tlv_in_buffer(eeprom_data, info[i].m_code);
		} else {
			read_from_eeprom(&tlv_info, info[i].m_code);
		}

		if (tlv_info != NULL) {
			if (info[i].is_data) {
				// Convert the numeric value to string
				strval = malloc(64);
				int num = 0;
				for (int j = 0; j < tlv_info->length && j < sizeof(num); j++) {
					num = (num << 8) | tlv_info->value[j];
				}
				sprintf(strval, "%d", num);
			} else {
				// Copy the value directly as string
				strval = malloc(tlv_info->length + 1);
				memcpy(strval, tlv_info->value, tlv_info->length);
				strval[tlv_info->length] = '\0';
			}
			env_set(info[i].m_name, strval);
			free(strval);
		} else {
			pr_debug("Cannot find TLV data: %s\n", info[i].m_name);
		}
	}
}

static int probe_shutdown_charge(void)
{
#ifdef CONFIG_SPACEMIT_SHUTDOWN_CHARGE
	struct udevice *udev;
	int ret;

#ifdef CONFIG_TYPEC_HUSB239
	ret = uclass_get_device_by_driver(UCLASS_I2C_GENERIC, DM_DRIVER_GET(husb239), &udev);
	if (ret) {
		pr_err("Failed to probe HUSB239: %d\n", ret);
	}
#endif

	ret = uclass_get_device_by_driver(UCLASS_MISC, DM_DRIVER_GET(shutdown_charge), &udev);
	if (ret) {
		pr_info("Continue to boot\n");
	}
	return ret;
#else
	return 0;
#endif
}

int board_init(void)
{
#ifdef CONFIG_DM_REGULATOR_SPM8XX
	int ret;

	ret = regulators_enable_boot_on(true);
	if (ret)
		pr_debug("%s: Cannot enable boot on regulator\n", __func__);
#endif

	return 0;
}

int board_late_init(void)
{
	ulong kernel_start;
	ofnode chosen_node;
	char ram_size_str[16] = {"\0"};
	int ret;
	u8 *eeprom_data;
	struct tlvinfo_header *tlv_hdr = NULL;
	struct tlvinfo_tlv *first_entry = NULL;

	// save_ddr_training_info();

	// it MAY be NULL when did NOT load build-in env and eeprom is empty
	if (NULL == env_get("product_name"))
		env_set("product_name", DEFAULT_PRODUCT_NAME);

	eeprom_data = memalign(ARCH_DMA_MINALIGN, TLV_INFO_MAX_LEN);
	if (!eeprom_data) {
		pr_err("Failed to allocate memory for EEPROM data\n");
		return -ENOMEM;
	}
	if (read_tlvinfo_tlv_eeprom(eeprom_data, &tlv_hdr, &first_entry, 0) < 0) {
		pr_err("Failed to read all EEPROM data\n");
	}
	if (tlv_hdr != NULL && first_entry != NULL && is_valid_tlvinfo_header(tlv_hdr)) {
		set_env_ethaddr(eeprom_data);
		set_dev_serial_no(eeprom_data);
		refresh_config_info(eeprom_data);
	} else {
		set_env_ethaddr(NULL);
		set_dev_serial_no(NULL);
		refresh_config_info(NULL);
	}

	set_serialnumber_based_on_boot_mode();

#ifdef CONFIG_VIDEO_SPACEMIT
	ret = uclass_probe_all(UCLASS_VIDEO);
	if (ret) {
		pr_info("video devices not found or not probed yet: %d\n", ret);
	}
	ret = uclass_probe_all(UCLASS_DISPLAY);
	if (ret) {
		pr_info("display devices not found or not probed yet: %d\n", ret);
	}
#endif

#ifdef CONFIG_BUTTON
	ret = uclass_probe_all(UCLASS_BUTTON);
	if (ret) {
		pr_err("Failed to probe all buttons: %d\n", ret);
	} else {
		pr_info("All buttons probed successfully\n");
	}
#endif

	run_fastboot_command();

	run_cardfirmware_flash_command();

	run_net_flash_command();

	probe_shutdown_charge();

	ret = run_uboot_shell();
	if (!ret) {
		pr_info("reboot into uboot shell\n");
		return 0;
	}

	/*import env.txt from bootfs*/
	import_env_from_bootfs();

	if (!is_video_connected) {
		env_set("stdout", "serial");
	}

	setenv_boot_mode();

	/*save ram size to env, transfer to MB*/
	sprintf(ram_size_str, "mem=%dMB", (int)(gd->ram_size / SZ_1MB));
	env_set("ram_size", ram_size_str);

	chosen_node = ofnode_path("/chosen");
	if (!ofnode_valid(chosen_node)) {
		pr_debug("No chosen node found, can't get kernel start address\n");
		return 0;
	}

	ret = ofnode_read_u64(chosen_node, "riscv,kernel-start",
				  (u64 *)&kernel_start);
	if (ret) {
		pr_debug("Can't find kernel start address in device tree\n");
		return 0;
	}

	env_set_hex("kernel_start", kernel_start);

	return 0;
}

void *board_fdt_blob_setup(int *err)
{
	*err = 0;

	/* Stored the DTB address there during our init */
	if (IS_ENABLED(CONFIG_OF_SEPARATE) || IS_ENABLED(CONFIG_OF_BOARD)) {
		if (gd->arch.firmware_fdt_addr){
			if (!fdt_check_header((void *)(ulong)gd->arch.firmware_fdt_addr)){
				return (void *)(ulong)gd->arch.firmware_fdt_addr;
			}
		}
	}
	return (ulong *)&_end;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	if (prio >= 1)
		return ENVL_UNKNOWN;

	u32 boot_mode = get_boot_mode();
	switch (boot_mode) {
#ifdef CONFIG_ENV_IS_IN_MTD
	case BOOT_MODE_NAND:
		return ENVL_MTD;
#endif
#ifdef CONFIG_ENV_IS_IN_NAND
	case BOOT_MODE_NAND:
		return ENVL_NAND;
#endif
#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
	case BOOT_MODE_NOR:
		return ENVL_SPI_FLASH;
#endif
#ifdef CONFIG_ENV_IS_IN_MMC
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
		return ENVL_MMC;
#endif
	default:
#ifdef CONFIG_ENV_IS_NOWHERE
		return ENVL_NOWHERE;
#else
		return ENVL_UNKNOWN;
#endif
	}
}

int misc_init_r(void)
{
	return 0;
}

int dram_init(void)
{
	get_ddr_config_info();
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;

	gd->ram_base = CONFIG_SYS_SDRAM_BASE;
	gd->ram_size = dram_size;

	return 0;
}

int dram_init_banksize(void)
{
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;

	memset(gd->bd->bi_dram, 0, sizeof(gd->bd->bi_dram));
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	if(dram_size > SZ_2GB) {
		gd->bd->bi_dram[0].size = SZ_2G;
		if (CONFIG_NR_DRAM_BANKS > 1) {
			gd->bd->bi_dram[1].start = 0x100000000;
			gd->bd->bi_dram[1].size = dram_size - SZ_2G;
		}
	} else {
		gd->bd->bi_dram[0].size = dram_size;
	}

	return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
	u64 dram_size = (u64)ddr_get_density() * SZ_1MB;

		/* Some devices (like the EMAC) have a 32-bit DMA limit. */
	if(dram_size > SZ_2GB) {
		return 0x80000000;
	} else {
		return dram_size;
	}
}

#if !defined(CONFIG_SPL_BUILD)
int board_fit_config_name_match(const char *name)
{
	char tmp_name[64];
	char *product_name = env_get("product_name");

	/*
		be compatible to previous format name,
		such as: k1_deb1 -> k1-x_deb1
	*/
	if (!strncmp(product_name, "k1_", 3)){
		sprintf(tmp_name, "%s_%s", "k1-x", &product_name[3]);
		product_name = tmp_name;
	}

	if ((NULL != product_name) && (0 == strcmp(product_name, name))) {
		log_emerg("Boot from fit configuration %s\n", name);
		return 0;
	}
	else
		return -1;
}
#endif

static uint32_t get_dro_from_efuse(void)
{
	struct udevice *dev;
	uint8_t fuses[2];
	uint32_t dro = SVT_DRO_DEFAULT_VALUE;
	int ret;

	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
			DM_DRIVER_GET(spacemit_k1x_efuse), &dev);
	if (ret) {
		return dro;
	}

	// read from efuse, each bank has 32byte efuse data
	// SVT-DRO in bank7 bit173~bit181
	ret = misc_read(dev, 7 * 32 + 21, fuses, sizeof(fuses));
	if (0 == ret) {
		// (byte1 bit0~bit5) << 3 | (byte0 bit5~7) >> 5
		dro = (fuses[0] >> 5) & 0x07;
		dro |= (fuses[1] & 0x3F) << 3;
	}

	if (0 == dro)
		dro = SVT_DRO_DEFAULT_VALUE;
	return dro;
}

static int get_chipinfo_from_efuse(uint32_t *product_id, uint32_t *wafer_tid)
{
	struct udevice *dev;
	uint8_t fuses[3];
	int ret;

	*product_id = 0;
	*wafer_tid = 0;
	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
			DM_DRIVER_GET(spacemit_k1x_efuse), &dev);
	if (ret) {
		return ENODEV;
	}

	// read from efuse, each bank has 32byte efuse data
	// product id in bank7 bit182~bit190
	ret = misc_read(dev, 7 * 32 + 22, fuses, sizeof(fuses));
	if (0 == ret) {
		// (byte1 bit0~bit6) << 2 | (byte0 bit6~7) >> 6
		*product_id = (fuses[0] >> 6) & 0x03;
		*product_id |= (fuses[1] & 0x7F) << 2;
	}

	// read from efuse, each bank has 32byte efuse data
	// product id in bank7 bit139~bit154
	ret = misc_read(dev, 7 * 32 + 17, fuses, sizeof(fuses));
	if (0 == ret) {
		// (byte1 bit0~bit6) << 2 | (byte0 bit3~7) >> 3
		*wafer_tid = (fuses[0] >> 3) & 0x1F;
		*wafer_tid |= fuses[1] << 5;
		*wafer_tid |= (fuses[2] & 0x07) << 13;
	}

	return 0;
}

static int ft_board_cpu_fixup(void *blob, struct bd_info *bd)
{
	int node, ret;
	uint32_t dro, product_id, wafer_tid;

	node = fdt_path_offset(blob, "/");
	if (node < 0) {
		pr_err("Can't find root node!\n");
		return -EINVAL;
	}

	get_chipinfo_from_efuse(&product_id, &wafer_tid);
	product_id = cpu_to_fdt32(product_id);
	wafer_tid = cpu_to_fdt32(wafer_tid);
	fdt_setprop(blob, node, "product-id", &product_id, sizeof(product_id));
	fdt_setprop(blob, node, "wafer-id", &wafer_tid, sizeof(wafer_tid));

	node = fdt_path_offset(blob, "/cpus");
	if (node < 0) {
		pr_err("Can't find cpus node!\n");
		return -EINVAL;
	}

	dro = cpu_to_fdt32(get_dro_from_efuse());
	ret = fdt_setprop(blob, node, "svt-dro", &dro, sizeof(dro));
	if (ret < 0)
		return ret;
	return 0;
}

static int ft_board_info_fixup(void *blob, struct bd_info *bd)
{
	int node;
	const char *part_number;

	node = fdt_path_offset(blob, "/");
	if (node < 0) {
		pr_err("Can't find root node!\n");
		return -EINVAL;
	}

	part_number = env_get("part#");
	if (NULL != part_number)
		fdt_setprop(blob, node, "part-number", part_number, strlen(part_number));

	return 0;
}

static int ft_board_mac_addr_fixup(void *blob, struct bd_info *bd)
{
	int node, i;
	const char *addr_value;
	// char addr_str[ARP_HLEN_ASCII + 1];
	const char *mac_item[] = {"wifi_addr", "bt_addr"};

	node = fdt_path_offset(blob, "/soc");
	if (node < 0) {
		pr_err("Can't find soc node!\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(mac_item); i++) {
		addr_value = env_get(mac_item[i]);
		if (NULL != addr_value) {
			// memset(addr_str, 0, sizeof(addr_str));
			// sprintf(addr_str, "%pM", addr_value);
			fdt_setprop(blob, node, mac_item[i], addr_value, strlen(addr_value));
		}
	}

	return 0;
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	struct fdt_memory mem;
	static const struct node_info nodes[] = {
		{ "spacemit,k1x-qspi", MTD_DEV_TYPE_NOR, },  /* SPI flash */
	};

	/* update MTD partition info for nor boot */
	if (CONFIG_IS_ENABLED(FDT_FIXUP_PARTITIONS) &&
		BOOT_MODE_NOR == get_boot_mode())
		fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));

	if (CONFIG_IS_ENABLED(FDT_SIMPLEFB)) {
		/* reserved with no-map tag the video buffer */
		mem.start = gd->video_bottom;
		mem.end = gd->video_top - 1;

		fdtdec_add_reserved_memory(blob, "framebuffer", &mem, NULL, 0, NULL, 0);
	}

	ft_board_cpu_fixup(blob, bd);
	ft_board_info_fixup(blob, bd);
	ft_board_mac_addr_fixup(blob, bd);
	return 0;
}

static bool has_bootarg(const char *args, const char *param, size_t param_len)
{
	const char *p = args;

	if (!args || !param || !param_len)
		return false;

	// Iterate through all parameters in args
	while (*p) {
		// Skip spaces
		while (*p == ' ')
			p++;
		if (!*p)
			break;

		// Check if current parameter matches
		if (strncmp(p, param, param_len) == 0 &&
			(p[param_len] == '\0' || p[param_len] == ' ' || p[param_len] == '=')) {
			return true;
		}

		// Move to next parameter
		while (*p && *p != ' ')
			p++;
	}
	return false;
}

char *board_fdt_chosen_bootargs(void)
{
	const void *fdt;
	const char *env_args = env_get("bootargs");
	const char *dts_args = NULL;
	char *merged = NULL;
	int nodeoffset;

	fdt = (void *)env_get_hex("fdt_addr", 0);
	if (!fdt) {
		return (char *)env_args;
	}

	if (fdt_check_header(fdt)) {
		pr_err("Invalid kernel DTB\n");
		return (char *)env_args;
	}

	nodeoffset = fdt_path_offset(fdt, "/chosen");
	if (nodeoffset >= 0)
		dts_args = fdt_getprop(fdt, nodeoffset, "bootargs", NULL);

	// Print env bootargs
	pr_debug("Env bootargs:\n    %s\n", env_args ? env_args : "NULL");
	// Print DTS bootargs
	pr_debug("DTS bootargs:\n    %s\n", dts_args ? dts_args : "NULL");

	if (!dts_args)
		return (char *)env_args;

	size_t total_len = 1;
	if (env_args)
		total_len += strlen(env_args);
	if (dts_args)
		total_len += strlen(dts_args) + 1;

	merged = calloc(1, total_len);
	if (!merged) {
		pr_err("Memory allocation failed\n");
		return NULL;
	}

	if (env_args)
		strcpy(merged, env_args);

	const char *p = dts_args;
	bool need_space = (merged[0] != '\0');

	while (p && *p) {
		while (*p && *p == ' ')
			p++;
		if (!*p)
			break;

		const char *end = p;
		while (*end && *end != ' ')
			end++;

		size_t param_len;
		const char *eq = memchr(p, '=', end - p);
		param_len = eq ? (size_t)(eq - p) : (size_t)(end - p);

		if (!has_bootarg(env_args, p, param_len)) {
			if (need_space)
				strcat(merged, " ");
			strncat(merged, p, end - p);
			need_space = true;
		}

		p = end;
	}

	if (!merged[0]) {
		free(merged);
		merged = NULL;
	}

	return merged;
}
