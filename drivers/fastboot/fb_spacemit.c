// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <config.h>
#include <fastboot.h>
#include <malloc.h>
#include <common.h>
#include <fastboot-internal.h>
#include <image-sparse.h>
#include <image.h>
#include <part.h>
#include <mmc.h>
#include <div64.h>
#include <fb_spacemit.h>
#include <mapmem.h>
#include <memalign.h>
#include <u-boot/crc.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <cJSON.h>
#include <mtd.h>
#include <spl.h>
#include <linux/io.h>
#include <fb_mtd.h>
#include <nvme.h>
#include <tlv_eeprom.h>
#include <misc.h>
#include <search.h>
#include <env_internal.h>

#define EMMC_MAX_BLK_WRITE 16384

#if CONFIG_IS_ENABLED(SPACEMIT_FLASH)
int _write_gpt_partition(struct flash_dev *fdev)
{
	char *gpt_table_str = NULL;
	int ret = 0;

	u32 boot_mode = get_boot_pin_select();

	if (fdev->gptinfo.gpt_table != NULL && strlen(fdev->gptinfo.gpt_table) > 0){
		gpt_table_str = malloc(strlen(fdev->gptinfo.gpt_table) + 32);
		if (gpt_table_str == NULL){
			pr_err("malloc size fail\n");
			return -1;
		}
		sprintf(gpt_table_str, "env set -f partitions '%s'", fdev->gptinfo.gpt_table);
		run_command(gpt_table_str, 0);
	} else{
		pr_info("parse gpt table is NULL, do nothing");
		return 0;
	}

	memset(gpt_table_str, 0, strlen(fdev->gptinfo.gpt_table) + 32);

	switch(boot_mode){
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
		sprintf(gpt_table_str, "gpt write mmc %x '%s'",
			CONFIG_FASTBOOT_FLASH_MMC_DEV, fdev->gptinfo.gpt_table);
		if (run_command(gpt_table_str, 0)){
			pr_err("write gpt fail");
			ret = -1;
			goto err;
		}
		break;
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_SUPPORT_BLOCK_DEV)
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
		char *blk_name;
		int blk_index;

		if (get_available_blk_dev(&blk_name, &blk_index)){
			ret = -1;
			goto err;
		}

		pr_info("write gpt to dev:%s\n", blk_name);

		sprintf(gpt_table_str, "gpt write %s %x '%s'",
			blk_name, blk_index, fdev->gptinfo.gpt_table);
		if (run_command(gpt_table_str, 0)){
			pr_err("write gpt fail");
			ret = -1;
			goto err;
		}
		break;
#endif
	default:
		pr_err("no dev to write gpt table, check your env\n");
		ret = -1;
		goto err;
	}
	pr_info("parse gpt/mtd table okay");
err:
	free(gpt_table_str);

	return ret;
}

int _clear_env_part(void *download_buffer, u32 download_bytes,
								 struct flash_dev *fdev)
{
	u32 boot_mode = get_boot_pin_select();

	switch(boot_mode){
#ifdef CONFIG_ENV_IS_IN_MMC
	case BOOT_MODE_EMMC:
	case BOOT_MODE_SD:
		/*write to emmc default offset*/
		debug("write env to mmc offset:%lx\n", (ulong)FLASH_ENV_OFFSET_MMC);

		/*should not write env to env part*/
		memset(download_buffer, 0, CONFIG_ENV_SIZE);
		fastboot_mmc_flash_offset((u32)FLASH_ENV_OFFSET_MMC, download_buffer, (u32)CONFIG_ENV_SIZE);
		break;
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MTD) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MTD)
	case BOOT_MODE_NOR:
	case BOOT_MODE_NAND:
		if (strlen(fdev->mtd_table) > 0){
			pr_info("updata mtd env, table:%s\n", fdev->mtd_table);

			/* find env partition and write env data to mtd part*/
			struct part_info *part;
			struct mtd_info *mtd;
			int ret;
			ret = fb_mtd_lookup("env", &mtd, &part);
			if (ret) {
				pr_err("invalid mtd device\n");
				return -1;
			}
			ret = _fb_mtd_erase(mtd, CONFIG_ENV_SIZE);
			if (ret)
				return -1;
		}
		break;
#endif
	default:
		break;
	}
	return 0;
}

int _write_mtd_partition(struct flash_dev *fdev)
{
#ifdef CONFIG_MTD
	struct mtd_info *mtd;
	char mtd_ids[36] = {"\0"};
	char *mtd_parts = NULL;

	mtd_parts = malloc(strlen(fdev->mtd_table) + 32);
	if (mtd_parts == NULL){
		pr_err("malloc size fail\n");
		return -1;
	}

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
		pr_err("can not get mtd device");
		free(mtd_parts);
		return -1;
	}

	/*to mtd device, it should write mtd table to env.*/
	sprintf(mtd_ids, "%s=spi-dev", mtd->name);
	sprintf(mtd_parts, "spi-dev:%s", fdev->mtd_table);

	env_set("mtdids", mtd_ids);
	env_set("mtdparts", mtd_parts);

#endif
	pr_info("parse gpt/mtd table okay");
	return 0;
}

/**
 * @brief transfer the string of size 'K' or 'M' to u32 type.
 *
 * @param reserve_size , the string of size
 * @return int , return the transfer result of KB.
 */
