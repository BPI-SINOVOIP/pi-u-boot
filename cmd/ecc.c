// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Renesas Electronics Corporation
 */

/*
 * DRAM ECC support
 */

#include <common.h>
#include <cpu_func.h>
#include <command.h>
#include <linux/log2.h>
#include <linux/io.h>
#include <mapmem.h>
#include <dm.h>

#define	DFUSAAREACR		0xE6785000	/* DRAM FuSa Area Conf */
#define	DECCAREACR		0xE6785040	/* DRAM ECC Area Conf */
#define	NUM_DAREA		16
#define	ECC_ENABLE_MASK_BIT	(BIT(31))
#define	DFUSACR			0xE6784020	/* FuSa Ctrl */
#define	DADSPLCR0		0xE6784008	/* Address Split Control 0 */
#define	DADSPLCR1		0xE678400C	/* Address Split Control 1 */
#define	DADSPLCR2		0xE6784010	/* Address Split Control 2 */
#define	DADSPLCR3		0xE6784014	/* Address Split Control 3 */
#define	NUM_DFUSACR		8

/* We define macro for supported ECC mode on which platform.
 * BIT(0) will be described for ECC Dual Channel setting
 * BIT(1) will be described for ECC Single Channel setting
 */
#define ECC_DUAL	BIT(0)
#define ECC_SINGLE	BIT(1)


/* As the saddr, specify high-memory address (> 4 GB) */
#define	FUSAAREACR(en, size, saddr)	\
	(((uint32_t)en << 31) | ((uint32_t)size << 24) | \
	(uint32_t)(((uintptr_t)saddr) >> 12))
#define	ECCAREACR(ecc, saddr) \
	(((uint32_t)ecc << 31) | (uint32_t)(((uintptr_t)saddr) >> 12))
#define	FUSACR(efusa, dfusa)	\
	(((uint32_t)efusa << 24) | ((uint32_t)dfusa << 16))
#define	GET_EFUSA(x)	(((uint32_t)x & (0xff << 24)) >> 24)
#define	SPLITSEL(x)	(((uint32_t)x & 0xff) << 16)
#define	AREA(x)		(((uint32_t)x & 0x1f) << 8)

#define SIZE_1MB		(1024 * 1024)
#define MAX_BLOCK_SIZE_MB	(512)
#define DRAM_ADDR_BASE		(0x400000000)
#define DRAM_LEGACY_ADDR_OFSET	(0x3C0000000)
#define ECC_ADDR_XOR		(0x200000000)
#define BANK_SIZE_MB		(2048)

static int ecc_support;

/* id_offset :
 * If device support 2 ECC modes, we should increase
 * the ID of ECC Single Channel in logging.
 */
int id_offset;

/* ecc_bzero64(u64 addr, u32 size)
 * Write zero-valued octa-byte words
 * addr : start address
 * size : size in bytes
 */
void ecc_bzero64(u64 addr, u32 size)
{
	void *buf;
	u64 *ptr;
	u64 *end;

	if (gd->bd->bi_dram[1].start == 0 ||
	    addr < gd->bd->bi_dram[1].start)
		addr = (addr - DRAM_LEGACY_ADDR_OFSET);

	buf = map_sysmem(addr, size);
	ptr = (u64 *)buf;
	end = ptr + (size / sizeof(u64));

	while (ptr < end)
		*ptr++ = 0;

	unmap_sysmem(buf);
	flush_dcache_all();
}

/* void ecc_bzero64_blocks(u32 block_pos, u32 blocks,
 *			u32 blocksize, u64 addr_mask)
 * block_pos : position of block in extra split mode
 * blocks : areas of ECC enabled
 * blocksize : size in MB
 * addr_mask : exclusive or with start address
 */
