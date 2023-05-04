/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#include <common.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <command.h>
#include <console.h>
#include <watchdog.h>
#include <malloc.h>
#include "nand_drv.h"

#define MAX_BLK_SIZE 0x100000 //include oob
static uint32_t blk_buf[MAX_BLK_SIZE / 4];

#ifndef CONFIG_NAND_BOOT_PARTITION_SIZE
#define CONFIG_NAND_BOOT_PARTITION_SIZE 0x40000
#endif

static char * get_manuf_name(uint8_t id)
{
	int i = 0;
	for(i = 0; ; i++) {
		if(nand_manuf_ids[i].id == 0x0)
			return NULL;
		if(nand_manuf_ids[i].id == id)
			return nand_manuf_ids[i].name;
	}
}

static int get_nand_flash_ids_index(uint8_t * id)
{
	int index = -1;
	int i = 0, j = 0;
	for(i = 0; ; i++) {
		if(NULL == nand_flash_ids[i].name)
			return -1; // didn't find
		for(j = 0; j < NAND_MAX_ID_LEN; j++) {
			if(nand_flash_ids[i].id[j] != id[j])
				break;
		}
		if(j == NAND_MAX_ID_LEN) {
			index = i;
			break;
		}
	}
	return index;
}

static int do_nand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *cmd;
	int i, read, ret = 0, is_oob = 0;
	uint32_t offset, boundary, addr, size, blks, rw_size, rw_offset;
	static uint32_t blk_size, page_size, ecc_strength, scrambler_en, oob_size;
	uint32_t force_write = 0, force_erase = 0;
	uint32_t mem_addr = 0x2000000;
	uint32_t id_size = 0;
	uint8_t id[8];
	char * manuf = NULL;
	int nand_ids_index = 0;

	/* at least two arguments please */
	if (argc < 2)
		goto usage;

	cmd = argv[1];

	if (strncmp(cmd, "test", 4) == 0) {
		//for speed test
		offset = 0x900000;
		printf("begin read\n");
		for(i = 0; i < 64; i ++) {
			mv_nand_read_block(offset + i*(0x100000), (uint8_t *)blk_buf, 0x100000);
		}
		printf("end read\n");
		return 0;
	}

	if (strncmp(cmd, "init2", 5) == 0) {

		diag_Nand_Read_ID(mem_addr, &id_size);
		for(i = 0; i < id_size; i++) {
			id[i] = *(uint8_t *)(uintptr_t)(mem_addr++);
		}

		manuf = get_manuf_name(id[0]);
		if(NULL == manuf) {
			printf("UNKNOW MANUFACTURER!!!\n");
			return -1;
		}
		nand_ids_index = get_nand_flash_ids_index(id);
		if(nand_ids_index < 0) {
			printf("UNSUPPORTED NAND TYPE!!!\n");
			for(i = 0; i < id_size; i++) {
				printf("%02x ", id[i]);
			}
			printf("\n");
			return -1;
		}

		printf("Device found, Manufacturer ID: 0x%02x, Chip ID: 0x%02x\n",
							nand_flash_ids[nand_ids_index].mfr_id, nand_flash_ids[nand_ids_index].dev_id);
		printf("%s %s\n", manuf, nand_flash_ids[nand_ids_index].name);
		printf("Total size: %d MiB, erase size: %d, page size: %d\n",
							nand_flash_ids[nand_ids_index].chipsize,
							nand_flash_ids[nand_ids_index].erasesize,
							nand_flash_ids[nand_ids_index].pagesize);

		blk_size = nand_flash_ids[nand_ids_index].erasesize;
		page_size = nand_flash_ids[nand_ids_index].pagesize;
		ecc_strength = 48;  //fixed according to G
		scrambler_en = 0;
		oob_size = 32;      //max supported oob size

		//printf("nand init %d %d %d %d %d\n", blk_size, page_size,
		//		ecc_strength, scrambler_en, oob_size);
		nand_drv_open(blk_size, page_size, ecc_strength, scrambler_en, oob_size);
		printf("nand init done\n");
		return 0;
	}

	if (strncmp(cmd, "init", 4) == 0) {
		if (argc < 7)
			goto usage;

		blk_size = (uint32_t)simple_strtoul(argv[2], NULL, 10);
		page_size = (uint32_t)simple_strtoul(argv[3], NULL, 10);
		ecc_strength = (uint32_t)simple_strtoul(argv[4], NULL, 10);
		scrambler_en = (uint32_t)simple_strtoul(argv[5], NULL, 10);
		oob_size = (uint32_t)simple_strtoul(argv[6], NULL, 10);

		printf("nand init %d %d %d %d %d\n", blk_size, page_size,
						ecc_strength, scrambler_en, oob_size);
		nand_drv_open(blk_size, page_size, ecc_strength, scrambler_en, oob_size);
		printf("nand init done\n");
		return 0;
	}

	if (strncmp(cmd, "erase", 5) == 0) {
		unsigned int blk_addr = 0;
		unsigned int blk_cnt = 0;
		if (argc < 4)
			goto usage;

		offset = (uint32_t)simple_strtoul(argv[2], NULL, 16);
		blks = (uint32_t)simple_strtoul(argv[3], NULL, 16);

		if(offset % blk_size) {
			printf("the address to erase should be block size aligned!\n");
			return 1;
		}

		if(strncmp(cmd, "erase.force", 11) == 0)
			force_erase = 1;

		printf("nand %s 0x%x %d\n", cmd, offset, blks);

		if(force_erase) {
			blk_addr = offset;
			for(; blks > 0;) {
				blk_cnt = (blks > 256) ? 256 : blks;
				ret = mv_nand_erase_multiblk(blk_addr, blk_cnt);
				if (ret != 0)
					return 1;
				blk_addr += (blk_cnt * blk_size);
				blks -= blk_cnt;
			}
			goto erase_done;
		}

		assert(blk_size != 0);
		assert((offset % blk_size) == 0);

		for (i = 0; i < blks; i++) {
			blk_addr = offset + i * nand_block_size();
			if (blk_addr >= (nand_block_size() + 8 * CONFIG_NAND_BOOT_PARTITION_SIZE)) {
				if (mv_nand_block_bad(blk_addr, 0)) {
					printf("Ignore the bad block 0x%08x\n", blk_addr);
					continue;
				}
			}
			ret = mv_nand_erase(blk_addr);
			// we don't mark bad block for block0 and boot 1 to boot 8 because oob option
			// is not available. so even the erase fail, we have to ignore it
			// don't worry about the burning, we'll read back and check the data
			// and we store same copies of preboot in boot 1 to boot 8
			if ((ret != 0) && (blk_addr >= (nand_block_size() + 8 * CONFIG_NAND_BOOT_PARTITION_SIZE))) {
				// FIXME: more thinking
				// if the erase of the block after 9 fail, how about marking it as bad block?
				// but can it be marked(write) bad for even erase op can't execute?
				printf("erase op failed at address 0x%08x\n", blk_addr);
				return 1;
			}
		}

erase_done:
		printf("nand %s 0x%x %d done\n", cmd, offset, blks);
		return 0;
	}

	if (strncmp(cmd, "read", 4) == 0 || strncmp(cmd, "write", 5) == 0) {
		uint32_t real_blksize = blk_size;
		uint32_t pages_perblk = blk_size/page_size;
		uint32_t retry = 0;

		if (argc < 6)
			goto usage;

		if(strncmp(cmd, "write.oob", 9) == 0)
			is_oob = 1;
		if(strncmp(cmd, "read.oob", 8) == 0)
			is_oob = 1;

		addr = (ulong)simple_strtoul(argv[2], NULL, 16);
		offset = (ulong)simple_strtoul(argv[3], NULL, 16);
		boundary = (ulong)simple_strtoul(argv[4], NULL, 16);
		size = (ulong)simple_strtoul(argv[5], NULL, 16);

		if((offset < (blk_size + 8 * CONFIG_NAND_BOOT_PARTITION_SIZE)) && is_oob) {
			printf("block0 and boot 1 to 8 don't touch oob %x\n", offset);
			return 1;
		}

		if(argc == 7)
			force_write = simple_strtoul(argv[5], NULL, 10);

		printf("nand %s 0x%x 0x%x 0x%x %d\n", cmd, addr, offset,
				boundary, size);
		assert(blk_size != 0);
		assert((offset % blk_size) == 0);
		read = strncmp(cmd, "read", 4) == 0;

		if(is_oob)
			real_blksize = blk_size + oob_size * pages_perblk;

		//for real_blksize may not be power of 2
		//so change function to caculate blks
		//blks = (size + blk_size - 1) / blk_size;
		blks = size / real_blksize;
		if(size % real_blksize)
			blks ++;

		if (!read) {
			invalidate_dcache_range(addr, addr + size);
		}

		rw_offset = offset;
		for (i = 0; i < blks; i++) {
			rw_size = size > real_blksize ? real_blksize : size;
			if (rw_offset >= (blk_size + 8 * CONFIG_NAND_BOOT_PARTITION_SIZE)) {
				if(force_write == 0) {
					while ((rw_offset < boundary) && mv_nand_block_bad(rw_offset, 0)){
						printf("nand %s skip bad blk at 0x%x\n", cmd, rw_offset);
						rw_offset += blk_size;
					}
				}
			}

			if (rw_offset >= boundary) {
					printf("try write to 0x%x crossed the partition boundary, abort!\n",
							rw_offset);
					return 1;
			}

			if (read) {
				if(is_oob) {
					ret = mv_nand_read_block_withoob(rw_offset,
							(uint8_t *)(uintptr_t)addr + i * real_blksize,
							rw_size);
				} else {
					ret = mv_nand_read_block(rw_offset,
							(uint8_t *)(uintptr_t)addr + i * blk_size,
							rw_size);
				}
				if (ret != rw_size)
					return 1;
			} else {
				mv_nand_erase(rw_offset);
				if(is_oob) {
					ret = mv_nand_write_block_withoob(rw_offset,
							(const uint8_t *)(uintptr_t)addr + i * real_blksize,
							rw_size);
				} else {
					ret = mv_nand_write_block(rw_offset,
							(const uint8_t *)(uintptr_t)addr + i * blk_size,
							rw_size);
				}
				if (ret != rw_size)
					return 1;

				if (rw_offset == 0) {
					// don't check block0
					return 0;
				}

				if(force_write <= 1) {
read_block:
				//from the spec, "No erase operation is allowed to detected bad blocks"
				if(is_oob) {
					ret = mv_nand_read_block_withoob(rw_offset, (uint8_t *)blk_buf, rw_size);
				} else {
					ret = mv_nand_read_block(rw_offset, (uint8_t *)blk_buf, rw_size);
				}
				if (ret != rw_size)
					return 1;

				if (memcmp((void *)(uintptr_t)addr + i * real_blksize,
							(void *)blk_buf, rw_size) != 0) {
					if (retry++ < 3)
						goto read_block;

					printf("nand write at 0x%x size %d failed\n", rw_offset, rw_size);

					printf("mark block 0x%x as bad\n", rw_offset);
					mv_nand_erase(rw_offset); // erase first then write bbm
					ret = mv_nand_mark_badblock(rw_offset, 0);
					if (ret != 0)
						return 1;
					rw_offset += blk_size;
					retry = 0;
					continue;
				}
				}
			}

			size -= rw_size;
			rw_offset += blk_size;
			// only used to show a progress bar
			if(0 == i%32)
				printf("\n");
			printf("#");
		}

		printf("\nnand %s 0x%x 0x%x done\n", cmd, addr, offset);
		return 0;
	}

usage:
	return CMD_RET_USAGE;
}

static char nand_help_text[] =
"nand init - block_size page_size, ecc_stregth, scrambler_en, oob_size\n"
"nand init2\n"
"nand read - addr offset boundary size\n"
"nand write - addr offset boundary size\n"
"    read/write 'size' bytes starting at 'offset', no exceed partition 'boundary'\n"
"    to/from memory address 'addr', skipping bad blocks.\n"
"    'offset must align to block size.\n"
"nand erase offset blks - erase number of 'blks' blocks from 'offset'\n"
"    'offset must align to block size.\n"
"";

U_BOOT_CMD(
		nand, CONFIG_SYS_MAXARGS, 1, do_nand,
		"NAND sub-system", nand_help_text
	  );
