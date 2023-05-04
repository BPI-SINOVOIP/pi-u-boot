/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2011(C)
 * Author: winters jiang, wj@iessys.com
 *
 * Copyright (C) 2019 Synaptics Incorporated
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 *
 */

#include <common.h>
#include <command.h>
#include <console.h>
#include <fs.h>
#include <malloc.h>
#include <net.h>
#include <net/tftp.h>
#include <part.h>
#include <part_efi.h>
#include <usb.h>
#include <getopt.h>
#include <nand.h>
#include <version_table.h>
#include <linux/ctype.h>
#include "uNANDimageheader.h"
#include "nand_drv.h"
#include "Galois_memmap.h"
#include "global.h"

#define UNUSED(var) do { (void)(var); } while(0)
#define MAX_NANDIMG_SIZE	0x10000000

enum image_src_type {
	TFTP,
	USB,
	USBSLAVE
};

static char * srcstr[] = {"tftp", "usbh", "usbs"};

/* find a string in a list, the strings is split by ",", such as
 * find "boot" in "bootloader,kernel,boot", it should match "boot",
 * not "bootloader".
 * @retval  NULL	can't find the string.
 * @retval !NULL	the pointer to the found string
 */
static const char *find_str_in_list(const char *list, const char *str, int split)
{
	int len;
	if (NULL == list || NULL == str)
		return NULL;

	if ('\0' == split)
		split = ',';

	len = strlen(str);
	while (*list) {
		const char *next_str = NULL;
		if ((split == list[len] || '\0' == list[len]) &&
			(0 == strncmp(list, str, len))) {
			return list;
		}
		/* goto next string */
		next_str = strchr(list, split);
		if (NULL == next_str)
			break;
		list = next_str + 1;
	}
	return NULL;
}

#if defined(CONFIG_CMD_USB) && defined(CONFIG_CMD_FAT)
extern int usb_started;
#endif

//#ifdef CONFIG_CMD_NAND
#define MAX_READ_SIZE	0x8000000
#define YAFFS_OOB_SIZE			(32)	// better to get by mv_nand_oob_size()
#define MAGIC_NUMBER 0xD2ADA3F1

#ifndef CONFIG_NAND_BOOT_PARTITION_SIZE
#define CONFIG_NAND_BOOT_PARTITION_SIZE 0x40000
#endif

static unsigned int g_block_size;
static unsigned int g_page_size;
static unsigned int g_oob_size;
static unsigned int g_ecc_strength;
static unsigned int g_scrambler_en;

int compare_buff(const char * src, const char * dst, unsigned int size)
{
	unsigned int i,j;

	for(i=0;i<size;i++)
	{
		if(dst[i] != src[i])
		{
			printf("\ndata is not same at 0x%08x.\n", i);
			printf("buffer 1 @ %08x:", (int)(uint64_t)src);
			for(j=0;j<64;j++)
				printf("%02x ", src[i+j]);
			printf("\n");
			printf("buffer 2 @ %08x:", (int)(uint64_t)dst);
			for(j=0;j<64;j++)
				printf("%02x ", dst[i+j]);
			printf("\n");
			return 1;
		}
	}
	return 0;
}

static const char *get_data_type_name(data_type_t data_type)
{
	switch (data_type) {
	case DATA_TYPE_NORMAL:
		return "normal";
	case DATA_TYPE_OOB:
		return "oob";
	case DATA_TYPE_RAW:
		return "raw";
	default:
		return "wrong";
	}
}

/* may used in future
static unsigned int nand_page_data_size(data_type_t data_type)
{
	unsigned int page_size = g_page_size;
	unsigned int oob_size = g_oob_size;
	switch (data_type) {
	case DATA_TYPE_NORMAL:
		return page_size;
	case DATA_TYPE_OOB:
		return page_size + YAFFS_OOB_SIZE;
	case DATA_TYPE_RAW:
		return page_size + oob_size;
	default:
		return page_size;
	}
}

static unsigned int nand_block_data_size(data_type_t data_type)
{
	unsigned int pages_per_block = g_block_size / g_page_size;
	int page_data_size = nand_page_data_size(data_type);

	return g_block_size;//page_data_size * pages_per_block;
}
*/