int transfer_string_to_ul(const char *reserve_size)
{
	char *ret, *token;
	char ch[3] = {"\0"};
	char strnum[10] = {"\0"};
	u32 get_size = 0;
	const char *get_char = reserve_size;

	if (get_char == NULL || strlen(get_char) == 0)
		return 0;

	if (!strncmp("-", get_char, 1)){
		return 0;
	}

	ret = strpbrk(get_char, "KMG");
	if (ret == NULL){
		pr_debug("can not get char\n");
		return 0;
	}
	strncpy(ch, ret, 1);
	if (ch[0] == 'K' || ch[0] == 'M' || ch[0] == 'G'){
		pr_debug("reserve_size:%s, reserve_size len:%ld\n", reserve_size, strlen(reserve_size));
		strncpy(strnum, reserve_size, strlen(reserve_size));
		token = strtok(strnum, ch);
		pr_debug("token:%s, ch:%s\n", token, ch);
		get_size = simple_strtoul(token, NULL, 0);
	}else{
		pr_debug("not support size %s, should use K/M/G\n", reserve_size);
		return 0;
	}

	switch(ch[0]){
	case 'K':
		return get_size;
	case 'M':
		return get_size * 1024;
	case 'G':
		return get_size * 1024 * 1024;
	}
	return 0;
}

/**
 * @brief parse the flash_config and save partition info
 *
 * @param fdev , struct flash_dev
 * @return u32 , return 0 if parse config success.
 */
int _parse_flash_config(struct flash_dev *fdev, void *load_flash_addr)
{
	u32 part_index = 0;
	bool parse_mtd_partition = false;
	cJSON *json_root;

	int result = 0;
	char *combine_str = NULL;
	int combine_len = 1;
	int combine_size = 0;
	int combine_len_extra = 0;
	int off = 0;

	/*init and would remalloc while size is increasing*/
	combine_str = malloc(combine_len);
	if (combine_str == NULL)
		return -1;

	memset(combine_str, '\0', combine_len);
	json_root = cJSON_Parse(load_flash_addr);
	if (!json_root){
		pr_err("can not parse json, check your flash_config.cfg is json format or not\n");
		return -1;
	}

	/*judge if parse mtd or gpt partition*/
	cJSON *cj_format = cJSON_GetObjectItem(json_root, "format");
	if (cj_format && cj_format->type == cJSON_String){
		if (!strncmp("gpt", cj_format->valuestring, 3)){
			fdev->gptinfo.fastboot_flash_gpt = true;
			combine_len_extra = 20;
		}else if(!strncmp("mtd", cj_format->valuestring, 3)){
			parse_mtd_partition = true;
			combine_len_extra = 6;
		}
	}

	cJSON *cj_parts = cJSON_GetObjectItem(json_root, "partitions");
	if (cj_parts && cj_parts->type == cJSON_Array){
		for(int i = 0; i < cJSON_GetArraySize(cj_parts); i++){
			const char *node_part = NULL;
			const char *node_file = NULL;
			const char *node_offset = NULL;
			const char *node_size = NULL;
			fdev->parts_info[part_index].hidden = false;

			cJSON *arraypart = cJSON_GetArrayItem(cj_parts, i);
			cJSON *cj_name = cJSON_GetObjectItem(arraypart, "name");
			if (cj_name && cj_name->type == cJSON_String)
				node_part = cj_name->valuestring;
			else
				node_part = "";

			/*bootinfo should be hidden as default in gpt partition*/
			if (!parse_mtd_partition){
				if (strlen(node_part) > 0 && !strncmp("bootinfo", node_part, 8)){
					pr_info("bootinfo would not add as partition\n");
					continue;
				}
			}

			cJSON *cj_hidden = cJSON_GetObjectItem(arraypart, "hidden");
			if (cj_hidden){
				if ((cj_hidden->type == cJSON_String && strcmp("true", cj_hidden->valuestring) == 0)
						|| cj_hidden->type == cJSON_True){
					printf("!!!! patr name:%s would set to hidden part !!!!\n", node_part);
					fdev->parts_info[part_index].hidden = true;
				}
			}

			cJSON *cj_filename = cJSON_GetObjectItem(arraypart, "image");
			if (cj_filename && cj_filename->type == cJSON_String)
				node_file = cj_filename->valuestring;
			else
				node_file = "";

			cJSON *cj_volume_images = cJSON_GetObjectItem(arraypart, "volume_images");
			if (cj_volume_images) {
				int volume_count = cJSON_GetArraySize(cj_volume_images);
				fdev->parts_info[part_index].volume_images = malloc(volume_count * sizeof(struct flash_volume_image));
				fdev->parts_info[part_index].volume_images_count = volume_count;

				int volume_index = 0;
				cJSON *cj_volume_image = NULL;
				cJSON_ArrayForEach(cj_volume_image, cj_volume_images) {
					const char *volume_name = cj_volume_image->string;
					const char *image_file = cj_volume_image->valuestring;

					fdev->parts_info[part_index].volume_images[volume_index].name = strdup(volume_name);
					fdev->parts_info[part_index].volume_images[volume_index].file_name = strdup(image_file);
					volume_index++;
				}
			}

			cJSON *cj_offset = cJSON_GetObjectItem(arraypart, "offset");
			if (cj_offset && cj_offset->type == cJSON_String)
				node_offset = cj_offset->valuestring;
			else
				node_offset = "";

			cJSON *cj_size = cJSON_GetObjectItem(arraypart, "size");
			if (cj_size && cj_size->type == cJSON_String)
				node_size = cj_size->valuestring;
			else
				node_size = "";

			/*make sure that offset would not over than previous size and offset*/
			off = transfer_string_to_ul(node_offset);

			if (off > 0 && off < combine_size){
				pr_err("offset must larger then previous, off:%x, combine_size:%x\n", off, combine_size);
				return -5;
			}

			combine_len += strlen(node_part) + strlen(node_offset) + strlen(node_size) + combine_len_extra;
			combine_str = realloc(combine_str, combine_len);
			if (combine_str == NULL){
				pr_err("realloc combine_str fail\n");
				return -1;
			}

			/*if next part has define offset, use it offset, or it would caculate front part offset and size*/
			if (off > 0)
				combine_size = off;

			/*TODO: support hidden partition for mtd dev*/
			if (parse_mtd_partition){
				/*parse mtd partition*/
				if (strlen(combine_str) == 0)
					sprintf(combine_str, "%s%s@%dK(%s)", combine_str, node_size, combine_size, node_part);
				else
					sprintf(combine_str, "%s,%s@%dK(%s)", combine_str, node_size, combine_size, node_part);
			}else if (!fdev->parts_info[part_index].hidden && fdev->gptinfo.fastboot_flash_gpt){
				/*parse gpt partition*/
				if (strlen(node_offset) == 0)
					sprintf(combine_str, "%sname=%s,size=%s;", combine_str, node_part, node_size);
				else
					sprintf(combine_str, "%sname=%s,start=%s,size=%s;", combine_str, node_part, node_offset, node_size);
			}

			/*save part offset and size to byte*/
			fdev->parts_info[part_index].part_offset = combine_size * 1024;
			fdev->parts_info[part_index].part_size = transfer_string_to_ul(node_size) * 1024;

			/*save as the next part offset*/
			combine_size += transfer_string_to_ul(node_size);

			/*after finish recovery, it would free the malloc paramenter at func recovery_show_result*/
			fdev->parts_info[part_index].part_name = malloc(strlen(node_part));
			if (!fdev->parts_info[part_index].part_name){
				pr_err("malloc part_name fail\n");
				result = RESULT_FAIL;
				goto free_cjson;
			}
			strcpy(fdev->parts_info[part_index].part_name, node_part);

			fdev->parts_info[part_index].size = malloc(strlen(node_size));
			if (!fdev->parts_info[part_index].size){
				pr_err("malloc size fail\n");
				result = RESULT_FAIL;
				goto free_cjson;
			}

			strcpy(fdev->parts_info[part_index].size, node_size);

			if (node_file == NULL){
				pr_err("not set file name, set to null\n");
				fdev->parts_info[part_index].file_name = NULL;
			}else{
				fdev->parts_info[part_index].file_name = malloc(strlen(node_file) + strlen(FLASH_IMG_FOLDER) + 2);
				if (!fdev->parts_info[part_index].file_name){
					pr_err("malloc file_name fail\n");
					result = RESULT_FAIL;
					goto free_cjson;
				}
				if (strlen(FLASH_IMG_FOLDER) > 0){
					strcpy(fdev->parts_info[part_index].file_name, FLASH_IMG_FOLDER);
					strcat(fdev->parts_info[part_index].file_name, "/");
					strcat(fdev->parts_info[part_index].file_name, node_file);
				}else{
					strcpy(fdev->parts_info[part_index].file_name, node_file);
				}
			}

			pr_info("Part info: %s, %s\n", fdev->parts_info[part_index].part_name, fdev->parts_info[part_index].file_name ? fdev->parts_info[part_index].file_name : "None");
			if (fdev->parts_info[part_index].volume_images_count > 0) {
				for (int j = 0; j < fdev->parts_info[part_index].volume_images_count; j++) {
					pr_info("Volume name: %s, Image file: %s\n",
						fdev->parts_info[part_index].volume_images[j].name,
						fdev->parts_info[part_index].volume_images[j].file_name);
				}
			}
			part_index++;
		}
	}else{
		pr_err("do not get partition info, check the input file\n");
		return -1;
	}
	if (parse_mtd_partition){
		fdev->mtd_table = realloc(fdev->mtd_table, combine_len);
		strcpy(fdev->mtd_table, combine_str);
	}
	else{
		fdev->gptinfo.gpt_table = realloc(fdev->gptinfo.gpt_table, combine_len);
		strcpy(fdev->gptinfo.gpt_table, combine_str);
	}

free_cjson:
	cJSON_free(json_root);
	free(combine_str);
	return result;
}