void ecc_bzero64_blocks(u32 block_pos, u32 blocks,
			u32 blocksize, u64 addr_mask)
{
	int i;
	u64 addr;

	for (i = 0; i < NUM_DFUSACR; i++) {
		if (blocks & (0x1 << i)) {
			addr = (DRAM_ADDR_BASE ^ addr_mask)
			     + block_pos * blocksize * NUM_DFUSACR * SIZE_1MB
			     + blocksize * i * SIZE_1MB;
			ecc_bzero64(addr, blocksize * SIZE_1MB);
		}
	}
}

/* ecc_check_address(unsigned long long addr)
 * Check if address is 36 bits
 * Base address is defined as DRAM_ADDR_BASE
 */
int ecc_check_address(unsigned long long addr)
{
	if (addr < DRAM_ADDR_BASE)
		return 0;
	return 1;
}

/* ecc_list_setting(int mode_process)
 * Check All ECC registers follow mode_process parameter, then print
 * value of them in format that User can understand.
 * mode_process: Select which ECC area have which mode will be printed
 */
void ecc_list_setting(int mode_process)
{
	int i;

	if (!(ecc_support & mode_process))
	{
		printf("ECC: Command not support for this platform\n");
		return;
	}

	if (mode_process & ECC_DUAL) {
		uint32_t dfusacr, adsplcr0, adsplcr1;

		dfusacr = readl((uint32_t *)DFUSACR);
		adsplcr0 = readl((uint32_t *)DADSPLCR0);
		adsplcr1 = readl((uint32_t *)DADSPLCR1);
		printf("ECC: List of ECC area Dual Channel:\n");
		for (i = 0; i < NUM_DFUSACR; i++) {
			if (dfusacr & FUSACR(0, 0x1 << i)) {
				printf("%2d: %s : Data from 0x%llx\n",
				       i, "Enabled",
				       (u64)(i * SIZE_1MB * BANK_SIZE_MB / 8
					   + DRAM_ADDR_BASE));
				printf("	      ECC from 0x%llx\n",
				       (u64)(ECC_ADDR_XOR ^ (i * SIZE_1MB
							   * BANK_SIZE_MB / 8
							   + DRAM_ADDR_BASE)));
				printf("	      size %dMB : mode %d bits\n",
				       BANK_SIZE_MB / 8, 8);
			} else if ((adsplcr0 & SPLITSEL(0x1 << i)) &&
				   (adsplcr1 & SPLITSEL(0x1 << i))) {
				printf("%2d: %s : Data in %dMB from 0x%llx\n",
				       i, "Enabled", BANK_SIZE_MB / 8,
				       (u64)(i * SIZE_1MB * BANK_SIZE_MB / 8
					   + DRAM_ADDR_BASE));
				printf("	      ECC in %dMB from 0x%llx\n",
				       BANK_SIZE_MB / 8,
				       (u64)(ECC_ADDR_XOR ^ (i * SIZE_1MB
							   * BANK_SIZE_MB / 8
							   + DRAM_ADDR_BASE)));
				printf("	      EFUSA  0x%x : mode %d bits\n",
				       GET_EFUSA(dfusacr), 8);
			} else {
				printf("%2d: Disabled\n", i);
			}
		}
	}

	if (mode_process & ECC_SINGLE) {
		uint32_t dfusaareacr;
		uint32_t deccareacr;

		printf("ECC: List of ECC area Single Channel:\n");
		for (i = 0; i < NUM_DAREA; i++) {
			dfusaareacr = readl(((uint32_t *)DFUSAAREACR + i));
			deccareacr = readl(((uint32_t *)DECCAREACR + i));
			if (dfusaareacr & ECC_ENABLE_MASK_BIT) {
				printf("%2d: %s : Data from 0x%llx : ECC from 0x%llx :",
				       i + id_offset,
				       dfusaareacr & ECC_ENABLE_MASK_BIT ?
				       "Enabled" : "Disabled",
				       (unsigned long long)(dfusaareacr << 8) << 4,
				       (unsigned long long)(deccareacr << 8) << 4);
				printf(" size  %d MB : mode %d bits\n",
				       1 << ((dfusaareacr >> 24) & 0xF),
				       deccareacr & ECC_ENABLE_MASK_BIT ? 64 : 8);
			} else {
				printf("%2d: Disabled\n",
				       i + id_offset);
			}
		}
	}

}

