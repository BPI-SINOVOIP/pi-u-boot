// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <blk.h>
#include <bootstage.h>
#include <command.h>
#include <common.h>
#include <console.h>
#include <div64.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <fs.h>
#include <image.h>
#include <malloc.h>
#include <mapmem.h>
#include <memalign.h>
#include <mmc.h>
#include <part.h>
#include <u-boot/crc.h>
#include <usb.h>
#include <fb_spacemit.h>
#include <cJSON.h>
#include <env.h>
#include <mtd.h>
#include <fb_mtd.h>
#include <nvme.h>

static int dev_emmc_num = -1;
static int dev_sdio_num = -1;
static u32 bootfs_part_index = 0;

void recovery_show_result(struct flash_dev *fdev, int ret);

static void free_flash_dev(struct flash_dev *fdev)
{
	for (int i = 0; i < MAX_PARTITION_NUM; i++){
		if (fdev->parts_info[i].part_name != NULL){
			free(fdev->parts_info[i].part_name);
			free(fdev->parts_info[i].file_name);
			free(fdev->parts_info[i].size);
		}else{
			break;
		}
	}
	free(fdev->gptinfo.gpt_table);
	free(fdev->mtd_table);
	free(fdev);
}


static int _write_gpt_partition(struct flash_dev *fdev)
{
	__maybe_unused char write_part_command[300] = {"\0"};
	char *gpt_table_str = NULL;

	u32 boot_mode = get_boot_pin_select();

	if (fdev->gptinfo.gpt_table != NULL && strlen(fdev->gptinfo.gpt_table) > 0){
		gpt_table_str = malloc(strlen(fdev->gptinfo.gpt_table) + 32);
		if (gpt_table_str == NULL){
			return -1;
		}
		sprintf(gpt_table_str, "env set -f partitions '%s'", fdev->gptinfo.gpt_table);
		run_command(gpt_table_str, 0);
		free(gpt_table_str);
	}

	switch(boot_mode){
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
		sprintf(write_part_command, "gpt write mmc %x '%s'",
			CONFIG_FASTBOOT_FLASH_MMC_DEV, fdev->gptinfo.gpt_table);
		if (run_command(write_part_command, 0)){
			printf("write gpt fail\n");
			return -1;
		}
		break;
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_SUPPORT_BLOCK_DEV)
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
		printf("write gpt to dev:%s\n", CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME);

		/*nvme need scan at first*/
		if (!strncmp("nvme", CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME, 4)
						&& nvme_scan_namespace()){
			printf("can not can nvme devices!\n");
			return -1;
		}

		sprintf(write_part_command, "gpt write %s %x '%s'",
			CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME, CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_INDEX,
			fdev->gptinfo.gpt_table);
		if (run_command(write_part_command, 0)){
			printf("write gpt fail\n");
			return -1;
		}
		break;
#endif
	default:
		break;
	}
	return 0;
}


static int _write_mtd_partition(char mtd_table[128])
{
#ifdef CONFIG_MTD
	struct mtd_info *mtd;
	char mtd_ids[36] = {"\0"};
	char mtd_parts[128] = {"\0"};

	mtd_probe_devices();

	/*
	try to find the first mtd device, it there have mutil mtd device such as nand and nor,
	it only use the first one.
	*/
	mtd_for_each_device(mtd) {
		if (!mtd_is_partition(mtd))
			break;
	}

	if (mtd == NULL){
		printf("can not get mtd device\n");
		return -1;
	}

	/*to mtd device, it should write mtd table to env.*/
	sprintf(mtd_ids, "%s=spi-dev", mtd->name);
	sprintf(mtd_parts, "spi-dev:%s", mtd_table);

	env_set("mtdids", mtd_ids);
	env_set("mtdparts", mtd_parts);
#endif
	return 0;
}

/* Initialize the mmc device given its number */
static int init_mmc_device(int dev_num)
{
	struct mmc *mmc = find_mmc_device(dev_num);

	if (!mmc) {
		debug("Cannot find mmc device %d\n", dev_num);
		return RESULT_FAIL;
	}

	if (mmc_init(mmc)) {
		debug("mmc init failed for device %d\n", dev_num);
		return RESULT_FAIL;
	}
	return RESULT_OK;
}

/* Detect and classify mmc device */
static void detect_and_classify_mmc(int dev_num)
{
	int current_dev_num, err;
	struct disk_partition info;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return;

	current_dev_num = mmc_get_blk_desc(mmc)->devnum;
	if (IS_SD(mmc)) {
		dev_sdio_num = current_dev_num;
		for (u32 p = 1; p <= MAX_SEARCH_PARTITIONS; p++) {
			err = part_get_info(mmc_get_blk_desc(mmc), p, &info);
			if (err)
				continue;
			if (!strcmp(FLASH_IMG_PARTNAME, info.name)){
				debug("match info.name:%s\n", info.name);
				bootfs_part_index = p;
				break;
			}
		}
		debug("SDIO detected with number: %d\n", dev_sdio_num);
	} else {
		dev_emmc_num = current_dev_num;
		debug("eMMC initialized with number: %d\n", dev_emmc_num);
	}
}