//FIXME: only support DATA_TYPE_NORMAL
int do_nand_read(unsigned int addr, unsigned int offset, unsigned int boundary, unsigned size)
{
	char cmd[512];
	memset(cmd, 0, 512);
	sprintf(cmd, "nand read %x %x %x 0x%x", addr, offset, boundary, size);
	printf("%s\n", cmd);
	return run_command(cmd, 0);
}

int do_nand_read_withoob(unsigned int addr, unsigned int offset, unsigned int boundary, unsigned size)
{
	char cmd[512];
	memset(cmd, 0, 512);
	sprintf(cmd, "nand read.oob %x %x %x 0x%x", addr, offset, boundary, size);
	printf("%s\n", cmd);
	return run_command(cmd, 0);
}

int do_nand_write(unsigned int addr, unsigned int offset, unsigned int boundary, unsigned size)
{
	char cmd[512];
	memset(cmd, 0, 512);
	sprintf(cmd, "nand write %x %x %x 0x%x", addr, offset, boundary, size);
	printf("%s\n", cmd);
	return run_command(cmd, 0);
}

int do_nand_write_withoob(unsigned int addr, unsigned int offset, unsigned int boundary, unsigned size)
{
	char cmd[512];
	memset(cmd, 0, 512);
	sprintf(cmd, "nand write.oob %x %x %x 0x%x", addr, offset, boundary, size);
	printf("%s\n", cmd);
	return run_command(cmd, 0);
}

int do_nand_erase(unsigned int offset, unsigned size)
{
	char cmd[512];
	memset(cmd, 0, 512);
	sprintf(cmd, "nand erase %x 0x%x", offset, size/g_block_size);
	printf("%s\n", cmd);
	return run_command(cmd, 0);
}

loff_t partition_off=0;

