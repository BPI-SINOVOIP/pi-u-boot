// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2014 Broadcom Corporation.
 */

#include <config.h>
#include <common.h>
#include <blk.h>
#include <env.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <fb_blk.h>
#include <image-sparse.h>
#include <image.h>
#include <log.h>
#include <part.h>
#include <div64.h>
#include <linux/compat.h>
#include <android_image.h>
#include <fb_spacemit.h>
#include <u-boot/crc.h>
#include <mmc.h>
#include <gzip.h>

#define FASTBOOT_MAX_BLK_WRITE 16384

struct fb_blk_sparse {
	struct blk_desc	*dev_desc;
};


static int do_get_part_info(struct blk_desc **dev_desc, const char *name,
			    struct disk_partition *info)
{
	int ret = -1;
#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
	if (strlen(CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME) > 0){

		/* First try partition names on the default device */
		*dev_desc = blk_get_dev(CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME,
							 CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_INDEX);
		if (*dev_desc) {
			ret = part_get_info_by_name(*dev_desc, name, info);
			if (ret >= 0)
				return ret;
		}
	}
#endif

	printf("has not define block device name \n");
	return ret;
}


/**
 * fb_blk_write() - Write/erase blk device in chunks of FASTBOOT_MAX_BLK_WRITE
 *
 * @block_dev: Pointer to block device
 * @start: First block to write/erase
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for write or NULL for erase
 */