int check_mmc_exist_and_initialize(void)
{
	int mmc_dev_num = get_mmc_num();

	for (int i = 0; i < mmc_dev_num; i++) {
		if (init_mmc_device(i) == RESULT_OK) {
			detect_and_classify_mmc(i);
		}
	}

	if (dev_sdio_num == -1) {
		printf("SDIO not detected.\n");
		return RESULT_FAIL;
	}
	return RESULT_OK;
}

int download_file_via_tftp(char *file_name, char *load_addr) {
	char full_path[128];
	char cmd_buffer[256];
	char *tftp_server_ip;
	char *tftp_path_prefix;

	tftp_server_ip = env_get("serverip");
	if (!tftp_server_ip) {
		printf("Error: TFTP server IP not set\n");
		return -1;
	}

	tftp_path_prefix = env_get("net_data_path");
	if (!tftp_path_prefix) {
		printf("Error: TFTP relative path not set\n");
		return -1;
	}

	sprintf(full_path, "%s%s", tftp_path_prefix, file_name);
	sprintf(cmd_buffer, "tftpboot %s %s:%s", load_addr, tftp_server_ip, full_path);

	if (run_command(cmd_buffer, 0)) {
		printf("Error: TFTP download failed\n");
		return -1;
	}
	return 0;
}

static int _find_partition_file(struct flash_dev *fdev, char *tmp_file, char *temp_fname, u32 temp_fname_size)
{
	if (strlen(FLASH_IMG_FOLDER) > 0){
		strcpy(temp_fname, FLASH_IMG_FOLDER);
		strcat(temp_fname, "/");
		strcat(temp_fname, tmp_file);
	}else{
		strcpy(temp_fname, tmp_file);
	}
	if (!run_commandf("fatsize %s %d:%d %s", fdev->device_name, fdev->dev_index,
										bootfs_part_index, temp_fname)){
		/*has find partition file name*/
		return 0;
	}else{
		memset(tmp_file, '\0', 30);
		memset(temp_fname, '\0', temp_fname_size);
	}
	return -1;
}

static int find_mtd_partition_file(struct flash_dev *fdev, char *temp_fname, u32 temp_fname_size)
{
	char tmp_file[30] = {"\0"};
	u32 mtd_size = fdev->mtdinfo.size;
	bool nand_flag = false;
	memset(temp_fname, '\0', temp_fname_size);

	switch (fdev->mtdinfo.size_type) {
	case MTD_SIZE_G:
		while (mtd_size/2){
			mtd_size /= 2;
			sprintf(tmp_file, "partition_%dG.json", mtd_size);
			if (!_find_partition_file(fdev, tmp_file, temp_fname, temp_fname_size))
				return 0;
		}

		/*retry to find until 64M*/
		mtd_size = 1024;

		/*if can not find at type G, try to find type M, should not break*/
		/*break;*/
	case MTD_SIZE_M:
		if (mtd_size >= 64)
			nand_flag = true;

		while (mtd_size/2){
			mtd_size /= 2;
			if (nand_flag && mtd_size < 64){
				pr_err("can not find suitable nand partition file\n");
				return -1;
			}
			sprintf(tmp_file, "partition_%dM.json", mtd_size);
			if (!_find_partition_file(fdev, tmp_file, temp_fname, temp_fname_size))
				return 0;
		}

		/*retry to find type K patition file*/
		mtd_size = 1024;
		/*break;*/
	case MTD_SIZE_K:
		while (mtd_size/2){
			mtd_size /= 2;
			sprintf(tmp_file, "partition_%dK.json", mtd_size);
			if (!_find_partition_file(fdev, tmp_file, temp_fname, temp_fname_size))
				return 0;
		}

	default:
		pr_err("undefine mtd size type, return fail\n");
		return -1;
	}
}