static int burn_sub_image_inc(sub_img_info_t* img_hdr, const char* img_buff, unsigned buff_size,
				 int verify)
{
	int i=0,j=0;
	loff_t ret;
	unsigned int block_size = g_block_size;
	//unsigned int page_size = g_page_size;
	//unsigned int oob_size = g_oob_size;
	//unsigned int block_data_size = nand_block_data_size(img_hdr->data_type);
	//unsigned int checksum1=0, checksum2=0;;
	unsigned int boundary = 0;

	if(img_hdr->size == 0)
		return 0;
	if(img_hdr->size > (img_hdr->chip_num_blocks - img_hdr->reserved_blocks) * block_size) {
		printf("error: %s image size exceeds avalible partition size.\n", img_hdr->name);
		return -1;
	}
	if(partition_off % block_size) {
		return -1;
	}

	boundary = (img_hdr->chip_start_blkind + img_hdr->chip_num_blocks) * block_size;

	if(strcmp((char*)img_hdr->name, IMG2_NAME) == 0) {
		/*
        if(verify) {
            printf("computing write crc...");
            checksum1 = crc32(0, (unsigned char*)img_buff, buff_size);
            printf(" %08x\n", checksum1);
        }
        */

		/* check the boot partition size
		 * 1. boot partition size should be equal or bigger than block size
		 * 2. boot partition size should be block size or multiple of block size
		 * 3. boot area should be multiple of boot partition size
		 */
		if((CONFIG_NAND_BOOT_PARTITION_SIZE < block_size) ||
			(CONFIG_NAND_BOOT_PARTITION_SIZE % block_size) ||
			img_hdr->chip_num_blocks % (CONFIG_NAND_BOOT_PARTITION_SIZE / block_size)) {
			printf("invalid boot partition size 0x%x(block_size = 0x%x)\n",
				CONFIG_NAND_BOOT_PARTITION_SIZE, block_size);
			return -1;
		}

        if(img_hdr->size > CONFIG_NAND_BOOT_PARTITION_SIZE) {
			printf("%s size %u is bigger than boot partition size.\n",
				img_hdr->name, (unsigned)img_hdr->size);
			return -1;
		}
		for(i = img_hdr->chip_start_blkind * block_size;
			i < (img_hdr->chip_start_blkind + img_hdr->chip_num_blocks) * block_size;
			i = i + CONFIG_NAND_BOOT_PARTITION_SIZE) {
			/* don't check bad block from 0 to 9
			if(mv_nand_block_bad((loff_t)i*block_size, 0)) {
				printf("bad block at 0x%08x, skip\n", i*block_size);
				continue;
			}
			*/
			boundary = i + CONFIG_NAND_BOOT_PARTITION_SIZE; // the boundary of pre-bootloader is boot partitoin size
			if(0 == do_nand_write((uint32_t)(uint64_t)img_buff, i, boundary, buff_size)) {
				j++;
				printf("###success %d\n", j);
			} else {
				printf("###failed %d\n", j);
			}
		}
		if(j == 0) {
			printf("Write %s error.\n", img_hdr->name);
			return -1;
		}
/*
	}else if(strcmp((char*)img_hdr->name, SYSCONF_NAME) == 0) {
		loff_t start, end;

		//write first part
		start = (loff_t)img_hdr->chip_start_blkind * block_size + partition_off;
		end = (loff_t)(img_hdr->chip_start_blkind + img_hdr->chip_num_blocks/2) * block_size;

		ret = do_nand_write(img_buff, start, end, buff_size);
		if(ret != 0)
			return -1;
		else if((end - ret) < img_hdr->reserved_blocks * block_size) {
			printf("Unused blocks %u is less than required reserve blocks %d",
				(unsigned int)(end-ret)/block_size, img_hdr->reserved_blocks);
			return -1;
		}
#if 1
		if(verify) {
            char *in_buf;
            char *out_buf;
            unsigned int len;
            loff_t rpos;
            int readlen;
			int ret;

            len = buff_size;
            rpos = start;
            in_buf = img_buff;
            out_buf = malloc(block_size);
            if(out_buf == NULL) {
                printf("%s malloc failed\n", __FUNCTION__);
                return -1;
            }
            while(len) {
                if(mv_nand_block_bad(rpos, 0)) {
                    rpos += block_size;
                    continue;
                }
				if(len >= block_size)
					readlen = block_size;
				else
					readlen = len;

                printf("Reading NAND at address 0x%09LX\r", rpos);
                ret = do_nand_read(out_buf, rpos, end, readlen);
                if(ret != 0) {
                    printf("read error ar 0x%09LX\n", rpos);
                    free(out_buf);
                    return -1;
                }
                if(compare_buff(in_buf, out_buf, readlen)) {
                    free(out_buf);
                    return -1;
                }
                in_buf += readlen;
                len -= readlen;
                rpos += block_size;
            }
            printf("\n");
            free(out_buf);
		}
#endif

		//write second part
		start = end + partition_off;
		end = (loff_t)(img_hdr->chip_start_blkind + img_hdr->chip_num_blocks) * block_size;

		ret = do_nand_write(img_buff, start, end, buff_size);
		if(ret != 0)
			return -1;
		else if((end - ret) < img_hdr->reserved_blocks * block_size) {
			printf("Unused blocks %u is less than required reserve blocks %d",
				(unsigned int)(end-ret)/block_size, img_hdr->reserved_blocks);
			return -1;
		}
		partition_off += ret - start;
#if 1
		if(verify) {
            char *in_buf;
            char *out_buf;
            unsigned int len;
            loff_t rpos;
            int readlen;
			int ret;

            len = buff_size;
            rpos = start;
            in_buf = img_buff;
            out_buf = malloc(block_size);
            if(out_buf == NULL) {
                printf("%s malloc failed\n", __FUNCTION__);
                return -1;
            }
            while(len) {
                if(mv_nand_block_bad(rpos, 0)) {
                    rpos += block_size;
                    continue;
                }
				if(len >= block_size)
					readlen = block_size;
				else
					readlen = len;
                printf("Reading NAND at address 0x%09LX\r", rpos);
                ret = do_nand_read(out_buf, rpos, end, readlen);
                if(ret != 0) {
                    printf("read error ar 0x%09LX\n", rpos);
                    free(out_buf);
                    return -1;
                }
                if(compare_buff(in_buf, out_buf, readlen)) {
                    free(out_buf);
                    return -1;
                }
                in_buf += readlen;
                len -= readlen;
                rpos += block_size;
            }
            printf("\n");
            free(out_buf);
		}
#endif
		return 0;
*/
	} else {
		loff_t start, end;

		start = (loff_t)img_hdr->chip_start_blkind * block_size;// + partition_off;
		end = (loff_t)(img_hdr->chip_start_blkind + img_hdr->chip_num_blocks) * block_size;


		if (img_hdr->data_type == DATA_TYPE_OOB) {
			ret = do_nand_write_withoob((uint32_t)(uint64_t)img_buff, start, end, buff_size);
		} else {
			ret = do_nand_write((uint32_t)(uint64_t)img_buff, start, end, buff_size);
		}
		if(ret != 0)
			return -1;
		/*
		else if((end - ret) < img_hdr->reserved_blocks * block_size) {
			printf("Unused blocks %u is less than required reserve blocks %d",
				(unsigned int)(end-ret)/block_size, img_hdr->reserved_blocks);
			return -1;
		}
		partition_off += ret - start;
		*/
	}
	return 0;
}