/**
 * fastboot_oem_flash_gpt() - parse flash_config and write gpt table.
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_oem_flash_gpt(const char *cmd, void *download_buffer, u32 download_bytes,
							char *response, struct flash_dev *fdev)
{
	int ret = 0;

	ret = _parse_flash_config(fdev, (void *)fastboot_buf_addr);
	if (ret){
		if (ret == -1){
			pr_err("parsing config fail\n");
		}
		if (ret == -5)
			fastboot_fail("offset must larger then previous size and offset", response);
		return;
	}

	if (strlen(fdev->gptinfo.gpt_table) > 0 && fdev->gptinfo.fastboot_flash_gpt){
		if (_write_gpt_partition(fdev)){
			fastboot_fail("write gpt tabel fail", response);
			return;
		}
	}

	if (strlen(fdev->mtd_table) > 0){
		if (_write_mtd_partition(fdev)){
			fastboot_fail("write mtd tabel fail", response);
			return;
		}
	}

	/*set partition to env*/
	if (_clear_env_part(download_buffer, download_bytes, fdev)){
		fastboot_fail("clear env fail", response);
		return;
	}

	/*maybe there doesn't have gpt/mtd partition, should not return fail*/
	fastboot_okay("parse gpt/mtd table okay", response);
	return;
}

/**
 * @brief flash env to reserve partition.
 *
 * @param cmd env
 * @param download_buffer load env.bin to addr
 * @param download_bytes env.bin size
 * @param response
 * @param fdev
 */
void fastboot_oem_flash_env(const char *cmd, void *download_buffer, u32 download_bytes,
							char *response, struct flash_dev *fdev)
{
	char cmdbuf[64] = {'\0'};

	/*load env.bin*/
	sprintf(cmdbuf, "env import -c 0x%lx 0x%lx", (ulong)download_buffer, (ulong)CONFIG_ENV_SIZE);

	if (run_command(cmdbuf, 0)){
		pr_err("can not import env, try to load env.txt\n");
		memset(cmdbuf, '\0', 32);
		/*load env.txt*/
		sprintf(cmdbuf, "env import -t 0x%lx", (ulong)download_buffer);
		if (run_command(cmdbuf, 0)){
			fastboot_fail("Cannot flash env partition", response);
			return;
		}
	}

	if (_clear_env_part(download_buffer, download_bytes, fdev)){
		fastboot_fail("clear env fail", response);
		return;
	}

	fastboot_okay("flash env partition okay", response);
	return;
}