static int load_from_device(struct cmd_tbl *cmdtp, char *load_str,
			int device_type, struct flash_dev *fdev)
{
	int retval = RESULT_OK;
	char blk_dev_str[10] = {"\0"};

	switch (device_type) {
#ifdef CONFIG_MMC
	case DEVICE_MMC:
		if (check_mmc_exist_and_initialize() != RESULT_OK) {
			retval = RESULT_FAIL;
			break;
		}
		fdev->dev_index = dev_sdio_num;
		fdev->device_name = strdup("mmc");
		break;
#endif //CONFIG_MMC

#ifdef CONFIG_USB_STORAGE
	case DEVICE_USB:
		static bool usb_init_flag = false;
		if (!usb_init_flag){
			usb_init();
			int device_number = usb_stor_scan(1);
			if (device_number < 0){
				printf("No USB storage devices found.\n");
				retval = RESULT_FAIL;
				break;
			}
			fdev->dev_index = device_number;
			fdev->device_name = strdup("usb");
			usb_init_flag = true;

			char cmd[128];
			for (u32 p = 1; p <= MAX_SEARCH_PARTITIONS; p++){
				sprintf(cmd, "fatls usb %d:%d", device_number, p);
				if (!run_command(cmd, 0))
				{
					bootfs_part_index = p;
					break;
				}
			}

			if (bootfs_part_index == 0){
				printf("No valid filesystem found in any partition on USB.\n");
				retval = RESULT_FAIL;
				break;
			}
		}
		break;
#else
		printf("USB storage support is not enabled.\n");
		retval = RESULT_FAIL;
		break;
#endif //CONFIG_USB_STORAGE

#ifdef CONFIG_CMD_TFTPBOOT
	case DEVICE_NET:
		if (run_command("dhcp", 0)) {
			printf("Error: DHCP request failed\n");
			retval = RESULT_FAIL;
			break;
		}

		fdev->device_name = strdup("net");
		break;
#endif //CONFIG_CMD_TFTPBOOT

	default:
		printf("Unknown device type!\n");
		retval = RESULT_FAIL;
		break;
	}

	/* If the above operation fails, return early */
	if (retval != RESULT_OK) {
		return retval;
	}

	debug("device_name: %s\n", fdev->device_name);
	debug("dev_index: %d\n", fdev->dev_index);

	u32 temp_fname_size = strlen(fdev->partition_file_name) + strlen(FLASH_IMG_FOLDER) + 2;
	char *temp_fname = malloc(temp_fname_size);
	if (!temp_fname){
		printf("malloc file_name fail\n");
		return RESULT_FAIL;
	}
	memset(temp_fname, '\0', temp_fname_size);
	if (strlen(FLASH_IMG_FOLDER) > 0){
		strcpy(temp_fname, FLASH_IMG_FOLDER);
		strcat(temp_fname, "/");
		strcat(temp_fname, fdev->partition_file_name);
	}else{
		strcpy(temp_fname, fdev->partition_file_name);
	}

	if (strcmp(fdev->device_name, "mmc") == 0 || strcmp(fdev->device_name, "usb") == 0) {

		/*would try to detect mtd partition file exists or not*/
		if (strcmp(FLASH_CONFIG_FILE_NAME, fdev->partition_file_name) &&
			run_commandf("fatsize %s %d:%d %s", fdev->device_name, fdev->dev_index,
                                                bootfs_part_index, temp_fname)) {
			/*can not find mtd partition file, would try to find suitable file*/
			if (find_mtd_partition_file(fdev, temp_fname, temp_fname_size)){
				pr_err("can not find suitable partition file\n");
				recovery_show_result(fdev, RESULT_FAIL);
			}
			printf("find temp_fname:%s\n", temp_fname);
		}

		sprintf(blk_dev_str, "%d:%d", fdev->dev_index, bootfs_part_index);
		char *fat_argv[] = {"fatload", fdev->device_name, blk_dev_str, load_str, temp_fname};

		if (do_load(cmdtp, 0, 5, fat_argv, FS_TYPE_FAT)) {
			printf("do_load flash_config from %s failed\n", fdev->device_name);
			retval = RESULT_FAIL;
		} else {
			printf("do_load flash_config %s success\n", fdev->device_name);
		}
	} else if (strcmp(fdev->device_name, "net") == 0) {

		if (download_file_via_tftp(temp_fname, load_str) < 0) {
			printf("Failed to download file via TFTP\n");
			retval = RESULT_FAIL;
		} else {
			printf("Downloaded file via TFTP successfully\n");
		}
	}

	free(temp_fname);
	return retval;
}

void recovery_show_result(struct flash_dev *fdev, int ret)
{
	if (ret) {
		printf("!!!!!!!!!!!!!!!!!!! recovery flash false !!!!!!!!!!!!!!!!!!!\n");
	} else {
		printf("################### recovery flash success ###################\n");
	}

	/*free the malloc paramenter*/
	free_flash_dev(fdev);

	while(1){
		/* do not retrun while flashing over! */
	}

}

int get_part_info(struct blk_desc *dev_desc, const char *name,
		struct disk_partition *info)
{
	int ret;

	if (dev_desc) {
		ret = part_get_info_by_name(dev_desc, name, info);
		if (ret >= 0)
			return ret;
	}

	printf("%s, can not find part info\n", __func__);
	return -1;
}


/**
 * mmc_blk_write() - Write/erase MMC in chunks of MAX_BLK_WRITE
 *
 * @block_dev: Pointer to block device
 * @start: First block to write/erase
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for write or NULL for erase
 */