int do_list2nand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;
	int ret = 0;
	int error=0, develop_mode=0, verify=0;
	int force_erase = 0;
	int oob_offset = -1;
	char *img_file = NULL;
	unsigned int mem_addr;
	img_hdr_t *nimg_hdr;
	sub_img_info_t *sub_img_hdr = NULL;
	unsigned int fpos;
	unsigned int src = 0;

	char img_name[512];
	unsigned int align_size = 0;
	unsigned int init_mode = 2;

	int r = 0;
	char cmd[512];

	const char *program_partition_list = NULL;
	const char *ignore_partition_list = NULL;

	char *var = malloc(MAX_NANDIMG_SIZE);
	if (!var)
		return CMD_RET_USAGE;
	mem_addr = (unsigned int)(uint64_t)var;
	nimg_hdr = (img_hdr_t *)(uint64_t)mem_addr;
	optind = 0;
	while (1) {
		static const char *short_options = "vmhfi:p:o:";
		int c = getopt(argc, argv, short_options);
		if (c == -1)
			break;
		switch (c) {
		case 'm':
			develop_mode = 1;
			break;
		case 'v':
			verify = 0;
			break;
		case 'f':
			force_erase = 1;
			break;
		case 'i':
			ignore_partition_list = optarg;
			develop_mode = 1;
			break;
		case 'p':
			program_partition_list = optarg;
			develop_mode = 1;
			break;
		case 'o':
			oob_offset = simple_strtol(optarg, NULL, 10);
			break;

		default:
			error = 1;
			break;
		}
	}

	int mimg = 0;
	if(strcmp(argv[0], "u2nand_whole") == 0) {
		src = 0;
	} else if(strcmp(argv[0], "usb2nand") == 0) {
		src = 1;
	} else if(strcmp(argv[0], "l2nand") == 0) {
		src = 2;
		hal_write32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_nfcEccClk), 0xa1);
		hal_write32((MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_nfcCoreClk), 0xa1);
		run_command("nand init2", 0);
	} else if (strcmp(argv[0], "m2nand") == 0) {
		mimg = 1;//image must be loaded to address argv[optind]
	}

	if ((argc - optind) > 2 || error) {
		cmd_usage(cmdtp);
		goto ERROR;
	}

	if(mimg == 0) {
		if(argc - optind == 1)
			img_file = argv[optind];
		else
			img_file = "/u2nandimg/uNAND.img";
	} else {
		mem_addr = simple_strtoul(argv[optind], NULL, 16);
		nimg_hdr = (img_hdr_t *)(uint64_t)mem_addr;
		if(argc - optind == 2)
			init_mode = simple_strtoul(argv[optind + 1], NULL, 16);
	}

	//FIXME: by default we use develop mode
	//develop_mode = 1;

	g_block_size = nand_block_size();
	g_page_size = nand_page_size();
	g_oob_size = nand_oob_size();
	g_ecc_strength = nand_ecc_strength();
	g_scrambler_en = nand_scramber_en();
	loff_t chip_size;

	if(!g_block_size || !g_page_size) {
		printf("please do nand init first\n");
		goto ERROR;
	}

	if(mimg == 0) {
		sprintf(cmd, "imgload %s %s 0x%p", srcstr[src], img_file, (void *)(uint64_t)mem_addr);
		ret = run_command(cmd, 0);
	}

	if (ret != 0) {
		printf("%s: can not get image %s\n", __func__, img_name);
		goto ERROR;
	}

	if(nimg_hdr->magic != MAGIC_NUMBER) {
		printf("MAGIC number does not match.\n");
		printf("Is this the new format NAND image?.\n");
		goto ERROR;
	}

	printf("\nHardware information in image:\n");
	printf("DDR%d, %d channel, CPU type %X\n",
		nimg_hdr->ddr_type, nimg_hdr->ddr_channel, nimg_hdr->cpu_type[0]);

	//FIXME
	chip_size = (loff_t)(nimg_hdr->blks_per_chip)* g_block_size;
	if((g_page_size != nimg_hdr->page_size) ||
		(g_block_size != nimg_hdr->pages_per_block * nimg_hdr->page_size) ||
		(chip_size != (loff_t)(nimg_hdr->blks_per_chip)* g_block_size)) {
		printf("NAND parameters in image are page_size=%u,"
			" pages_per_block=%u, blks_per_chip=%u.\n",
			nimg_hdr->page_size, nimg_hdr->pages_per_block,
			nimg_hdr->blks_per_chip);
		printf("NAND device parameters are page_size=%u,"
			" pages_per_block=%u, blks_per_chip=%u.\n",
			g_page_size, g_block_size/g_page_size,
			(unsigned int)(chip_size/g_block_size));
		printf("NAND parameters in image do not match this NAND chip.\n");
		goto ERROR;
	}

	//by default, skip this step
	if(!develop_mode) {
		printf("\nErase NAND chip...\n");
		printf("chip_size 0x%09LX\n", chip_size);
		ret =do_nand_erase((loff_t)0, (unsigned int)(chip_size));
		printf("erased done.\n");
	}

	fpos = sizeof(img_hdr_t) + sizeof(sub_img_info_t)*nimg_hdr->num_sub_images;
	for(i = 0;i < nimg_hdr->num_sub_images;i++) {
		char *sub_img_pos = NULL;
		sub_img_hdr = &nimg_hdr->sub_image[i];

		if (ctrlc()) {
			printf("\nAbort\n");
			goto ERROR;
		}

		printf("\n\n########################################################################\n");
		printf("\n\n################## %s ##################\n", sub_img_hdr->name);
		if(ignore_partition_list &&
				find_str_in_list(ignore_partition_list, sub_img_hdr->name, ',')) {
			printf("ignore partition %s\n", sub_img_hdr->name);
			fpos += sub_img_hdr->size;
			continue;
		}

		if(program_partition_list &&
				!find_str_in_list(program_partition_list, sub_img_hdr->name, ',')) {
			printf("skip partition %s\n", sub_img_hdr->name);
			fpos += sub_img_hdr->size;
			continue;
		}

		if(init_mode == 2) {
			if(strcmp((char*)sub_img_hdr->name, BLOCK0) == 0) {
				sprintf(cmd, "nand init %d %d 0 %d 0", g_block_size, g_page_size, g_scrambler_en);
			} else if(strcmp((char*)sub_img_hdr->name, IMG2_NAME) == 0) {
				sprintf(cmd, "nand init %d %d %d %d 0", g_block_size, g_page_size, g_ecc_strength, g_scrambler_en);
			} else {
				sprintf(cmd, "nand init %d %d %d %d %d", g_block_size, g_page_size, g_ecc_strength, g_scrambler_en, g_oob_size);
			}
		} else if(init_mode == 1) {
			sprintf(cmd, "nand init %d %d 0 0 0", g_block_size, g_page_size);
		}

		if(init_mode != 0) {
			r = run_command(cmd, 0);
			if(r)
				goto ERROR;
		}
		printf("\nburn %s, image size %u, partition start:%u, size %u, reserve %u, data type: %s, crc 0x%08x\n",
			sub_img_hdr->name, (int)sub_img_hdr->size,
			sub_img_hdr->chip_start_blkind, sub_img_hdr->chip_num_blocks,
			sub_img_hdr->reserved_blocks,
			get_data_type_name(sub_img_hdr->data_type), sub_img_hdr->crc);

		if(develop_mode) {
			ret = do_nand_erase((loff_t)sub_img_hdr->chip_start_blkind *
					g_block_size, sub_img_hdr->chip_num_blocks * g_block_size);
			printf("erased done.\n");
		}

		partition_off=0;

		/* find the subimg */
		sub_img_pos = (char *)nimg_hdr + fpos;
		fpos += sub_img_hdr->size;

		if (sub_img_hdr->crc) {
			unsigned int crc = 0;
			crc = crc32(0, (unsigned char *)sub_img_pos, sub_img_hdr->size);
			if (sub_img_hdr->crc != crc) {
				printf("subimg %s CRC verify failed, 0x%08x != 0x%08x(original)!\n",
					sub_img_hdr->name, crc, sub_img_hdr->crc);
				goto ERROR;
			}
		}

		partition_off=0;
		if (sub_img_hdr->data_type == DATA_TYPE_OOB) {
			printf("Take care!!! It's maybe a yaffs image\n");
			align_size = sub_img_hdr->size / (g_page_size + g_oob_size);
			align_size *= (g_page_size + g_oob_size);
			if(sub_img_hdr->size % (g_page_size + g_oob_size))
				align_size += (g_page_size + g_oob_size);
		} else {
			align_size = (sub_img_hdr->size + g_page_size - 1)/g_page_size;
			align_size *= g_page_size;
		}
		printf("subimg size = %x, align size = %x\n", (uint32_t)sub_img_hdr->size, align_size);
		// verify = 0 for command nand write has done the verify
		ret = burn_sub_image_inc(sub_img_hdr, sub_img_pos,
					align_size, verify);
		if(ret != 0)
		{
			printf("burn sub image %d failed.\n", i);
			//FIXME: even failure, continue
			goto ERROR;
		}
		printf("burn sub image %s done.\n", sub_img_hdr->name);
	}

	UNUSED(oob_offset);
	UNUSED(force_erase);
	free(var);
	printf("Congratulations! %s succeed!\n", argv[0]);
	return 0;