/* ecc_add_configure(unsigned long long data_addr, unsigned long long ecc_addr,
 *		     unsigned long block_size, unsigned long mode)
 * Set registers to enable an ECC area in single channel mode
 * data_addr : Data start address must be 36 bits address
 * ecc_addr : ECC start address must be 36 bits address
 * size : size of Data area
 * mode : ECC mode : 64 / 8 : 64bitsdata/8bitsECC OR 8bitsdata/5bitsECC
 */
int ecc_add_configure(unsigned long long data_addr, unsigned long long ecc_addr,
		      unsigned long block_size, unsigned long mode)
{
	int i;
	uint32_t dfusaareacr;
	uint32_t deccareacr;

	if (!(ecc_support & ECC_SINGLE))
	{
		printf("ECC: Command not support for this platform\n");
		return 0;
	}

	for (i = 0; i < NUM_DAREA; i++) {
		if (readl((uint32_t *)DFUSAAREACR + i) & ECC_ENABLE_MASK_BIT) {
			if (i == (NUM_DAREA - 1))
				goto err_no_resource;
			continue;
		}

		if (mode == 64) {
			ecc_bzero64(ecc_addr,
				    (block_size / NUM_DFUSACR) * SIZE_1MB);
			writel(FUSAAREACR(1, ilog2(block_size), data_addr),
			       ((uint32_t *)DFUSAAREACR + i));
			writel(ECCAREACR(1, ecc_addr),
			       ((uint32_t *)DECCAREACR + i));
			ecc_bzero64(data_addr, block_size * SIZE_1MB);
		} else if (mode == 8) {
			ecc_bzero64(ecc_addr, block_size * SIZE_1MB);
			writel(FUSAAREACR(1, ilog2(block_size), data_addr),
			       ((uint32_t *)DFUSAAREACR + i));
			writel(ECCAREACR(0, ecc_addr),
			       ((uint32_t *)DECCAREACR + i));
			ecc_bzero64(data_addr, block_size * SIZE_1MB);
		} else {
			printf("ECC: ERROR: not support mode %ld\n", mode);
			return 0;
		}

		dfusaareacr = readl(((uint32_t *)DFUSAAREACR + i));
		deccareacr = readl(((uint32_t *)DECCAREACR + i));
		if (dfusaareacr & ECC_ENABLE_MASK_BIT) {
			printf("ECC: Enabled an ECC area : Data from 0x%llx :",
			       (unsigned long long)(dfusaareacr << 8) << 4);
			printf(" ECC from 0x%llx : size %d MB : mode %d bits\n",
			       (unsigned long long)(deccareacr << 8) << 4,
			       1 << ((dfusaareacr >> 24) & 0xF),
			       deccareacr & ECC_ENABLE_MASK_BIT ? 64 : 8);
		} else {
			printf("ECC: Failed to enable ECC at 0x%llx\n",
			       data_addr);
		}

		break;
	}

	return 1;

err_no_resource:
	printf("ECC: ERROR : Not Enough Resource, only support %d Areas\n",
	       NUM_DAREA);
	return 0;
}