static __maybe_unused lbaint_t mmc_blk_write(struct blk_desc *block_dev, lbaint_t start,
			lbaint_t blkcnt, const void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_written;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int i;

	for (i = 0; i < blkcnt; i += MAX_BLK_WRITE) {
		cur_blkcnt = min((int)blkcnt - i, MAX_BLK_WRITE);
		if (buffer) {
			blks_written = blk_dwrite(block_dev, blk, cur_blkcnt,
					buffer + (i * block_dev->blksz));
		} else {
			blks_written = blk_derase(block_dev, blk, cur_blkcnt);
		}
		blk += blks_written;
		blks += blks_written;
	}
	return blks;
}


int blk_write_raw_image(struct blk_desc *dev_desc,
		struct disk_partition *info, const char *part_name,
		void *buffer, u32 download_bytes)
{
#ifdef CONFIG_MMC
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = lldiv(blkcnt, info->blksz);

	if (blkcnt > info->size) {
		printf("too large for partition: '%s'\n", part_name);
		return RESULT_FAIL;
	}

	puts("Flashing Raw Image\n");

	blks = mmc_blk_write(dev_desc, info->start, blkcnt, buffer);

	if (blks != blkcnt) {
		printf("failed writing to device %d\n", dev_desc->devnum);
		return RESULT_FAIL;
	}

	printf("........ wrote " LBAFU " bytes to '%s'\n", \
		blkcnt * info->blksz,part_name);
	return RESULT_OK;
#else
	printf("not mmc dev found\n");
	return RESULT_FAIL;
#endif
}

int mtd_write_raw_image(struct mtd_info *mtd, const char *part_name, void *buffer, u32 download_bytes)
{
	int ret;

	printf("Erasing MTD partition %s\n", part_name);
	ret = _fb_mtd_erase(mtd, download_bytes);
	if (ret) {
		printf("failed erasing from device %s\n", mtd->name);
		return RESULT_FAIL;
	}

	ret = _fb_mtd_write(mtd, buffer, 0, download_bytes, NULL);
	if (ret < 0) {
		printf("Failed to write mtd part:%s\n", part_name);
		return RESULT_FAIL;
	}

	return 0;
}

void specific_flash_mmc_opt(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	char blk_dev_str[20] = {"\0"};
	char file_name[50] = {"\0"};
	u32 image_size = 0;
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	sprintf(blk_dev_str, "%d:%d", fdev->dev_index, bootfs_part_index);

	/*flash emmc info to boot0*/
	fastboot_oem_flash_bootinfo(NULL, load_addr, image_size, NULL, fdev);

	/*load fsbl.bin to load_addr*/
	if (strlen(FLASH_IMG_FACTORY_FOLDER) > 0){
		strcpy(file_name, FLASH_IMG_FACTORY_FOLDER);
		strcat(file_name, "/");
		strcat(file_name, "FSBL.bin");
	}else{
		strcpy(file_name, "FSBL.bin");
	}

	struct blk_desc *dev_desc = blk_get_dev("mmc",
					   CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (strcmp(fdev->device_name, "net") == 0) {
		if (download_file_via_tftp(file_name, simple_xtoa((ulong)load_addr)) < 0) {
			printf("Failed to download file via TFTP\n");
			return;
		}
		image_size = env_get_hex("filesize", 0);
	} else {
		char *const argv_image[] = {"fatload", fdev->device_name, blk_dev_str, simple_xtoa((ulong)load_addr), file_name};

		if (do_load(cmdtp, 0, 5, argv_image, FS_TYPE_FAT)) {
			printf("Cannot load file %s\n", file_name);
			return;
		}
		image_size = env_get_hex("filesize", 0);
	}

	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return;
	}

	/*flash fsbl.bin to boot0*/
	if (flash_mmc_boot_op(dev_desc, load_addr, 1, image_size, BOOT_INFO_EMMC_SPL0_OFFSET)){
		printf("flash fsbl fail\n");
		return;
	}

	/*flash fsbl.bin to boot1*/
	if (flash_mmc_boot_op(dev_desc, load_addr, 2, image_size, BOOT_INFO_EMMC_SPL1_OFFSET)){
		printf("flash fsbl fail\n");
		return;
	}
#endif
}