ERROR:
	free(var);
	return CMD_RET_USAGE;
}

int do_ncbad(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int start = 0, offset = 0;
	int num = 0, i = 0;

	g_block_size = nand_block_size();

	start = simple_strtol(argv[1], NULL, 16);
	num = simple_strtol(argv[2], NULL, 16);

	for(i = 0; i < num; i ++) {
		offset = start + i * g_block_size;
		printf("checking 0x%x\n", offset);
		if(mv_nand_block_bad(offset, 0) != 0)
			printf("bad block: offset = 0x%x\n", offset);
	}

	return 0;
}

static unsigned int blk_size, page_size, ecc_strength;

static void parse_nand_config(char *buf,
		unsigned int size,
		unsigned int *blk_size,
		unsigned int *page_size,
		unsigned int *ecc_strength)
{
	unsigned int i = 0;

	while(i < size) {
		if (strstr(buf, "nand_page_size=") == buf) {
			*page_size = simple_strtoull(buf + strlen("nand_page_size="), NULL, 10);
		}
		else if (strstr(buf, "nand_block_size=") == buf) {
			*blk_size = simple_strtoull(buf + strlen("nand_block_size="), NULL, 10);
		}
		else if (strstr(buf, "nand_ecc_strength=") == buf) {
			*ecc_strength = simple_strtoull(buf + strlen("nand_ecc_strength="), NULL, 10);
		}
		else if (strstr(buf, "nand_total_size=") == buf) {
		}

		while(*(buf + i) != '\n') i++;
		i++;
		buf += i;
	}

	printf("blk_size %d, page_size %d, ecc_strength %d\n",
			*blk_size, *page_size, *ecc_strength);
}

