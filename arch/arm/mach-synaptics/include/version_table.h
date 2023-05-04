/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#ifndef _VERSION_TABLE_H_
#define _VERSION_TABLE_H_

#define PART_NAME_MAX_LEN	15
#define BLOCK0	"block0"
#define IMG2_NAME	"pre-bootloader"
#define IMG3_NAME	"bootimgs"
#define SYSCONF_NAME	"sysconf"
#define FLASHLESS_NAME	"flashless_data"

//DDR type
#define DDR_TYPE_DDR2	2
#define DDR_TYPE_DDR3	3

//DDR channel
#define DDR_CHANNEL_DUAL	2
#define DDR_CHANNEL_SINGLE	1

//CPU type
#define CPU_TYPE_B		0xb
#define CPU_TYPE_C		0xc

typedef struct _version_t_ {
	union {
		struct {
			unsigned minor_version;
			unsigned major_version;
		};
		unsigned long long version;
	};
} version_t __attribute__ ((aligned (4)));

typedef enum data_type_t_ {
	DATA_TYPE_NORMAL,
	DATA_TYPE_OOB,
	DATA_TYPE_RAW
}data_type_t;

typedef struct _sub_img_info_t_ {
	char name[PART_NAME_MAX_LEN+1];
	unsigned long long size;
	unsigned crc;
	version_t version;

	unsigned reserved_blocks; // refer to Notes*
	unsigned chip_start_blkind;
	unsigned chip_num_blocks;
	unsigned data_type;
	unsigned char reserved[12]; //64 bytes aligned
} sub_img_info_t;

typedef struct _img_hdr_t_ {
	unsigned magic;
	version_t version;

	unsigned page_size;
	unsigned oob_size;
	unsigned pages_per_block;
	unsigned blks_per_chip;

	unsigned num_sub_images;
	unsigned char ddr_type		: 4;
	unsigned char ddr_channel	: 4;
	unsigned char cpu_type[2];
	unsigned char reserved[29]; //64 bytes aligned
	sub_img_info_t sub_image[];
} img_hdr_t;

typedef struct _ver_table_entry_t_ {
	char name[PART_NAME_MAX_LEN+1];
	version_t part1_version;
	unsigned part1_start_blkind;
	unsigned part1_blks;
	version_t part2_version;
	unsigned part2_start_blkind;
	unsigned part2_blks;
} ver_table_entry_t;

typedef struct _version_table_t_ {
	unsigned int magic;
	unsigned int ou_status;
	unsigned int num_entries;
	ver_table_entry_t table[];
} version_table_t;

#endif