/**
 * fb_mmc_blk_write() - Write/erase MMC in chunks of EMMC_MAX_BLK_WRITE
 *
 * @block_dev: Pointer to block device
 * @start: First block to write/erase
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for write or NULL for erase
 */
static __maybe_unused lbaint_t fb_mmc_blk_write(struct blk_desc *block_dev, lbaint_t start,
				 lbaint_t blkcnt, const void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_written;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int i;

	for (i = 0; i < blkcnt; i += EMMC_MAX_BLK_WRITE) {
		cur_blkcnt = min((int)blkcnt - i, EMMC_MAX_BLK_WRITE);
		if (buffer) {
			if (fastboot_progress_callback)
				fastboot_progress_callback("writing");
			blks_written = blk_dwrite(block_dev, blk, cur_blkcnt,
						  buffer + (i * block_dev->blksz));
		} else {
			if (fastboot_progress_callback)
				fastboot_progress_callback("erasing");
			blks_written = blk_derase(block_dev, blk, cur_blkcnt);
		}
		blk += blks_written;
		blks += blks_written;
	}
	return blks;
}

int flash_mmc_boot_op(struct blk_desc *dev_desc, void *buffer,
							int hwpart, u32 buff_sz, u32 offset)
{
	lbaint_t blkcnt;
	lbaint_t blks;
	lbaint_t blkoff;
	unsigned long blksz;

	// To operate on EMMC_BOOT1/2 (mmc0boot0/1) we first change the hwpart
	if (blk_dselect_hwpart(dev_desc, hwpart)) {
		pr_err("Failed to select hwpart\n");
		return -1;
	}

	if (buffer) { /* flash */
		pr_info("%s, %p\n", __func__, buffer);
		/* determine number of blocks to write */
		blksz = dev_desc->blksz;
		blkcnt = ((buff_sz + (blksz - 1)) & ~(blksz - 1));
		blkcnt = lldiv(blkcnt, blksz);

		if (blkcnt > dev_desc->lba) {
			pr_err("Image size too large\n");
			return -1;
		}
		if (offset % blksz) {
				pr_err("offset must be %lx align\n", blksz);
				return -1;
		}

		debug("Start Flashing Image to EMMC_BOOT%d...\n", hwpart);
		blkoff = offset / blksz;
		blks = fb_mmc_blk_write(dev_desc, blkoff, blkcnt, buffer);

		if (blks != blkcnt) {
			pr_err("Failed to write EMMC_BOOT%d\n", hwpart);
			return -1;
		}

		pr_info("........ wrote %lu bytes to EMMC_BOOT%d\n",
			   blkcnt * blksz, hwpart);
	}

	return 0;
}

/**
 * fastboot_mmc_flash_offset() - Write fsbl image to eMMC
 *
 * @start_offset: start offset to write.
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 */
int fastboot_mmc_flash_offset(u32 start_offset, void *download_buffer,
							 u32 download_bytes)
{
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	struct blk_desc *dev_desc;
	struct disk_partition info = {0};
	lbaint_t blkcnt;
	u32 offset = start_offset;
	lbaint_t blks;

	dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc){
		return -1;
	}
	part_get_info(dev_desc, 1, &info);
	info.blksz = dev_desc->blksz;
	if(info.blksz == 0)
		return -1;
	if (!download_bytes){
		pr_err("it should run command 'fastboot stage fsbl.bin' before run flash fsbl\n");
		return -1;
	}

	info.start = offset / info.blksz;
	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info.blksz - 1)) & ~(info.blksz - 1));
	blkcnt = lldiv(blkcnt, info.blksz);

	blks = fb_mmc_blk_write(dev_desc, info.start, blkcnt, download_buffer);

	if (blks != blkcnt) {
			pr_err("failed writing to device %d\n", dev_desc->devnum);
			return -1;
	}

	pr_info("........ wrote 0x%lx sector bytes to blk offset 0x%lx\n", blkcnt, info.start);
#endif
	return 0;
}


u64 checksum64(u64 *baseaddr, u64 size)
{
	u64 sum = 0;
	u64 i, cachelines;
	u64 dwords, bytes;
	u8 *data;

	// each cache line has 64bytes
	cachelines = size / 64;
	bytes = size % 64;
	dwords = bytes / 8;
	bytes = bytes % 8;

	for (i = 0; i < cachelines; i++) {
		u64 val1 = *(baseaddr + 0);
		u64 val2 = *(baseaddr + 1);
		u64 val3 = *(baseaddr + 2);
		u64 val4 = *(baseaddr + 3);
		u64 val5 = *(baseaddr + 4);
		u64 val6 = *(baseaddr + 5);
		u64 val7 = *(baseaddr + 6);
		u64 val8 = *(baseaddr + 7);

		sum += val1;
		sum += val2;
		sum += val3;
		sum += val4;
		sum += val5;
		sum += val6;
		sum += val7;
		sum += val8;
		baseaddr += 8;
	}

	/*calculate the rest of dowrd*/
	for (i = 0; i < dwords; i++) {
		sum += *baseaddr;
		baseaddr++;
	}

	data = (u8*)baseaddr;
	/*calculate the rest of byte*/
	for (i = 0; i < bytes; i++) {
		sum += data[i];
	}

	return sum;
}