static void get_next_nand_partition(char *str,
		char *partition_name,
		unsigned int *start_blk,
		unsigned int *total_blks,
		char *type)
{
	int word_len = 0;
	char *p = str;
	char *q = NULL;

	while(!isblank(*p)) {
		word_len++;
		p++;
	}

	memcpy(partition_name, str, word_len);
	partition_name[word_len] = '\0';

	while(!isdigit(*p)) p++;
	*start_blk = simple_strtoull(p, NULL, 10);

	/*find next word*/
	while(isdigit(*p))p++;
	while(!isdigit(*p))p++;
	*total_blks = simple_strtoull(p, NULL, 10);

	while(isdigit(*p))p++;
	while(isblank(*p))p++;
	q = p;

	word_len = 0;
	while(!isblank(*q)) {
		word_len++;
		q++;
	}

	memcpy(type, p, word_len);
	type[word_len] = '\0';

	printf("partition_name %s, start_blk %d, total_blks %d, type %s\n",
			partition_name, *start_blk, *total_blks, type);
}

static int burn_one_partition(char *img_path,
		unsigned int ddr_addr,
		char *partition_name,
		unsigned int start_blk,
		unsigned int total_blks,
		char *type)
{
	int ret, i;
	char cmd[128];
	int include_oob = 0;
	unsigned int align_size = 0;
	unsigned int oob_size = 32; //fixme: hard code
	unsigned int imgsize;
	sprintf(cmd, "imgload  %s %s/%s.subimg 0x%x", srcstr[2], img_path, partition_name, ddr_addr);
	ret = run_command (cmd, 0);
	if (ret != 0) {
		printf("run cmd %s ret %d\n", cmd, ret);
		return ret;
	}
	imgsize = env_get_hex("filesize", 0);

	printf("imgsize=%x\n", imgsize);
	if(strncmp(type, "oob", 3) == 0) {
		printf("Take care!!! It's maybe a yaffs image\n");
		include_oob = 1;
	}

	if(include_oob) {
		align_size = imgsize / (page_size + oob_size);
		align_size *= (page_size + oob_size);
		if(imgsize % (page_size + oob_size))
			align_size += (page_size + oob_size);
	} else {
		align_size = (imgsize + page_size - 1)/page_size;
		align_size *= page_size;
	}
	printf("align_size=%x\n", align_size);
	if (start_blk == 0) {
		sprintf(cmd, "nand init %d %d 0 0 0", blk_size, page_size);
	} else if (start_blk == 1) {
		sprintf(cmd, "nand init %d %d %d 0 0", blk_size, page_size, ecc_strength);
	} else {
		sprintf(cmd, "nand init %d %d %d 0 %d", blk_size, page_size, ecc_strength, oob_size);
	}
	ret = run_command (cmd, 0);
	if (ret != 0) {
		printf("run cmd %s ret %d\n", cmd, ret);
		return ret;
	}

	if (start_blk == 1) {
		for (i = 0; i < 8; i++) {
			unsigned int boot_partition = (start_blk + i) * blk_size;
			sprintf(cmd, "nand write 0x%x 0x%x 0x%x 0x%x",
					ddr_addr,
					boot_partition,
					boot_partition + blk_size,
					align_size);
			ret = run_command (cmd, 0);
			if (ret != 0) {
				printf("run cmd %s ret %d\n", cmd, ret);
				return ret;
			}
		}
	}
	else {
		if(include_oob) {
			sprintf(cmd, "nand write.oob 0x%x 0x%x 0x%x 0x%x",
					ddr_addr,
					start_blk * blk_size,
					(start_blk + total_blks) * blk_size,
					align_size);
		} else {
			sprintf(cmd, "nand write 0x%x 0x%x 0x%x 0x%x",
					ddr_addr,
					start_blk * blk_size,
					(start_blk + total_blks) * blk_size,
					align_size);
		}
		ret = run_command (cmd, 0);
		if (ret != 0) {
			printf("run cmd %s ret %d\n", cmd, ret);
			return ret;
		}
	}

	return 0;
}

