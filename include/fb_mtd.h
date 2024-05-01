// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Spacemit, Inc
 */

#ifndef _FB_MTD_H_
#define _FB_MTD_H_

#include <jffs2/load_kernel.h>
#include <mtd.h>


/**
 * @brief find mtd part
 * 
 * @param partname: mtd part name.
 * @param mtd: mtd dev.
 * @param part: mtd dev part info.
 * @return int 
 */
int fb_mtd_lookup(const char *partname, struct mtd_info **mtd,
					struct part_info **part);

/**
 * @brief erase mtd partition
 * 
 * @param mtd: mtd dev.
 * @param erase_size: the size to erase at mtd dev.
 * @return int 
 */
int _fb_mtd_erase(struct mtd_info *mtd, u32 erase_size);


/**
 * @brief write data to mtd part.
 * 
 * @param mtd: mtd dev.
 * @param buffer: the data would write from buffer.
 * @param offset: the offset to write to the mtd dev.
 * @param length: the length to write to the mtd dev.
 * @param written
 * @return int 
 */
int _fb_mtd_write(struct mtd_info *mtd, void *buffer, u32 offset,
			  size_t length, size_t *written);

/**
 * @brief read data to mtd part.
 * 
 * @param mtd: mtd dev.
 * @param buffer: the data would read to buffer.
 * @param offset: the offset to read from the mtd dev.
 * @param length: the length to read from the mtd dev.
 * @param written
 * @return int 
 */
int _fb_mtd_read(struct mtd_info *mtd, void *buffer, u32 offset,
			  size_t length, size_t *written);

/**
 * fastboot_mtd_get_part_info() - Lookup MTD partion by name
 *
 * @part_name: Named device to lookup
 * @part_info: Pointer to returned part_info pointer
 * @response: Pointer to fastboot response buffer
 */
int fastboot_mtd_get_part_info(const char *part_name,
				struct part_info **part_info, char *response);

/**
 * fastboot_mtd_flash_write() - Write image to MTD for fastboot
 *
 * @cmd: Named device to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_mtd_flash_write(const char *cmd, void *download_buffer,
			       u32 download_bytes, char *response);

/**
 * fastboot_mtd_flash_erase() - Erase MTD for fastboot
 *
 * @cmd: Named device to erase
 * @response: Pointer to fastboot response buffer
 */
void fastboot_mtd_flash_erase(const char *cmd, char *response);


/**
 * fastboot_mtd_flash_read() - load data from mtd for fastboot
 *
 * @part_name: Named partition to read
 * @response: Pointer to fastboot response buffer
 */
u32 fastboot_mtd_flash_read(const char *part_name, u32 offset,
					void *download_buffer, char *response);

#endif