int compare_blk_image_val(struct blk_desc *dev_desc, u64 compare_val, lbaint_t part_start_cnt,
			ulong blksz, uint64_t image_size)
{
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	u32 div_times = (image_size + RECOVERY_LOAD_IMG_SIZE - 1) / RECOVERY_LOAD_IMG_SIZE;
	u64 calculate = 0;
	uint64_t byte_remain = image_size;
	uint64_t download_bytes = 0;
	u32 blk_size, n;
	unsigned long time_start_flash = get_timer(0);

	/*if compare_val is 0, return 0 directly*/
	if (!compare_val)
		return 0;

	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return -1;
	}

	for (int i = 0; i < div_times; i++) {
		pr_info("download and flash div %d\n", i);
		download_bytes = byte_remain > RECOVERY_LOAD_IMG_SIZE ? RECOVERY_LOAD_IMG_SIZE : byte_remain;

		blk_size = (download_bytes + (blksz - 1)) / blksz;
		n = blk_dread(dev_desc, part_start_cnt, blk_size, load_addr);
		if (n != blk_size) {
			pr_err("mmc read blk not equal it should be\n");
			return -1;
		}
		// calculate = crc32_wd(crc, (const uchar *)load_addr, download_bytes, CHUNKSZ_CRC32);
		calculate += checksum64(load_addr, download_bytes);

		part_start_cnt += blk_size;
		byte_remain -= download_bytes;
	}

	pr_info("get calculate value:%llx, compare calculate:%llx\n", calculate, compare_val);
	time_start_flash = get_timer(time_start_flash);
	pr_info("compare over, use time:%lu ms\n", time_start_flash);
	return (calculate == compare_val) ? 0 : -1;
}


int compare_mtd_image_val(struct mtd_info *mtd, u64 compare_val, uint64_t image_size)
{
	void *load_addr = (void *)map_sysmem(RECOVERY_LOAD_IMG_ADDR, 0);
	u32 div_times = (image_size + RECOVERY_LOAD_IMG_SIZE - 1) / RECOVERY_LOAD_IMG_SIZE;
	u64 calculate = 0;
	uint64_t byte_remain = image_size;
	uint64_t download_bytes = 0;
	u32 hdr_off = 0;
	int ret;

	debug("mtd size:%llx, image_size:%llx\n", mtd->size, image_size);
	unsigned long time_start_flash = get_timer(0);

	/*if compare_val is 0, return 0 directly*/
	if (!compare_val)
		return 0;

	for (int i = 0; i < div_times; i++) {
		pr_info("\ndownload and flash div %d\n", i);
		download_bytes = byte_remain > RECOVERY_LOAD_IMG_SIZE ? RECOVERY_LOAD_IMG_SIZE : byte_remain;
		ret = _fb_mtd_read(mtd, load_addr, hdr_off, download_bytes, NULL);
		if (ret){
			pr_err("cannot read data from mtd dev\n");
			return -1;
		}

		// calculate = crc32_wd(calculate, (const uchar *)load_addr, download_bytes, CHUNKSZ_CRC32);
		calculate += checksum64(load_addr, download_bytes);
		hdr_off += download_bytes;
		byte_remain -= download_bytes;
	}

	pr_info("get calculate value:%llx, compare calculate:%llx\n", calculate, compare_val);
	time_start_flash = get_timer(time_start_flash);
	pr_info("compare over, use time:%lu ms\n", time_start_flash);
	return (calculate == compare_val) ? 0 : -1;
}


/**
 * @brief flash bootinfo to reserve partition.
 *
 * @param cmd
 * @param download_buffer
 * @param download_bytes
 * @param response
 * @param fdev
 */
void fastboot_oem_flash_bootinfo(const char *cmd, void *download_buffer,
		u32 download_bytes, char *response, struct flash_dev *fdev)
{
#if CONFIG_IS_ENABLED(FASTBOOT_FLASH_MMC) || CONFIG_IS_ENABLED(FASTBOOT_MULTI_FLASH_OPTION_MMC)
	debug("%s\n", __func__);
	struct blk_desc *dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		if (response)
			fastboot_fail("invalid mmc device", response);
		return;
	}

	/*fill up emmc bootinfo*/
	struct boot_parameter_info *boot_info;
	boot_info = (struct boot_parameter_info *)download_buffer;
	memset(boot_info, 0, sizeof(boot_info));
	boot_info->magic_code = BOOT_INFO_EMMC_MAGICCODE;
	boot_info->version_number = BOOT_INFO_EMMC_VERSION;
	boot_info->page_size = BOOT_INFO_EMMC_PAGESIZE;
	boot_info->block_size = BOOT_INFO_EMMC_BLKSIZE;
	boot_info->total_size = BOOT_INFO_EMMC_TOTALSIZE;
	boot_info->spl0_offset = BOOT_INFO_EMMC_SPL0_OFFSET;
	boot_info->spl1_offset = BOOT_INFO_EMMC_SPL1_OFFSET;
	boot_info->spl_size_limit = BOOT_INFO_EMMC_LIMIT;
	strcpy(boot_info->flash_type, "eMMC");
	boot_info->crc32 = crc32_wd(0, (const uchar *)boot_info, 0x40, CHUNKSZ_CRC32);

	/*flash bootinfo*/
	pr_info("bootinfo:%p, boot_info->crc32:%x, sizeof(boot_info):%lx, download_buffer:%p\n", boot_info, boot_info->crc32, sizeof(boot_info), download_buffer);

	if (flash_mmc_boot_op(dev_desc, download_buffer, 1, sizeof(boot_info), 0)){
		if (response)
			fastboot_fail("flash mmc boot fail", response);
		return;
	}
	if (response)
		fastboot_okay(NULL, response);
#endif

	return;
}
#endif

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_CONFIG_ACCESS)
#if defined(CONFIG_SPL_BUILD)
extern int get_tlvinfo_from_eeprom(int tcode, char *buf);
extern int set_val_to_tlvinfo(int tcode, char *valvoid);
extern int write_tlvinfo_to_eeprom(void);
#else
static bool tlvinfo_init = false;
#endif