static int do_l2nand2(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *str;
	int ret, i;
	char buff[1024];
	char cmd[128];
	unsigned int line_len = 0;
	unsigned int ddr_addr;
	char partition_name[128];
	char type[32];
	unsigned int start_blk = 0, total_blks = 0;
	unsigned int layout_size = 0;
	unsigned int imgsize;
	char *var = malloc(MAX_NANDIMG_SIZE);
	if (!var)
		return -1;
	ddr_addr = (unsigned int)(uint64_t)var;

	if (argc < 2) {
		ret = -1;
		goto NEXT;
	}

	if (strlen(argv[1]) > 255) {
		printf("%s, file name %s is too long.\n", __func__, argv[1]);
		ret = -1;
		goto NEXT;
	}
	if (argc == 3)
		ddr_addr = simple_strtoull(argv[2], NULL, 16);

	sprintf(cmd, "imgload %s %s/flash_type.cfg 0x%x", srcstr[2], argv[1], (uint32_t)(uint64_t)buff);
	ret = run_command (cmd, 0);
	if (ret != 0) {
		printf("run cmd %s ret %d\n", cmd, ret);
		goto NEXT;
	}

	imgsize = env_get_hex("filesize", 0);
	parse_nand_config(buff, imgsize, &blk_size, &page_size, &ecc_strength);

	sprintf(cmd, "imgload %s %s/subimglayout 0x%x", srcstr[2], argv[1], (uint32_t)(uint64_t)buff);
	ret = run_command (cmd, 0);
	printf("img_size = %d\n", imgsize);
	if (ret != 0) {
		printf("run cmd %s ret %d\n", cmd, ret);
		goto NEXT;
	}
	imgsize = env_get_hex("filesize", 0);

	i = 0;
	str = buff;
	layout_size = imgsize;
	while(i < layout_size) {
		get_next_nand_partition(str,
				partition_name,
				&start_blk,
				&total_blks,
				type);

		line_len = 0;
		while(*(str + line_len) != '\n') line_len++;
		line_len++;
		i += line_len;
		str += line_len;

		ret = burn_one_partition(argv[1],
				ddr_addr,
				partition_name,
				start_blk,
				total_blks,
				type);
		if (ret != 0) {
			printf("burn partition %s failed, ret %d\n",
					partition_name, ret);
			goto NEXT;
		} else {
			printf("burn partition %s successfully!!!\n\n\n", partition_name);
		}
	}
NEXT:
	free(var);
	return ret;
}