/* ecc_add_setting(unsigned long long data_start_addr,
 *		   unsigned long size, unsigned long mode,
 *		   unsigned long long ecc_start_addr,
 *                 int mode_process)
 * Get user input to enable ECC areas which ECC mode determite by mode_process
 * data_start_addr : Data start address must be 36 bits address
 * size : size of Data area
 * mode_process: select ECC mode will be configurated.
 * For ECC Single mode only:
 *   mode : ECC mode : 64 / 8 : 64bitsdata/8bitsECC OR 8bitsdata/5bitsECC
 *   ecc_start_addr : ECC start address must be 36 bits address
 * For ECC Single Channel Setting:
 *
 *  ________________________  data_start_addr
 * |                        |   |
 * |                        |   |
 * |     Data area          |    > size
 * |                        |   |
 * |________________________|   |
 * |                        |
 * |  this gap will be 0    |
 *    if ecc_start_addr
 *    not set
 * |                        |
 * |________________________| ecc_start_addr
 * |                        |   |
 * |       ECC area         |   |
 * |                        |    > auto-caculated size depend on mode
 * |                        |   |
 * |________________________|   |
 *
 * For ECC Dual Channel Setting:
 *
 *                DRAM Channel 0              DRAM Channel 1
 * data_start_addr______________  _ _ _ _ _ _ ______________ ecc_start_addr
 *               |              |   |        |              |
 *               |              |   |        |              |
 *               |  Data area   |    > size  |   ECC area   |
 *               |              |   |        |              |
 *               |______________| _ | _ _ _ _|______________|
 *               |              |            |              |
 *               |              |            |              |
 *               |              |            |              |
 *               |              |            |              |
 *               |              |            |              |
 *               |______________|            |______________|
 */
void ecc_add_setting(unsigned long long data_start_addr,
		     unsigned long size, unsigned long mode,
		     unsigned long long ecc_start_addr,
		     int mode_process)
{
	unsigned int block_size;
	unsigned long long data_addr = data_start_addr;
	unsigned long long ecc_addr = ecc_start_addr;

	if (!(ecc_support & mode_process))
	{
		printf("ECC: Command not support for this platform\n");
		return;
	}

	if (mode_process & ECC_DUAL) {
		u32 extra_block_size, block_pos, block_count;
		u32 dfusa = 0;
		u32 dfusacr, adsplcr0, adsplcr1, adsplcr2, adsplcr3;

		block_size = BANK_SIZE_MB / 8;
		extra_block_size = block_size / 8;
		dfusacr = readl((u32 *)DFUSACR);
		adsplcr0 = readl((u32 *)DADSPLCR0);
		adsplcr1 = readl((u32 *)DADSPLCR1);
		adsplcr2 = 0;
		adsplcr3 = readl((u32 *)DADSPLCR3);

		if (adsplcr0 == 0 || adsplcr1 == 0 || adsplcr3 == 0) {
			adsplcr0 = AREA(ilog2(block_size * SIZE_1MB));
			adsplcr1 = AREA(ilog2(block_size * SIZE_1MB));
			adsplcr3 = AREA(ilog2(extra_block_size * SIZE_1MB));
		}

		if (size >= block_size) {
			block_count = size / block_size;
			block_pos = 0;
		} else if (size >= extra_block_size && size < block_size) {
			block_count = size / extra_block_size;
			block_pos = (data_start_addr - DRAM_ADDR_BASE)
				  / (block_size * SIZE_1MB);
			if (dfusacr & FUSACR(0, 0x1 << block_pos)) {
				printf("ECC: ECC already enabled from 0x%llx (%dMB)\n",
				       (u64)(DRAM_ADDR_BASE
					   + block_pos * block_size * SIZE_1MB),
				       block_size);
				return;
			}

			if (dfusacr & FUSACR(0xff, 00)) {
				printf("ECC: Extra split mode used (FUSACR:0x%x)\n",
				       dfusacr);
				printf("ECC: Only one ECC area can use split mode\n");
				return;
			}
			block_size = extra_block_size;
		} else {
			printf("ECC: size %ldMB is too small, smallest size is %dMB\n",
			       size, extra_block_size);
			return;
		}

		if (data_start_addr & (block_size * SIZE_1MB - 1)) {
			printf("ECC: 0x%llx not aligned to block size (%dMB)\n",
			       data_start_addr, block_size);
			data_addr = data_start_addr
				  & (~((u64)(block_size * SIZE_1MB - 1)));
			printf("ECC: new address 0x%llx\n", data_addr);
		}

		if (size % block_size) {
			printf("ECC: size %ldMB is not aligned to block size (%dMB)\n",
			       size,
			       block_size);
			size = block_count * block_size;
			printf("ECC: new size %ldMB\n", size);
		}

		for (; block_count > 0 ; block_count--) {
			dfusa |= (0x1 << ((data_addr - DRAM_ADDR_BASE -
					   block_pos * block_size * 8 * SIZE_1MB)
					 / (block_size * SIZE_1MB)));
			data_addr += block_size * SIZE_1MB;
		}

		ecc_bzero64_blocks(block_pos, dfusa, block_size, ECC_ADDR_XOR);
		if (block_size == extra_block_size) {
			adsplcr0 |= (SPLITSEL(0x1 << block_pos));
			adsplcr1 |= (SPLITSEL(0x1 << block_pos));
			writel(dfusacr | FUSACR(dfusa, 0), (uint32_t *)DFUSACR);
		} else {
			writel(dfusacr | FUSACR(0, dfusa), (uint32_t *)DFUSACR);
		}

		writel(adsplcr0, (uint32_t *)DADSPLCR0);
		writel(adsplcr1, (uint32_t *)DADSPLCR1);
		writel(adsplcr2, (uint32_t *)DADSPLCR2);
		writel(adsplcr3, (uint32_t *)DADSPLCR3);
		ecc_bzero64_blocks(block_pos, dfusa, block_size, 0);
	} else if (mode_process & ECC_SINGLE) {
		unsigned int block_size = MAX_BLOCK_SIZE_MB;
		if (ecc_addr == 0)
			ecc_addr = data_start_addr + size * SIZE_1MB;

		while ((block_size >= 1) && (size != 0)) {
			if (size >= block_size) {
				ecc_add_configure(data_addr, ecc_addr,
						  block_size, mode);
				data_addr = data_addr + block_size * SIZE_1MB;
				if (mode == 64) {
					ecc_addr = ecc_addr +
						   block_size * SIZE_1MB / 8;
				} else if (mode == 8) {
					ecc_addr = ecc_addr +
						   block_size * SIZE_1MB;
				}
				size = size - block_size;
			} else {
				block_size = block_size / 2;
			}
		}
	}
}