int load_and_flash_file(struct cmd_tbl *cmdtp, struct flash_dev *fdev, char *file_name, char *partition, uint64_t *partition_offset)
{
	char load_str[20];
	char addr_str[20];
	char offset_str[20];
	char blk_dev_str[16];
	struct disk_partition info = {0};
	struct mtd_info *mtd = NULL;
	struct part_info *mtd_part_info = NULL;
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	lbaint_t part_start_addr;
	uint64_t image_size = 0;
	uint64_t byte_remain = 0;
	uint64_t download_offset, download_bytes, bytes_read;
	u64 compare_value = 0;
	int div_times, data_source;

	memset(load_str, 0, sizeof(load_str));
	memset(offset_str, 0, sizeof(offset_str));
	strcpy(load_str, simple_xtoa((ulong)load_addr));
	sprintf(blk_dev_str, "%d:%d", fdev->dev_index, bootfs_part_index);

	if (strcmp(fdev->device_name, "mmc") == 0 || strcmp(fdev->device_name, "usb") == 0) {
		// load data from fat disk
		data_source = 0;
		char *const argv_image_size[] = {"fatsize", fdev->device_name, blk_dev_str, file_name};
		if (do_size(cmdtp, 0, 4, argv_image_size, FS_TYPE_FAT)) {
			printf("can not find file :%s, \n", file_name);
			return RESULT_FAIL;
		}

		image_size = env_get_hex("filesize", 0);
		byte_remain = image_size;
		div_times = (image_size + RECOVERY_LOAD_IMG_SIZE - 1) / RECOVERY_LOAD_IMG_SIZE;
		pr_info("\n\ndev_times:%d\n", div_times);
	} else if (strcmp(fdev->device_name, "net") == 0) {
		// load data from net with tftp
		data_source = 1;
		/* Temporarily, the logic for network fragment download has not been added,
		so set the number of downloads to 1 and directly download the entire file. */
		div_times = 1;
	} else {
		printf("NOT support data source %s\n", fdev->device_name);
		return RESULT_FAIL;
	}

	if (fdev->blk_write != NULL && get_part_info(fdev->dev_desc, partition, &info) < 0) {
		printf("can not get part %s in gpt tabel\n", partition);
		return RESULT_FAIL;
	}
	if(fdev->mtd_write != NULL){
		/*init mtd info*/
		if(fb_mtd_lookup(partition, &mtd, &mtd_part_info)){
			printf("invalid mtd device \n");
			return RESULT_FAIL;
		}
		/*update info to mtd dev*/
		info.start = 0;
		info.blksz = 1;
	}

	download_offset = 0;
	compare_value = 0;
	info.start += *partition_offset;

	/* save the partition start cnt */
	part_start_addr = info.start;
	for (int j = 0; j < div_times; j++) {
		debug("\nflash data count %d\n", j);
		if (0 == data_source) {
			download_bytes = byte_remain > RECOVERY_LOAD_IMG_SIZE ? RECOVERY_LOAD_IMG_SIZE : byte_remain;
			strcpy(addr_str, simple_xtoa((ulong)download_bytes));
			strcpy(offset_str, simple_xtoa((ulong)download_offset));

			char *const argv_image[] = {"fatload", fdev->device_name, blk_dev_str,
										load_str, file_name, addr_str, offset_str};
			printf("load from %llx, bytes:%llx\n", download_offset, download_bytes);
			if (do_load(cmdtp, 0, 7, argv_image, FS_TYPE_FAT))
				return RESULT_FAIL;

			bytes_read = env_get_hex("filesize", 0);
			printf("read data size %lld\n", bytes_read);
			if (bytes_read != download_bytes) {
				printf("download file size is not equal require\n");
				return RESULT_FAIL;
			}
		} else {
			if (download_file_via_tftp(file_name, load_str) < 0) {
				printf("Failed to download file via TFTP\n");
				return RESULT_FAIL;
			}
			image_size = download_bytes = env_get_hex("filesize", 0);
		}

		// compare_value = crc32_wd(compare_value, (const uchar *)load_addr, download_bytes, CHUNKSZ_CRC32);
		compare_value += checksum64(load_addr, download_bytes);
		info.size = (download_bytes + (info.blksz - 1)) / info.blksz;
		printf("write storage at block: 0x%lx, size: %lx\n", info.start, info.size);

		if (fdev->blk_write != NULL){
			if (fdev->blk_write(fdev->dev_desc, &info, partition, load_addr, download_bytes)){
				return RESULT_FAIL;
			}
		}else{
			/*write to mtd dev*/
			if (fdev->mtd_write(mtd, partition, load_addr, download_bytes))
				return RESULT_FAIL;
		}

		info.start += info.size;
		*partition_offset += info.size;
		download_offset += download_bytes;
		byte_remain -= download_bytes;
	}

	/* read from device and check crc */
	debug("check crc, read %lx, imagesize:%lld\n", part_start_addr, image_size);
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)

	if (fdev->blk_write){
		if (compare_blk_image_val(fdev->dev_desc, compare_value, part_start_addr, info.blksz, image_size)) {
			printf("check image crc32 fail, \n");
			return RESULT_FAIL;
		}
	}else{
		if (compare_mtd_image_val(mtd, compare_value, image_size)) {
			printf("check image crc32 fail, \n");
			return RESULT_FAIL;
		}
	}
#endif
	return RESULT_OK;
}