struct oem_config_info
{
	const char *name;
	uint32_t id;
	uint32_t max_len;
	char* (*convert)(char *);
};
const struct oem_config_info config_info[] = {
	{ "product_name", TLV_CODE_PRODUCT_NAME, 32, NULL },
	{ "part#", TLV_CODE_PART_NUMBER, 32, NULL },
	{ "serial#", TLV_CODE_SERIAL_NUMBER, 32, NULL },
	{ "ethaddr", TLV_CODE_MAC_BASE, 17, NULL },
	{ "wifi_addr", TLV_CODE_WIFI_MAC_ADDR, 17, NULL },
	{ "bt_addr", TLV_CODE_BLUETOOTH_ADDR, 17, NULL },
	{ "ethsize", TLV_CODE_MAC_SIZE, 6, NULL },/*size must equal or less than 65535*/
	{ "manufacture_date", TLV_CODE_MANUF_DATE, 19, NULL },
	{ "device_version", TLV_CODE_DEVICE_VERSION, 3, NULL },
	{ "manufacturer", TLV_CODE_MANUF_NAME, 32, NULL },
	{ "sdk_version", TLV_CODE_SDK_VERSION, 3, NULL},
	{ "ddr_cs_num", TLV_CODE_DDR_CSNUM, 3, NULL},
	{ "ddr_datarate", TLV_CODE_DDR_DATARATE, 5, NULL},
	{ "ddr_tx_odt", TLV_CODE_DDR_TX_ODT, 3, NULL},
	{ "ddr_type", TLV_CODE_DDR_TYPE, 32, NULL},
	{ "pmic_type", TLV_CODE_PMIC_TYPE, 3, NULL},
	{ "eeprom_i2c_index", TLV_CODE_EEPROM_I2C_INDEX, 3, NULL},
	{ "eeprom_pin_group", TLV_CODE_EEPROM_PIN_GROUP, 3, NULL},
};

static int write_config_info_to_eeprom(uint32_t id, char *value)
{
#if defined(CONFIG_SPL_BUILD)
	if (set_val_to_tlvinfo(id, value) == 0)
#else
	if (!tlvinfo_init){
		run_command("tlv_eeprom", 0);
		tlvinfo_init = true;
	}
	if (run_commandf("tlv_eeprom set 0x%x %s", id, value) == 0)
#endif
		return 0;
	else
		return -1;
}

#if CONFIG_IS_ENABLED(SPACEMIT_K1X_EFUSE)
static int write_config_info_to_efuse(uint32_t id, char *value)
{
	struct udevice *dev;
	uint8_t fuses[2];
	int ret;

	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(spacemit_k1x_efuse), &dev);
	if (ret) {
		return ret;
	}

	memset(fuses, 0, sizeof(fuses));
	if (TLV_CODE_PMIC_TYPE == id)
		fuses[1] |= dectoul(value, NULL) & 0x0F;
	else if (TLV_CODE_EEPROM_I2C_INDEX == id)
		fuses[0] |= dectoul(value, NULL) & 0x0F;
	else if (TLV_CODE_EEPROM_PIN_GROUP == id)
		fuses[0] |= (dectoul(value, NULL) & 0x03) << 4;
	else {
		pr_err("NOT support efuse ID %d\n", id);
		return EFAULT;
	}

	// write to efuse, each bank has 32byte efuse data
	return misc_write(dev, K1_EFUSE_USER_BANK0 * 32, fuses, sizeof(fuses));
}
#endif

static struct oem_config_info* get_config_info(char *key)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(config_info); i++) {
		if (0 == strcmp(key, config_info[i].name))
			return (struct oem_config_info*)&config_info[i];
	}

	return NULL;
}

static void read_oem_configuration(char *config, char *response)
{
	struct oem_config_info* info;
	char *ack;

	ack = calloc(0, 256);
	if (NULL == ack) {
		pr_err("malloc buffer for ack fail\n");
		return;
	}

	info = get_config_info(config);
	if (NULL != info){
		pr_info("%s, %x, \n", info->name, info->id);
#if defined(CONFIG_SPL_BUILD)
		if (get_tlvinfo_from_eeprom(info->id, ack) == 0){
#else
		char *tmp_str = env_get(info->name);
		if (tmp_str != NULL){
			strcpy(ack, tmp_str);
#endif
			fastboot_okay(ack, response);
		}else{
			fastboot_fail("key NOT exist", response);
		}
	}else{
		fastboot_fail("key NOT exist", response);
	}
	free(ack);
}

static void write_oem_configuration(char *config, char *response)
{
	char *key, *value, *dest;
	const struct oem_config_info* info;
	int (*config_write)(uint32_t id, char *value), ret = -1;

	dest = strsep(&config, ":");
	key = strsep(&dest, "@");
	value = config;
	pr_info("try to set config info for %s: %s@%s\n", key, value, dest);

	if (0 == strcmp(dest, "eeprom"))
		config_write = write_config_info_to_eeprom;
#if CONFIG_IS_ENABLED(SPACEMIT_K1X_EFUSE)
	else if (0 == strcmp(dest, "efuse"))
		config_write = write_config_info_to_efuse;
#endif
	else {
		fastboot_fail("NOT support destination", response);
		return;
	}

	info = get_config_info(key);
	if ((NULL != info) && (strlen(value) <= info->max_len)) {
		if (0 == config_write(info->id, value)) {
			env_set(key, value);
			ret = 0;
		}
	}

	if (0 == ret)
		fastboot_okay(NULL, response);
	else
		fastboot_fail("NOT exist", response);
}

static void flush_oem_configuration(char *config, char *response)
{
#if defined(CONFIG_SPL_BUILD)
	if (0 == write_tlvinfo_to_eeprom())
#else
	if (!tlvinfo_init){
		run_command("tlv_eeprom", 0);
		tlvinfo_init = true;
	}
	if (run_command("tlv_eeprom write", 0) == 0)
#endif
		fastboot_okay(NULL, response);
	else
		fastboot_fail("write fail", response);
}

/**
 * fastboot_config_access() - Access configurations.
 *
 * @operation: Pointer to operation string
 *			  read: read configuration
 *			  write: write configuration
 * @config: Pointer to config string
 *			  if is read operation, then
 * @response: Pointer to fastboot response buffer
 */
void fastboot_config_access(char *operation, char *config, char *response)
{
	if (0 == strcmp(operation, "read"))
		read_oem_configuration(config, response);
	else if (0 == strcmp(operation, "write"))
		write_oem_configuration(config, response);
	else if (0 == strcmp(operation, "flush"))
		flush_oem_configuration(config, response);
	else
		fastboot_fail("NOT support", response);
}
#endif /*CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_CONFIG_ACCESS)*/

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_ENV_ACCESS)
#if defined(CONFIG_SPL_BUILD)
extern char *product_name;
static void read_oem_env(char *env, char *response)
{
	char *key = env;
	char *value = NULL;

	if (NULL != key) {
		pr_debug("try to find env info for %s\n", key);
		if ((0 == strcmp(key, "product_name")) && (NULL != product_name))
			value = product_name;
		else
			value = env_get(key);
	}

	if (NULL != value) {
		fastboot_okay(value, response);
	} else {
		fastboot_fail("NOT exist", response);
	}
}