/* ecc_rm_setting(unsigned int id, int mode_process)
 * Remove an ECC area by id in 'mode_process' ECC mode
 * id : input id , can get id by 'ecc list'
 * mode_process: input ecc mode will be processed.
 */
void ecc_rm_setting(unsigned int id, int mode_process)
{
	if (!(ecc_support & mode_process))
	{
		printf("ECC: Command not support for this platform\n");
		return;
	}

	if (mode_process & ECC_DUAL) {
		u32 dfusacr;

		dfusacr = readl((uint32_t *)DFUSACR);

		if (id >= NUM_DFUSACR) {
			printf("ECC: Not support ECC at id %d\n", id);
			return;
		}

		if (dfusacr & FUSACR(0, 0x1 << id)) {
			writel(dfusacr & (~(FUSACR(0, 0x1 << id))),
			       (u32 *)DFUSACR);
		} else {
			u32 adsplcr0 = readl((u32 *)DADSPLCR0);
			u32 adsplcr1 = readl((u32 *)DADSPLCR1);

			writel(adsplcr0 & (~SPLITSEL(0x1 << id)),
			       (u32 *)DADSPLCR0);
			writel(adsplcr1 & (~SPLITSEL(0x1 << id)),
			       (u32 *)DADSPLCR1);
			writel(dfusacr & (~(FUSACR(0xff, 0))), (u32 *)DFUSACR);
		}
		printf("ECC: Disabled ECC at id %d\n", id);
	} else if (mode_process & ECC_SINGLE) {
		uint32_t dfusaareacr;

		if (id >= NUM_DAREA) {
			printf("ECC: Not support ECC at id %d\n",
			       id + id_offset);
			return;
		}

		writel(0, ((uint32_t *)DFUSAAREACR + id));
		writel(0, ((uint32_t *)DECCAREACR + id));

		dfusaareacr = readl(((uint32_t *)DFUSAAREACR + id));

		if (dfusaareacr & ECC_ENABLE_MASK_BIT) {
			printf("ECC: Failed to disable ECC at id %d\n",
			       id + id_offset);
		} else {
			printf("ECC: Disabled ECC at id %d\n",
			       id + id_offset);
		}
	}
}

