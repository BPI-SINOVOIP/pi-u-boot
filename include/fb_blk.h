// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#ifndef _FB_BLK_H_
#define _FB_BLK_H_

struct blk_desc;
struct disk_partition;

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
			       struct disk_partition *part_info,
			       char *response);

/**
 * fastboot_blk_flash_write() - Write image to blk device for fastboot
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_blk_flash_write(const char *cmd, void *download_buffer,
			      u32 download_bytes, char *response);
/**
 * fastboot_blk_flash_erase() - Erase blk device for fastboot
 *
 * @cmd: Named partition to erase
 * @response: Pointer to fastboot response buffer
 */
void fastboot_blk_erase(const char *cmd, char *response);

/**
 * fastboot_blk_read() - load data from blk device for fastboot
 *
 * @part: Named partition to erase
 * @response: Pointer to fastboot response buffer
 */
u32 fastboot_blk_read(const char *part, u32 offset,
			void *download_buffer, char *response);

#endif