int flash_volume_from_file(struct cmd_tbl *cmdtp, struct flash_dev *fdev, const char *volume_name, const char *file_name, const char *partition, uint64_t *partition_offset) {
	char blk_dev_str[32];
	uint64_t image_size = 0;
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
    char cmd_buf[256];

	sprintf(blk_dev_str, "%d:%d", fdev->dev_index, bootfs_part_index);

	sprintf(cmd_buf, "fatload %s %s %lx %s", fdev->device_name, blk_dev_str, (ulong)load_addr, file_name);
	if (run_command(cmd_buf, 0) != 0) {
		printf("Failed to load file %s\n", file_name);
		return RESULT_FAIL;
	}

	image_size = env_get_hex("filesize", 0);
	printf("Loaded %s, size: %llu bytes\n", file_name, image_size);

	printf("Creating and writing to UBI volume: %s\n", volume_name);

	sprintf(cmd_buf, "ubi part %s", partition);
	if (run_command(cmd_buf, 0) != 0) {
		printf("Failed to select MTD partition %s\n", partition);
		return RESULT_FAIL;
	}

	sprintf(cmd_buf, "ubi check %s", volume_name);
	if (run_command(cmd_buf, 0) != 0) {
		sprintf(cmd_buf, "ubi create %s %llx dynamic", volume_name, image_size);
		if (run_command(cmd_buf, 0) != 0) {
			printf("Failed to create UBI volume %s\n", volume_name);
			return RESULT_FAIL;
		}
	}

	sprintf(cmd_buf, "ubi write %lx %s %llx", (ulong)load_addr, volume_name, image_size);
	if (run_command(cmd_buf, 0) != 0) {
		printf("Failed to write to UBI volume %s\n", volume_name);
		return RESULT_FAIL;
	}

	return RESULT_OK;
}

static int flash_image(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
	int ret = RESULT_OK, i, j;
	uint64_t time_start_flash, partition_offset;
	char *part_name, *file_name;
	char blk_dev_str[16], *split_file_name, *name, *extension;

	for (i = 0; i < MAX_PARTITION_NUM; i++) {
		part_name = fdev->parts_info[i].part_name;
		file_name = fdev->parts_info[i].file_name;
		time_start_flash = get_timer(0);

		if (part_name == NULL || strlen(part_name) == 0) {
			printf("no more partition to flash\n");
			break;
		}

		if ((file_name == NULL || strlen(file_name) == 0) && (fdev->parts_info[i].volume_images_count == 0)) {
			/* if not file not exists, it mean not to flash */
			printf("file name is null, not to flashing, continue\n");
			continue;
		}

		if (fdev->parts_info[i].volume_images_count == 0) {
			partition_offset = 0;
			printf("\n\nFlashing part: %s, file:%s\n", part_name, file_name);
			// big rootfs image(larger than 4GB) will split to multi files except flash to nand.
			sprintf(blk_dev_str, "%d:%d", fdev->dev_index, bootfs_part_index);
			if ((0 == strcmp(part_name, BIG_IMG_PARTNAME))
				&& (strcmp(fdev->device_name, "mmc") == 0 || strcmp(fdev->device_name, "usb") == 0)
				&& !file_exists(fdev->device_name, blk_dev_str, file_name, FS_TYPE_FAT)) {
					split_file_name = malloc(strlen(file_name) + 8);
					extension = file_name;
					// MUST has only 1 "." inside file name
					name = strsep(&extension, ".");
					j = 1;
					while (1) {
						sprintf(split_file_name, "%s_%d.%s", name, j, extension);
						if (file_exists(fdev->device_name, blk_dev_str, split_file_name, FS_TYPE_FAT)) {
							printf("write %s to device %s\n", split_file_name, fdev->device_name);
							ret = load_and_flash_file(cmdtp, fdev, split_file_name, part_name, &partition_offset);
							if (RESULT_OK != ret)
								break;
							j++;
						}
						else
							break;
					}

					free(split_file_name);
				}
			else{
				ret = load_and_flash_file(cmdtp, fdev, file_name, part_name, &partition_offset);
			}

			if (RESULT_OK != ret) {
				printf("Write %s to partition %s fail(%d)\n", file_name, part_name, ret);
				break;
			}
			time_start_flash = get_timer(time_start_flash);
			printf("finish image %s flash, consume %lld ms\n", file_name, time_start_flash);
		} else if (fdev->parts_info[i].volume_images_count > 0) {
			for (j = 0; j < fdev->parts_info[i].volume_images_count; ++j) {
				const char *volume_name = fdev->parts_info[i].volume_images[j].name;
				char *volume_file_name = fdev->parts_info[i].volume_images[j].file_name;

				printf("\n\nFlashing volume %s with file %s\n", volume_name, volume_file_name);
				ret = flash_volume_from_file(cmdtp, fdev, volume_name, volume_file_name, part_name, &partition_offset);
				if (ret != RESULT_OK) {
					printf("Failed to flash volume %s from file %s\n", volume_name, volume_file_name);
					break;
				}

				time_start_flash = get_timer(time_start_flash);
				printf("finish image %s flash, consume %lld ms\n", file_name, time_start_flash);
			}
		}

	}

	return ret;
}