static void write_oem_env(char *env, char *response)
{
	char *key, *value = env;

	key = strsep(&value, ":");
	if ((NULL != key) && (NULL != value) && (0 == strcmp(key, "product_name"))) {
		pr_debug("try to set env %s to %s\n", key, value);
		// NOT support env_set API in SPL stage
		if (NULL != product_name)
			free(product_name);
		product_name = strdup(value);
		fastboot_okay(NULL, response);
	} else {
		fastboot_fail("NOT support", response);
	}
}
#else
static void read_oem_env(char *env, char *response)
{
	char *key = env;
	char *value = NULL;

	if (NULL != key) {
		pr_debug("try to find env info for %s\n", key);
		value = env_get(key);
	}

	if (NULL != value) {
		fastboot_okay(value, response);
	} else {
		fastboot_fail("NOT exist", response);
	}
}

static void write_oem_env(char *env, char *response)
{
	char *key, *value = env;

	key = strsep(&value, ":");
	if ((NULL != key) && (NULL != value)) {
		pr_debug("try to set env %s to %s\n", key, value);
		env_set(key, value);
		fastboot_okay(NULL, response);
	} else {
		fastboot_fail("NOT support", response);
	}
}
#endif

/**
 * fastboot_env_access() - Access env variables.
 *
 * @operation: Pointer to env operation string
 *			  get: read env
 *			  set: write env
 * @env: Pointer to env string
 *			  if is read operation, then
 * @response: Pointer to fastboot response buffer
 */
void fastboot_env_access(char *operation, char *env, char *response)
{
	if (0 == strcmp(operation, "get"))
		read_oem_env(env, response);
	else if (0 == strcmp(operation, "set"))
		write_oem_env(env, response);
	else
		fastboot_fail("NOT support", response);
}
#endif

#define GZIP_HEADER_HEAD_CRC		2
#define GZIP_HEADER_EXTRA_FIELD		4
#define GZIP_HEADER_ORIG_NAME		8
#define GZIP_HEADER_COMMENT		0x10
#define GZIP_HEADER_RESERVED		0xe0
#define GZIP_HEADER_DEFLATED		8
int check_gzip_format(const unsigned char *src, unsigned long len)
{
	int i, flags;

	/* skip header */
	i = 10;
	flags = src[3];
	if (src[2] != GZIP_HEADER_DEFLATED || (flags & GZIP_HEADER_RESERVED) != 0) {
		pr_info("is not gzipped data\n");
		return (-1);
	}
	if ((flags & GZIP_HEADER_EXTRA_FIELD) != 0)
		i = 12 + src[10] + (src[11] << 8);
	if ((flags & GZIP_HEADER_ORIG_NAME) != 0)
		while (src[i++] != 0)
			;
	if ((flags & GZIP_HEADER_COMMENT) != 0)
		while (src[i++] != 0)
			;
	if ((flags & GZIP_HEADER_HEAD_CRC) != 0)
		i += 2;
	if (i >= len) {
		pr_info("gunzip out of data in header\n");
		return (-1);
	}
	return i;
}

#if CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_ERASE)
/*boot0,boot1 would erase 128KB size*/
#define ERASE_BOOT_SIZE (0x100)

int clear_emmc(lbaint_t blkcnt)
{
	struct blk_desc *dev_desc;
	struct mmc *mmc;
	u32 n;

	mmc_init_device(MMC_DEV_EMMC);
	mmc = find_mmc_device(MMC_DEV_EMMC);
	if (!mmc){
		pr_err("can not get mmc dev\n");
		return -1;
	}
	if (mmc_init(mmc)){
		pr_err("can not init mmc\n");
		return -1;
	}

	dev_desc = mmc_get_blk_desc(mmc);
	if (!dev_desc){
		pr_err("can not get blk dev of emmc\n");
		return -1;
	}

	n = blk_derase(dev_desc, 0, blkcnt);
	if (n != blkcnt){
		pr_err("erase size %lx fail\n", blkcnt);
		return -1;
	}

	/* erase boot0/boot1 partition*/
	if (mmc_set_part_conf(mmc, 0, 0, 1) ||
			ERASE_BOOT_SIZE != blk_derase(dev_desc, 0, ERASE_BOOT_SIZE)){
		pr_err("erase boot0 fail\n");
		return -1;
	}
	if (mmc_set_part_conf(mmc, 0, 0, 2) ||
			ERASE_BOOT_SIZE != blk_derase(dev_desc, 0, ERASE_BOOT_SIZE)){
		pr_err("erase boot1 fail\n");
		return -1;
	}
	return 0;
}

int clear_mtd(char *mtd_dev, u32 erase_size)
{
	static struct mtd_info *mtd = NULL;
	int ret;

	if (mtd == NULL){
		pr_info("mtd is not init\n");
		mtd_probe_devices();
	}
	mtd = get_mtd_device_nm(mtd_dev);
	if (IS_ERR_OR_NULL(mtd)){
		pr_info("MTD device %s not found\n", mtd_dev);
		return -1;
	}

	erase_size = round_up(erase_size, mtd->erasesize);
	ret = _fb_mtd_erase(mtd, erase_size);
	if (ret)
		return -1;
	return 0;
}

