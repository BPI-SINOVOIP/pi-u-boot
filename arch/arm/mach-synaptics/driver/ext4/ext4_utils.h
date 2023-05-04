/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _EXT4_UTILS_H_
#define _EXT4_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <common.h>
#include <blk.h>
#include <malloc.h>
#include <linux/types.h>

#include "ext4_sb.h"

extern int force;

#define warn(fmt, args...) do { fprintf(stderr, "warning: %s: " fmt "\n", __func__, ## args); } while (0)
#define error_errno(s, args...) printf(s, ##args)
#define critical_error(s, args...) printf(s, ##args)
#define critical_error_errno(s, args...) printf(s, ##args)

#define EXT4_JNL_BACKUP_BLOCKS 1

#ifndef min /* already defined by windows.h */
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define EXT4_ALIGN(x, y) ((y) * DIV_ROUND_UP((x), (y)))

struct block_group_info;
struct xattr_list_element;

struct ext2_group_desc {
	u32 bg_block_bitmap;
	u32 bg_inode_bitmap;
	u32 bg_inode_table;
	u16 bg_free_blocks_count;
	u16 bg_free_inodes_count;
	u16 bg_used_dirs_count;
	u16 bg_flags;
	u32 bg_reserved[2];
	u16 bg_reserved16;
	u16 bg_checksum;
};

struct fs_aux_info {
	struct ext4_super_block *sb;
	struct ext4_super_block *sb_block;
	struct ext4_super_block *sb_zero;
	struct ext4_super_block **backup_sb;
	struct ext2_group_desc *bg_desc;
	struct block_group_info *bgs;
	struct xattr_list_element *xattrs;
	u32 first_data_block;
	u64 len_blocks;
	u32 inode_table_blocks;
	u32 groups;
	u32 bg_desc_blocks;
	u32 default_i_flags;
	u32 blocks_per_ind;
	u32 blocks_per_dind;
	u32 blocks_per_tind;
};

extern struct fs_info info;
extern struct fs_aux_info aux_info;
extern struct sparse_file *ext4_sparse_file;
extern struct block_allocation *base_fs_allocations;

static inline int log_2(int j)
{
	int i;

	for (i = 0; j > 0; i++)
		j >>= 1;

	return i - 1;
}

int ext4_bg_has_super_block(int bg);
void write_ext4_image(struct blk_desc *dev, lbaint_t size);
void ext4_create_fs_aux_info(void);
void ext4_free_fs_aux_info(void);
void ext4_fill_in_sb(void);
void ext4_create_resize_inode(void);
void ext4_create_journal_inode(void);
void ext4_update_free(void);
void ext4_queue_sb(u64 start_block, struct ext4_super_block *sb);
u16 ext4_crc16(u16 crc_in, const void *buf, int size);

typedef void (*fs_config_func_t)(const char *path, int dir, const char *target_out_path,
        unsigned *uid, unsigned *gid, unsigned *mode, uint64_t *capabilities);


int make_ext4fs_internal(struct blk_desc *dev, lbaint_t start);

#ifdef __cplusplus
}
#endif

#endif