static int parse_flash_config(struct flash_dev *fdev)
{
	int ret = 0;
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);

	ret = _parse_flash_config(fdev, load_addr);
	if (ret){
		if (ret == -1)
			printf("parsing config fail\n");
		if (ret == -5)
			printf("offset must larger then previous size and offset\n");
		return ret;
	}

	if (fdev->gptinfo.gpt_table != NULL && strlen(fdev->gptinfo.gpt_table) > 1 && fdev->gptinfo.fastboot_flash_gpt){
		_write_gpt_partition(fdev);
	}

	if (fdev->mtd_table != NULL && strlen(fdev->mtd_table) > 1){
		_write_mtd_partition(fdev->mtd_table);
	}

	/*set partition to env*/
	if (_clear_env_part(load_addr, 0, fdev)){
		printf("update part info to env fail\n");
		return -1;
	}
	return 0;
}

/*Attempt to load recovery files from all possible sources*/
static int load_recovery_file(struct cmd_tbl *cmdtp, struct flash_dev *fdev,
			int argc, char *const argv[])
{
	char load_str[13] = {"\0"};
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	strcpy(load_str, simple_xtoa((ulong)load_addr));
	int device_type, result;

	if (argc < 2) {
		printf("Error: Missing source argument. Expected 'mmc', 'usb', or 'net'.\n");
		return CMD_RET_USAGE;
	}
	if (strcmp(argv[1], "mmc") == 0) {
		device_type = DEVICE_MMC;
	} else if (strcmp(argv[1], "usb") == 0) {
		device_type = DEVICE_USB;
	} else if (strcmp(argv[1], "net") == 0) {
		device_type = DEVICE_NET;
	} else {
		printf("Error: Invalid source '%s'. Expected 'mmc', 'usb', or 'net'.\n", argv[1]);
		return CMD_RET_USAGE;
	}

	result = load_from_device(cmdtp, load_str, device_type, fdev);

	return result;
}

static int perform_flash_operations(struct cmd_tbl *cmdtp, struct flash_dev *fdev)
{
	u32 boot_mode = get_boot_pin_select();
	switch(boot_mode){
#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
	case BOOT_MODE_NOR:
		/*nvme devices need scan at first*/
		if (!strncmp("nvme", CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME, 4)){
			run_command("nvme scan", 0);
		}

		fdev->dev_desc = blk_get_dev(CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME,
							 CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_INDEX);
		if (!fdev->dev_desc || fdev->dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("get blk faild\n");
			return -1;
		}

		if (flash_image(cmdtp, fdev)) {
			return RESULT_FAIL;
		}

		break;
#endif //CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME

	case BOOT_MODE_NAND:
		if (flash_image(cmdtp, fdev)) {
			return RESULT_FAIL;
		}

		break;

	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
		fdev->dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
		if (!fdev->dev_desc || fdev->dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("get emmc_device faild\n");
			return -1;
		}

		if (flash_image(cmdtp, fdev)) {
			return RESULT_FAIL;
		}
		/*if flash to emmc，it should write bootinfo to boot0/boot1*/
		specific_flash_mmc_opt(cmdtp, fdev);
		break;
#endif //CONFIG_FASTBOOT_FLASH_MMC_DEV

	default:
		return -1;
	}

	/* all flash operations successed */
	return RESULT_OK;
}

void get_mtd_partition_file(struct flash_dev *fdev)
{
	char tmp_file[30] = {"\0"};

	u32 boot_mode = get_boot_pin_select();
	switch(boot_mode){
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MTD) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MTD)
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
		/*if select nor/nand, it would check if mtd dev exists or not*/
		struct mtd_info *mtd;
		mtd_probe_devices();
		mtd_for_each_device(mtd) {
			if (!mtd_is_partition(mtd)) {
				if (mtd->size / 0x40000000){
					sprintf(tmp_file, "partition_%lldG.json", mtd->size / 0x40000000);
					fdev->mtdinfo.size_type = MTD_SIZE_G;
					fdev->mtdinfo.size = mtd->size / 0x40000000;
				} else if (mtd->size / 0x100000){
					sprintf(tmp_file, "partition_%lldM.json", mtd->size / 0x100000);
					fdev->mtdinfo.size_type = MTD_SIZE_M;
					fdev->mtdinfo.size = mtd->size / 0x100000;
				} else if (mtd->size / 0x400){
					sprintf(tmp_file, "partition_%lldK.json", mtd->size / 0x400);
					fdev->mtdinfo.size_type = MTD_SIZE_K;
					fdev->mtdinfo.size = mtd->size / 0x400;
				}
			}
		}
		pr_info("get mtd partition file name:%s, \n", tmp_file);
		strcpy(fdev->partition_file_name, tmp_file);
		return;
#endif
	default:
		return;
	}
}