#define DEFAULT_EEPROM_ERASE_SIZE (256)
#define DEFAULT_EEPROM_DEV (0)
#define DEFAULT_EMMC_ERASE_SIZE (0x10000000)
#define DEFAULT_MTD_ERASE_SIZE (0x100000)
void clear_storage_data(char *cmd_parameter, char *response)
{
	char *cmd_str, *operation;
	u32 erase_size;

	cmd_str = cmd_parameter;
	operation = strsep(&cmd_str, " ");
	if (cmd_str != NULL){
		pr_info("get erase size:%s\n", cmd_str);
		erase_size = hextoul(cmd_str, NULL);
	}else {
		pr_info("has not define erase size, use default size\n");
		erase_size = 0;
	}

	if (!strncmp("eeprom", operation, 6)){
		erase_size = (erase_size == 0) ? DEFAULT_EEPROM_ERASE_SIZE : erase_size;
		pr_info("erase eeprom, erase size:%x\n", erase_size);
#if defined(CONFIG_SPL_BUILD)
		if (clear_eeprom(DEFAULT_EEPROM_DEV, erase_size))
#else
		if (run_command("tlv_eeprom;tlv_eeprom erase;tlv_eeprom write", 0))
#endif
			fastboot_fail("erase eeprom fail", response);
		else
			fastboot_okay(NULL, response);
		return;
	} else if (!strncmp("emmc", operation, 4)){
		erase_size = (erase_size == 0) ? DEFAULT_EMMC_ERASE_SIZE : erase_size;
		pr_info("erase emmc, erase size:%x\n", erase_size);

		if (clear_emmc(erase_size/512))
			fastboot_fail("erase emmc fail", response);
		else
			fastboot_okay(NULL, response);
		return;
	}

	erase_size = (erase_size == 0) ? DEFAULT_MTD_ERASE_SIZE : erase_size;
	if (!strncmp("nor", operation, 3)){
		if (clear_mtd("nor0", erase_size))
			fastboot_fail("erase nor fail", response);
		else
			fastboot_okay(NULL, response);
		return;
	} else if (!strncmp("nand", operation, 4)){
		if (clear_mtd("spi-nand0", erase_size))
			fastboot_fail("erase nand fail", response);
		else
			fastboot_okay(NULL, response);
		return;
	}

	fastboot_response("FAIL", response, "not support erase operation:%s", operation);
	return;
}

#endif /*CONFIG_IS_ENABLED(FASTBOOT_CMD_OEM_ERASE)*/

/**
 * @brief detect blk dev exist or not.
 *
 * @param blk_name try to find blk dev.
 * @param partition try to find partition exist or not.
 * @return int return partition index while finding partition in blk dev.
*/
int detect_blk_dev_or_partition_exist(char *blk_name, int blk_index, const char *partition)
{
	struct blk_desc *dev_desc;
	struct disk_partition info;
	u32 part;
	int err;

	dev_desc = blk_get_dev(blk_name, blk_index);
	if (!dev_desc) {
		pr_info("Cannot find blk device\n");
		return -1;
	}

	if (partition != NULL) {
		for (part = 1; part <= MAX_SEARCH_PARTITIONS; part++) {
			err = part_get_info(dev_desc, part, &info);
			if (err) {
				continue;
			}

			if (!strcmp(partition, info.name)) {
				return part;
			}
		}

		if (part > MAX_SEARCH_PARTITIONS) {
			pr_info("can not find partition in blk dev\n");
			return -1;
		}
	}

	return 0;
}

int _get_available_blk_or_part(char **blk_dev, int *index, const char *partition)
{
#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
	static bool scan_nvme = false;
	/*nvme devices need scan at first*/
	if (!scan_nvme){
		if (!strncmp("nvme", CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME, 4)
#ifdef CONFIG_FASTBOOT_SUPPORT_SECOND_BLOCK_DEV_NAME
			|| !strncmp("nvme", CONFIG_FASTBOOT_SUPPORT_SECOND_BLOCK_DEV_NAME, 4)
#endif
		){
			run_command("nvme scan", 0);
			scan_nvme = true;
		}
	}
#endif

#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
	if (strlen(CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME) > 0){
		*blk_dev = CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME;
		*index = CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_INDEX;
		if (detect_blk_dev_or_partition_exist(*blk_dev, *index, partition) < 0){
#ifdef CONFIG_FASTBOOT_SUPPORT_SECOND_BLOCK_DEV_NAME
			*blk_dev = CONFIG_FASTBOOT_SUPPORT_SECOND_BLOCK_DEV_NAME;
			*index = CONFIG_FASTBOOT_SUPPORT_SECOND_BLOCK_DEV_INDEX;
			if (detect_blk_dev_or_partition_exist(*blk_dev, *index, partition) < 0)
#endif
				return -1;
		}
	}else{
		printf("not defind blk dev, check make config\n");
		return -1;
	}
#else
	printf("not defind blk dev, check make config\n");
	return -1;
#endif //CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
	printf("detect available blk:%s\n", *blk_dev);
	return 0;
}



/**
 * @brief try to find available blk dev while defind multi blks at nor boot.
 *
 * @param blk_dev return available blk dev.
 * @param index return available blk index.
 * @param return return 0 while detect available blk dev.
*/
int get_available_blk_dev(char **blk_dev, int *index)
{
	return _get_available_blk_or_part(blk_dev, index, NULL);
}

/**
 * @brief try to find available bootable blk dev.
 * @param blk_dev return available bootable blk dev.
 * @param index return available bootable blk index.
 * @param return return 0 while detect available blk dev.
*/
int get_available_boot_blk_dev(char **blk_dev, int *index)
{
	return _get_available_blk_or_part(blk_dev, index, BOOTFS_NAME);
}