/* ecc_help()
 * print usage log
 */
void ecc_help(void)
{
	printf("usage: ecc <command> [<mode>] [<args>]\n"
	       "\n"
	       "There are two modes supported depends on SoC\n"
	       "  -s, --single     single channel mode (RZ/G2H, RZ/G2N, RZ/G2E, RZ/G2M v3.0)\n"
	       "  -d, --dual       dual channel mode (RZ/G2H, RZ/G2M v1.3, RZ/G2M v3.0)\n"
	       "\n");
	printf("These are commands supported, they may take different args\n"
	       "depend on single or dual mode\n"
	       "  list             list all ecc areas with enabled or disabled status\n"
	       "		   list -d : Will list all ecc areas with dual channel mode\n"
	       "		   list -s : Will list all ecc areas with single channel mode\n"
	       "                   this command can be executed without <mode> and <args>\n"
	       "\n");
	printf("  add              enable one or multiple ecc areas depend on size\n"
	       "                   args with -s : <data address> [ecc address] <size> <bits>\n"
	       "                   <data address> : 36 bits data start address\n"
	       "                   [ecc address]  : 36 bits ecc start address, if not defined\n"
	       "                                    ecc will be placed right after data area\n"
	       "                   <size> : size in MB, support in blocksize of 2^n MB (1MB,\n"
	       "                            2MB,..512MB). One block is one ecc area. if size\n"
	       "                            is not aligned to 2^n, many blocks will be used\n"
	       "                   <bits> : 8 Or 64 (8bitsdata/5bitsECC Or 64bitsdata/8bitsECC)\n"
	       "\n"
	       "                   args with -d: <data address> <size>\n"
	       "                   only support 8bitsdata/5bitsECC\n"
	       "                   <data address> : 36 bits data start address,\n"
	       "                                    it must be in DRAM channel 0\n"
	       "                                    (RZ/G2M v1.3, RZ/G2M v3.0 and RZ/G2H).\n"
	       "                                    ECC address will be data_start_address \n"
	       "                                    exclusive or with 0x2_0000_0000\n"
	       "                   <size> : size in MB, support in blocksize (DRAM_BANK_SIZE/8)\n"
	       "                            one block is one ECC area. If input size > blocksize\n"
	       "                            it will be re-alinged to blocksize. Big size will\n"
	       "                            take mulitple blocks. If input size < blocksize,\n"
	       "                            extra split mode will be used, one block will be\n"
	       "                            splited into 8 small blocks (DRAM_BANK_SIZE/64).\n"
	       "                            Note that only one ecc area can be configured with\n"
	       "                            extra split\n"
	       "\n");
	printf("  rm               disable an ECC area\n"
	       "                   rm <id> : remove ECC Area with <id> in 'ecc list <mode>'\n"
	       "                   rm all  : remove all ECC Area\n"
	       "\n"
	       "  help             print this help log\n");
}

/* do_ecc(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
 * Main process parse user command
 * argc : number of parameters
 * argv : array of parameters
 */