void get_blk_partition_file(char *file_name)
{
	struct blk_desc *dev_desc = NULL;
	const char *blk_name;
	int blk_index;

	u32 boot_mode = get_boot_pin_select();
	switch(boot_mode){
#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
	case BOOT_MODE_NOR:
		blk_name = CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME;
		blk_index = CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_INDEX;

		/*nvme devices need scan at first*/
		if (!strncmp("nvme", CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME, 4)){
			run_command("nvme scan", 0);
		}

		dev_desc = blk_get_devnum_by_typename(blk_name, blk_index);
		if (dev_desc != NULL)
			strcpy(file_name, FLASH_CONFIG_FILE_NAME);
		return;
#endif //CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
	case BOOT_MODE_NAND:
		return;
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
		blk_name = "mmc";
		blk_index = CONFIG_FASTBOOT_FLASH_MMC_DEV;
		dev_desc = blk_get_devnum_by_typename(blk_name, blk_index);
		if (dev_desc != NULL)
			strcpy(file_name, FLASH_CONFIG_FILE_NAME);
		return;
#endif //CONFIG_FASTBOOT_FLASH_MMC_DEV

	default:
		return;
	}
}

static int do_flash_image(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	printf("RECOVERY_LOAD_IMG_ADDR:%lx, RECOVERY_LOAD_IMG_SIZE:%llx\n", RECOVERY_LOAD_IMG_ADDR, RECOVERY_LOAD_IMG_SIZE);
	struct flash_dev *fdev;

	/*fdev would free after finish revocery at func recovery_show_result*/
	fdev = malloc(sizeof(struct flash_dev));
	if (!fdev) {
		printf("Memory allocation failed!\n");
		return RESULT_FAIL;
	}
	/*would realloc the size*/
	fdev->gptinfo.gpt_table = malloc(1);
	if (!fdev->gptinfo.gpt_table)
		printf("can not malloc fdev->gptinfo.gpt_table\n");

	fdev->mtd_table = malloc(1);
	if (!fdev->mtd_table)
		printf("can not malloc fdev->mtd_table\n");

	memset(fdev, 0, sizeof(struct flash_dev));
	memset(fdev->partition_file_name, '\0', sizeof(fdev->partition_file_name));

	/*start flash*/
	unsigned long time_start_flash = get_timer(0);

	get_mtd_partition_file(fdev);
	if (strlen(fdev->partition_file_name) > 0){
		/*flash image to mtd dev*/
		printf("partition file:%s\n", fdev->partition_file_name);

		/*only one write method.*/
		fdev->mtd_write = mtd_write_raw_image;
		fdev->blk_write = NULL;

		/*Load partitino.json file*/
		int result = load_recovery_file(cmdtp, fdev, argc, argv);
		if (result != RESULT_OK) {
			recovery_show_result(fdev, RESULT_FAIL);
			return RESULT_FAIL;
		}

		/*Parse json file and fill in relevant data structures*/
		if (parse_flash_config(fdev)) {
			printf("Failed to parse flash config.\n");
			recovery_show_result(fdev, RESULT_FAIL);
			return RESULT_FAIL;
		}

		/*Perform programming operation based on the provided information*/
		if (perform_flash_operations(cmdtp, fdev)) {
			printf("Failed to flash the device.\n");
			recovery_show_result(fdev, RESULT_FAIL);
			return RESULT_FAIL;
		}
	}

	memset(fdev->partition_file_name, '\0', sizeof(fdev->partition_file_name));
	get_blk_partition_file(fdev->partition_file_name);
	if (strlen(fdev->partition_file_name) > 0){
		/*flash image to blk dev*/
		printf("partition file:%s\n", fdev->partition_file_name);

		/*clear parts infomation*/
		for (int i = 0; i < MAX_PARTITION_NUM; i++){
			if (fdev->parts_info[i].part_name != NULL){
				free(fdev->parts_info[i].part_name);
				free(fdev->parts_info[i].file_name);
				free(fdev->parts_info[i].size);
			}else{
				break;
			}
		}

		/*only one write method.*/
		fdev->mtd_write = NULL;
		fdev->blk_write = blk_write_raw_image;

		/*Load partition.json file*/
		int result = load_recovery_file(cmdtp, fdev, argc, argv);
		if (result != RESULT_OK) {
			recovery_show_result(fdev, RESULT_FAIL);
			return RESULT_FAIL;
		}

		/*Parse json file and fill in relevant data structures*/
		if (parse_flash_config(fdev)) {
			printf("Failed to parse flash config.\n");
			recovery_show_result(fdev, RESULT_FAIL);
			return RESULT_FAIL;
		}

		/*Perform programming operation based on the provided information*/
		if (perform_flash_operations(cmdtp, fdev)) {
			printf("Failed to flash the device.\n");
			recovery_show_result(fdev, RESULT_FAIL);
			return RESULT_FAIL;
		}
	}

	ulong time_enc_flash = get_timer(0);
	printf("flashing over, use time:%lu ms\n", time_enc_flash - time_start_flash);
	recovery_show_result(fdev, RESULT_OK);
	return 0;
}

U_BOOT_CMD(
	flash_image, 2, 1, do_flash_image,
	"flash image from specified source",
	"<source>\n"
	"    - <source>: mmc | usb | net\n"
	"      flash image from the specified source (e.g., mmc, usb, or net) to emmc/nand/nor.");