static lbaint_t fb_blk_write(struct blk_desc *block_dev, lbaint_t start,
				 lbaint_t blkcnt, const void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_written;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int i;

	for (i = 0; i < blkcnt; i += FASTBOOT_MAX_BLK_WRITE) {
		cur_blkcnt = min((int)blkcnt - i, FASTBOOT_MAX_BLK_WRITE);
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

static lbaint_t fb_blk_sparse_write(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
	struct fb_blk_sparse *sparse = info->priv;
	struct blk_desc *dev_desc = sparse->dev_desc;

	return fb_blk_write(dev_desc, blk, blkcnt, buffer);
}

static lbaint_t fb_blk_sparse_reserve(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt)
{
	return blkcnt;
}

static void write_raw_image(struct blk_desc *dev_desc,
			    struct disk_partition *info, const char *part_name,
			    void *buffer, u32 download_bytes, char *response)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = lldiv(blkcnt, info->blksz);

	if (blkcnt > info->size) {
		pr_err("too large for partition: '%s'\n", part_name);
		fastboot_fail("too large for partition", response);
		return;
	}

	puts("Flashing Raw Image\n");

	blks = fb_blk_write(dev_desc, info->start, blkcnt, buffer);

	if (blks != blkcnt) {
		pr_err("failed writing to device %d\n", dev_desc->devnum);
		fastboot_fail("failed writing to device", response);
		return;
	}

	printf("........ wrote " LBAFU " bytes to '%s'\n", blkcnt * info->blksz,
	       part_name);
	fastboot_okay(NULL, response);
}


/**
 * fastboot_blk_get_part_info() - Lookup blk device partion by name
 *
 * @part_name: Named partition to lookup
 * @dev_desc: Pointer to returned blk_desc pointer
 * @part_info: Pointer to returned struct disk_partition
 * @response: Pointer to fastboot response buffer
 */
int fastboot_blk_get_part_info(const char *part_name,
			       struct blk_desc **dev_desc,
			       struct disk_partition *part_info, char *response)
{
	int ret;

	if (!part_name || !strcmp(part_name, "")) {
		fastboot_fail("partition not given", response);
		return -ENOENT;
	}

	ret = do_get_part_info(dev_desc, part_name, part_info);
	if (ret < 0) {
		fastboot_fail("can not find partition or devices", response);
	}

	return ret;
}


/**
 * fastboot_blk_flash_write() - Write image to blk device for fastboot
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_blk_flash_write(const char *cmd, void *download_buffer,
			      u32 download_bytes, char *response)
{
	struct blk_desc *dev_desc;
	struct disk_partition info = {0};
#ifdef CONFIG_SPACEMIT_FLASH
	static struct flash_dev *fdev = NULL;
	u32 __maybe_unused fsbl_offset = 0;
	/*save crc value to compare after flash image*/
	u64 compare_val = 0;
	/*use for gzip image*/
	static u32 __maybe_unused part_offset_t = 0;
	static char __maybe_unused part_name_t[20] = "";
	unsigned long __maybe_unused src_len = ~0UL;
	bool gzip_image = false;

	if (fdev == NULL){
		fdev = malloc(sizeof(struct flash_dev));
		if (!fdev) {
			printf("Memory allocation failed!\n");
		}
		memset(fdev, 0, sizeof(struct flash_dev));
		fdev->gptinfo.fastboot_flash_gpt = false;
		/*would realloc the size while parsing the partition table*/
		fdev->gptinfo.gpt_table = malloc(10);
		fdev->mtd_table = malloc(10);
		memset(fdev->gptinfo.gpt_table, '\0', 10);
		memset(fdev->mtd_table, '\0', 10);
		printf("init fdev success\n");
	}

	/*blk device would not flash bootinfo except emmc*/
	if (strcmp(cmd, "bootinfo") == 0) {
		fastboot_okay(NULL, response);
		return;
	}

	if (strcmp(cmd, "gpt") == 0) {
		fastboot_oem_flash_gpt(cmd, fastboot_buf_addr, download_bytes,
						response, fdev);
		return;
	}
#endif

	if (fastboot_blk_get_part_info(cmd, &dev_desc, &info, response) < 0)
		return;

	if (gzip_parse_header((uchar *)download_buffer, src_len) >= 0) {
		/*is gzip data and equal part name*/
		gzip_image = true;
		if (strcmp(cmd, part_name_t)){
			pr_info("flash part name %s is not equal to %s, \n", cmd, part_name_t);
			strcpy(part_name_t, cmd);
			part_offset_t = 0;
		}

		void *decompress_addr = (void *)GZIP_DECOMPRESS_ADDR;
		pr_info("decompress_addr:%p\n", decompress_addr);
		if (run_commandf("unzip %x %x", download_buffer, decompress_addr)){
			printf("unzip gzip data fail, \n");
			fastboot_fail("unzip gzip data fail", response);
			return;
		}

		u32 decompress_size = env_get_hex("filesize", 0);
		pr_info("get decompress_size:%x, \n", decompress_size);
		download_buffer = decompress_addr;
		download_bytes = decompress_size;
		info.start += part_offset_t / info.blksz;

		pr_info("write gzip raw data to part:%s, %p, %x, blkaddr:%lx\n", cmd, download_buffer, download_bytes, info.start);
	} else {
		strcpy(part_name_t, cmd);
		part_offset_t = 0;
	}

	if (download_bytes > info.size * info.blksz){
		printf("download_bytes is greater than part size\n");
		fastboot_fail("download_bytes is greater than part size", response);
		return;
	}

	if (!gzip_image && is_sparse_image(download_buffer)) {
		struct fb_blk_sparse sparse_priv;
		struct sparse_storage sparse = { .erase = NULL };;
		int err;

		sparse_priv.dev_desc = dev_desc;

		sparse.blksz = info.blksz;
		sparse.start = info.start;
		sparse.size = info.size;
		sparse.write = fb_blk_sparse_write;
		sparse.reserve = fb_blk_sparse_reserve;
		sparse.mssg = fastboot_fail;

		printf("Flashing sparse image at offset " LBAFU "\n",
		       sparse.start);

		sparse.priv = &sparse_priv;
		err = write_sparse_image(&sparse, cmd, download_buffer,
					 response);
		if (!err)
			fastboot_okay(NULL, response);
	} else {
		write_raw_image(dev_desc, &info, cmd, download_buffer,
				download_bytes, response);
#ifdef CONFIG_SPACEMIT_FLASH
		/*if download and flash div to many time, that the crc is not correct*/
		printf("write_raw_image, \n");
		// compare_val = crc32_wd(compare_val, (const uchar *)download_buffer, download_bytes, CHUNKSZ_CRC32);
		compare_val += checksum64(download_buffer, download_bytes);
		if (compare_blk_image_val(dev_desc, compare_val, info.start, info.blksz, download_bytes))
			fastboot_fail("compare crc fail", response);
#endif
		part_offset_t += download_bytes;
	}
}