static char list2nand_help[] =\
"read NAND image from tftp/usb device/usb host, burn it to NAND chip.";

static char list2nand_usage[] =\
"\n###############################################################################"
"\n               list2nand/usb2nand/l2nand/u2nand usage manual"
"\n###############################################################################\n"
"[-v] [-m] [-o <oob_offset>] [-i <ignore_partition_list>] [-p <program_partition_list>] uNAND_IMG_PATH\n"
"    options:\n"
"      -v don't do readback verification.\n"
"      -m develop mode, only erase the partitions that will be wrote.\n"
"	      without -m, whole chip will be erase at first.\n"
"      -f force erase the blocks, ignoring the bad block marker.\n"
"      -i skip these partitions in the list, each partition is sperated\n"
"         by ',', such as 'block0,system'\n"
"      -p only program these partitions in list, each partiton is sperated\n"
"         by ',', sucha as 'bootloader,kernel'\n"
"      -o move original oob data by the offset, e.g. for original YAFFS2 data, need -o 2\n"
"    examples(usb2nand):\n"
"      erase the whole chip, and program the uNAND.img to chip\n"
"         usb2nand uNAND.img\n"
"      don't erase the whole chip, only erase the partition need to program\n"
"      it's to program the uNAND.img that only consists some partitular partitions\n"
"         usb2nand -m sbt/1022/uNAND.img\n"
"      only program partition 'bootloader,kernel' in the image\n"
"         usb2nand -p bootloader,kernel uNAND.img\n"
"      program all the partitions except 'block0,system' in the image\n"
"         usb2nand -p block0,system sbt/1022/uNAND.img\n"
"      m2nand is a debug command, please first load your image to an address\n"
"         m2nand [memory address]\n"
;

U_BOOT_CMD(
	u2nand_whole,	16,	0,	do_list2nand,
	list2nand_help,
	list2nand_usage
);

U_BOOT_CMD(
	m2nand,	16,	0,	do_list2nand,
	list2nand_help,
	list2nand_usage
);

static char null_help[] = "\n";
static char null_usage[] = "\n";

U_BOOT_CMD(
	ncbad,	16,	0,	do_ncbad,
	null_help,
	null_usage
);

#if defined(CONFIG_CMD_USB) && defined(CONFIG_CMD_FAT)
U_BOOT_CMD(
	usb2nand_whole,	16,	0,	do_list2nand,
	list2nand_help,
	list2nand_usage
);
#endif

U_BOOT_CMD(
	l2nand,	16,	0,	do_list2nand,
	list2nand_help,
	list2nand_usage
);

static char l2nand_help[] = \
"Download image from host over USB connection.";

static char l2nand_usage[] = \
"  examples:\n"
"          l2nand2 nand_img_folder [address]\n"
"            1. load nand device parameters from\n"
"              \'nand_img_folder\'/flash_type.cfg\n"
"               i.e. nand_page_size,nand_block_size,nand_ecc_strength\n"
"            2. load nand sub images from\n"
"              \'nand_img_folder\' to DDR \'address\',\n"
"            3. erase original NAND content\n"
"            4. burn the images listed in\n"
"              \'nand_img_folder\'/subimglayout to NAND, \n"
"            5.\'address\' is optional.\n"
;

U_BOOT_CMD(l2nand2, 16, 0, do_l2nand2,
	l2nand_help,
	l2nand_usage);


//#endif