int do_ecc(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int mode_process = 0;

	switch (rmobile_get_cpu_type()) {
	case RMOBILE_CPU_TYPE_R8A7795:
		ecc_support = ECC_SINGLE | ECC_DUAL;
		break;
	case RMOBILE_CPU_TYPE_R8A7796:
		ecc_support = ECC_DUAL;
		if (rmobile_get_cpu_rev_integer() == 3)
			ecc_support |= ECC_SINGLE;
		break;
	case RMOBILE_CPU_TYPE_R8A77965:
	case RMOBILE_CPU_TYPE_R8A77990:
		ecc_support = ECC_SINGLE;
		break;
	default:
		return -1;
	}

	if (!(ecc_support & ECC_SINGLE || ecc_support & ECC_DUAL))
	{
		printf("ECC: Command not support for this platform yet\n");
		return 1;
	}

	id_offset = (ecc_support & ECC_DUAL) ? NUM_DFUSACR : 0;

	if (strcmp(argv[2], "-s") == 0 ||
	    strcmp(argv[2], "--single") == 0)
		mode_process |= ECC_SINGLE;     /* Single channel process */
	else if (strcmp(argv[2], "-d") == 0 ||
		 strcmp(argv[2], "--dual") == 0)
		mode_process |= ECC_DUAL;       /* Dual channel process */
	if (strcmp(argv[1], "list") == 0) {
		/* We only allow command has no parameter or "select ECC mode"
		 * after "list" parameter.
		 */
		if ((mode_process == 0 && argc > 2) ||
		    (mode_process != 0 && argc > 3)) {
			ecc_help();
		} else {
			/* List all Register ID with ECC status
			 * ECC dual channel setting should be printed first.
			 */
			if (mode_process == 0)
				mode_process = ecc_support;
			if (mode_process & ECC_DUAL)
				ecc_list_setting(ECC_DUAL);
			if (mode_process & ECC_SINGLE)
				ecc_list_setting(ECC_SINGLE);
		}
	} else if (strcmp(argv[1], "add") == 0) {
		/* Add an ECC area*/
		unsigned long long data_addr = 0;
		unsigned long long ecc_addr = 0;
		unsigned long size = 0;
		unsigned long mode = 8;

		data_addr = simple_strtoull(argv[3], NULL, 16);
		if (mode_process & ECC_SINGLE) {
			if (argc == 7) {
				ecc_addr = simple_strtoull(argv[4], NULL, 16);
				if (ecc_check_address(ecc_addr) == 0) {
					printf("ECC: Please use 36 bits"
					       " address\n");
					return 0;
				}
				size = simple_strtoul(argv[5], NULL, 10);
				mode = simple_strtoul(argv[6], NULL, 10);
			} else if (argc == 6) {
				ecc_addr = 0;
				size = simple_strtoul(argv[4], NULL, 10);
				mode = simple_strtoul(argv[5], NULL, 10);
			} else {
				ecc_help();
				return 0;
			}
		} else if (mode_process & ECC_DUAL) {
			size = simple_strtoul(argv[4], NULL, 10);
		} else {
			ecc_help();
		}

		if (ecc_check_address(data_addr)) {
			ecc_add_setting(data_addr, size, mode,
					ecc_addr, mode_process);
		} else {
			printf("ECC: Please use 36 bits address\n");
		}

	} else if (strcmp(argv[1], "rm") == 0) {
		/* Remove an ECC area*/
		if (argc > 3) {
			ecc_help();
		} else {
			unsigned long id;

			mode_process = ecc_support;
			/* User want to remove all ECC area
			 */
			if (strcmp(argv[2], "all") == 0) {
				if (mode_process & ECC_DUAL) {
					for (id = 0; id < NUM_DFUSACR; id++)
						ecc_rm_setting(id, ECC_DUAL);
				}
				if (mode_process & ECC_SINGLE) {
					for (id = 0; id < NUM_DAREA; id++)
						ecc_rm_setting(id, ECC_SINGLE);
				}
			} else {
				id = simple_strtoul(argv[2], NULL, 10);
				/* If device support more than 1 ECC setting
				 * we should select one
				 */
				if (mode_process & ECC_DUAL &&
				    mode_process & ECC_SINGLE) {
				/* When the id is over the number of
				 * area in ECC Dual Channel, we need
				 * to process it as ECC Single Channel.
				 */
					if (id >= NUM_DFUSACR) {
						id -= NUM_DFUSACR;
						mode_process = ECC_SINGLE;
					} else {
						mode_process = ECC_DUAL;
					}
				}
					ecc_rm_setting(id, mode_process);
			}
		}
	} else {
		/* print usage log*/
		ecc_help();
	}
	return 1;
}

U_BOOT_CMD(
	ecc,	7,	0,	do_ecc,
	"Enable/disable ECC for a memory area\n", NULL
);