/**
 * fastboot_blk_flash_erase() - Erase blk device for fastboot
 *
 * @cmd: Named partition to erase
 * @response: Pointer to fastboot response buffer
 */
void fastboot_blk_erase(const char *cmd, char *response)
{
	struct blk_desc *dev_desc;
	struct disk_partition info;
	lbaint_t blks, blks_start, blks_size;

	if (fastboot_blk_get_part_info(cmd, &dev_desc, &info, response) < 0)
		return;

	/* Align blocks to erase group size to avoid erasing other partitions */
	//TODO: align to blk dev erase size.
#ifdef CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME
	if (!strncmp("mmc", CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_NAME, 3)){
		struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_SUPPORT_BLOCK_DEV_INDEX);
		lbaint_t grp_size;

		grp_size = mmc->erase_grp_size;
		blks_start = (info.start + grp_size - 1) & ~(grp_size - 1);
		if (info.size >= grp_size)
			blks_size = (info.size - (blks_start - info.start)) &
					(~(grp_size - 1));
		else
			blks_size = 0;
	}else{
		blks_start = info.start;
		blks_size = info.size;
	}
#else
	return;
#endif

	blks = fb_blk_write(dev_desc, blks_start, blks_size, NULL);

	if (blks != blks_size) {
		pr_err("failed erasing from device %d\n", dev_desc->devnum);
		fastboot_fail("failed erasing from device", response);
		return;
	}

	printf("........ erased " LBAFU " bytes from '%s'\n",
	       blks_size * info.blksz, cmd);
	fastboot_okay(NULL, response);
}

/**
 * fastboot_blk_read() - load data from blk device for fastboot
 *
 * @part: Named partition to erase
 * @response: Pointer to fastboot response buffer
 */
u32 fastboot_blk_read(const char *part, u32 offset,
					void *download_buffer, char *response)
{
	struct blk_desc *dev_desc;
	struct disk_partition info = {0};
	lbaint_t hdr_sectors, off_blk, size_blk;
	lbaint_t res;

	if (do_get_part_info(&dev_desc, part, &info) < 0){
		if (dev_desc && dev_desc->blksz > 0){
			info.blksz = dev_desc->blksz;
			info.size = dev_desc->lba;
			info.start = 0;
		}else{
			fastboot_response("OKAY", response, "%08x", 0);
			return 0;
		}
	}

	if (offset >= (info.size * info.blksz)){
		fastboot_response("OKAY", response, "%08x", 0);
		return 0;
	}

	printf("info.size:%lx, info.start:%lx\n", info.size, info.start);
	/*transfer offset to blk size*/
	off_blk = (offset / info.blksz) + info.start;
	size_blk = info.size - (offset / info.blksz);

	if (offset % info.blksz)
		printf("offset should be align to 0x%lx, would change offset to 0x%lx\n",
								info.blksz, (offset / info.blksz) * info.blksz);

	debug("info->blksize:%lx, off_blk:%lx, size_blk:%lx\n", info.blksz, off_blk, size_blk);

	/*if size > buffer_size, it would only load buffer_size, and return offset*/
	if (size_blk * info.blksz > fastboot_buf_size){
		/* Read the boot image header */
		hdr_sectors = fastboot_buf_size / info.blksz;
	}else{
		hdr_sectors = size_blk;
	}

	res = blk_dread(dev_desc, off_blk, hdr_sectors, download_buffer);
	if (res != hdr_sectors) {
		fastboot_fail("cannot read data from blk dev", response);
		return 0;
	}

	/*return had read size*/
	fastboot_response("OKAY", response, "0x%08x", (u32)(hdr_sectors * info.blksz));
	return hdr_sectors * info.blksz;
}
