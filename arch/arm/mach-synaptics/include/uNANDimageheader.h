/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#ifndef _IMAGEHEADER_H_
#define _IMAGEHEADER_H_

typedef enum {
	UNKNOWN_IMG_HEADER,
	GLOBAL_IMG_HEADER,
	SINGLE_IMG_HEADER,
} imgheader_t;

typedef enum {
	UNKNOWN_IMG_TYPE,
	RAW_IMG_IMAGE2PLUS3,
	RAW_IMG_KEYSTORE,
	RAW_IMG_FS_WITH_OOB,
	RAW_IMG_FS_NO_OOB,
	RAW_IMG_BOXINFO,
	RAW_IMG_LINUX_BOOT_PARAM,
	RAW_IMG_SYSTEM_CONFIG,
	RAW_IMG_APPFS_NO_OOB,
	RAW_IMG_ROOTFS_NO_OOB,
	RAW_IMG_ROOTFS_TGZ_FOR_OU,
	RAW_IMG_RECOVERY_ROOTFS,
	RAW_IMG_LINUX_BOOT_PARAM_UBOOT,
	RAW_IMG_ROOTFS_TGZ,
	RAW_IMG_ERASE_PART,
} rawimg_t;

typedef struct {
	imgheader_t hdr_type;

	int num_single_image;
	int uNAND_image_size;
	int hdr_crc;

	unsigned char reserved[16];/* 32 bytes aligned */
} global_img_hdr_t;

typedef struct {
	imgheader_t hdr_type;

	int raw_image_size;
	int padding_size;
	int page_data_size;
	int page_oob_size;
	int num_page_per_block;
	rawimg_t raw_img_type;

	int partition_index;
	int hdr_crc;
	int raw_img_crc;

	unsigned int major_version;
	unsigned int minor_version;

	unsigned int ou_when_pt_changes;

	unsigned char reserved[12];/* 64 bytes aligned */
} single_img_hdr_t;

//extern ulong crc32 (ulong, const unsigned char *, uint);

/* special size referring to all the remaining space in a partition */
#define SIZE_REMAINING (~0U)
#define OFFSET_CONTINUOUS (~0U)
#define MAX_PARTITION_NUM 16
#define PART_INDEX_ANY MAX_PARTITION_NUM

#endif
