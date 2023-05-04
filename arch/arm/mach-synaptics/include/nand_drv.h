/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#ifndef __NAND_DEV_PRIV_H__
#define __NAND_DEV_PRIV_H__

#include <common.h>
#include <linux/compat.h>
#include "Galois_memmap.h"
#include "nfc_struct.h"
#include "nfc_reg_def.h"
#include "nfc_utility.h"

//#define DEBUG 1
#ifdef DEBUG
#define NAND_DEBUG printf
#else
#define NAND_DEBUG(...)
#endif

#define NAND_ERR printf

static inline uint32_t hal_read32(uint32_t addr)
{
	return *(volatile uint32_t *)(uintptr_t)addr;
}

static inline int32_t hal_write32(uint32_t addr, uint32_t val)
{
	*(volatile uint32_t *)(uintptr_t)addr = val;
	return 0;
}

#define TIMER_CLOCK_KHZ (1000*1000)
static inline void hal_delay_us(uint32_t count)
{
	volatile uint32_t i;
	if (count > 50*1000)
		count = (count / 1000) * TIMER_CLOCK_KHZ;
	else
		count = count * TIMER_CLOCK_KHZ / 1000;

	count /= 10; //more than 10 instruction in one loop

	for (i = 0; i < count; i++) {
		i--; i++;
	}
}

#ifndef assert
#define assert(x)                                      \
    do {                                               \
        if (!(x)) {                                    \
            NAND_DEBUG("ASSERT: %s, %s(%d): (!" #x ")\n",  \
                    __FILE__, __FUNCTION__, __LINE__); \
            while(1);                                  \
        }                                              \
    } while (0)
#endif // #ifndef assert

#define NAND_READ (0x1)
#define NAND_WRITE (0x2)
#define NAND_BOOT_PARTITION_SIZE 0x100000
#define NAND_BOOT_PARTITION_NUM (0x8)
#define NAND_PAGE_BUF_SIZE (2048)
#define NAND_ECC_SECTOR_SIZE (2048)
#define MAX_PAGE_SIZE 8192
#define MAX_OOB_SIZE 32
#define MV_NAND_MAX_PAGE_SIZE MAX_PAGE_SIZE
#undef NFC_REG
#undef NFC_RD_REG
#undef NFC_WR_REG
#undef NFC_RD_REG_NAME
#undef NFC_WR_REG_NAME
#undef NFC_RD_FLD_NAME
#undef NFC_WR_FLD_NAME
#define NFC_REG(offset) hal_read32((NFC_REG_BASE) + (offset))
#define NFC_RD_REG(ra) hal_read32((NFC_REG_BASE) + (ra))
#define NFC_WR_REG(ra, v) hal_write32(((NFC_REG_BASE) + (ra)), v)
#define NFC_RD_REG_NAME(regname) hal_read32((NFC_REG_BASE) + (RA__##regname))
#define NFC_WR_REG_NAME(regname, v) hal_write32(((NFC_REG_BASE) + (RA__##regname)), v)
#define NFC_RD_FLD_NAME(fieldname, v)   do { \
	uint32_t t9d8fu289139d; \
	t9d8fu289139d = NFC_RD_REG(BA__##fieldname); \
	v = (t9d8fu289139d >> LSb__##fieldname) & MSK__##fieldname; \
} while (0)

#define NFC_WR_FLD_NAME(fieldname, v)   do { \
	uint32_t t9d8fu289139d; \
	t9d8fu289139d = NFC_RD_REG(BA__##fieldname); \
	t9d8fu289139d &= ~(MSK__##fieldname << LSb__##fieldname); \
	t9d8fu289139d |= ((v) << LSb__##fieldname); \
	NFC_WR_REG(BA__##fieldname, t9d8fu289139d); \
} while (0)

#undef NFC_DELAY_US
#define NFC_DELAY_US(n) hal_delay_us(n)
#undef DUMP_FIELD_DEC
#define DUMP_FIELD_DEC(var, fieldname) NAND_DEBUG("%s = %d\n", #fieldname, var.fieldname)
#undef DUMP_FIELD_HEX
#define DUMP_FIELD_HEX(var, fieldname) NAND_DEBUG("%s = 0x%x\n", #fieldname, var.fieldname)
#undef DEFAULT_TIMEOUT_COUNT
#define DEFAULT_TIMEOUT_COUNT (8000)
#undef DEFAULT_DELAY_INTERVAL
#define DEFAULT_DELAY_INTERVAL (50)
#define ERR_ALL_EXCEPT_ECC_MAX (ERR_DESC \
		| ERR_ECC \
		| ERR_DEV \
		| ERR_FAIL \
		| ERR_BUS)

#define NAND_MAGIC_NUMBER       0xD2ADA3F1

typedef struct _bootparam_t_{
	uint32_t magic;
	union{
		uint32_t nand_param;
		uint32_t u32;
		struct{
			uint32_t page_size       : 2; // @0
			uint32_t address_cycle   : 1; // @1
			uint32_t scrambler_en    : 1; // @2
			uint32_t block_size      : 4; // @4
			uint32_t ecc_en          : 1; // @8
			uint32_t bch_en          : 1; // @9
			uint32_t spare_en        : 1; // @10
			uint32_t chunk_size      : 2; // @11
			uint32_t ecc_strength    : 5; // @13
			uint32_t slc_en          : 1; // @18
			uint32_t slc_page_table_offset : 8; // @19
			uint32_t blk_num         : 4; // @27
			uint32_t page_size_hi    : 1; // @31
		};
	} ;
} bootparam_t;

typedef struct nand_timing_t {
	uint32_t tRH;
	uint32_t tRP;
	uint32_t tWH;
	uint32_t tWP;
	uint32_t tADL;
	uint32_t tCCS;
	uint32_t tWHR;
	uint32_t tRHW;
	uint32_t tRHZ;
	uint32_t tWB;
	uint32_t tCWAW;
	uint32_t tVDLY;
	uint32_t tFEAT;
	uint32_t CS_hold_time;
	uint32_t CS_setup_time;
	uint32_t PHY_CTRL_REG__phony_dqs_timing;
} nand_timing_t;

typedef struct blk_dev_nand_t {
	uint32_t curr_position;
	uint32_t curr_partition;
	uint32_t total_partitions;
	uint32_t curr_partition_offset;
	uint32_t nand_base;

	uint32_t page_size;
	uint32_t blk_size;
	uint32_t spare_bytes_per_page;
	uint32_t addr_cycle;
	uint32_t delay_int_val;
	uint32_t timeout_val;
	uint32_t stable_cnt;
	uint32_t sect_cnt;
	uint32_t sect_size; /* unit size for BCH algorithm, including user
				 data + ECC check sums + padding data*/
	uint32_t last_sect_size;
	uint32_t ecc_en;
	uint32_t ecc_strength;
	uint32_t ecc_size; /* ECC bytes used per sector*/
	uint32_t scrambler_en;
	nand_timing_t timings;
} blk_dev_nand_t;

int nand_page_size(void);

int nand_block_size(void);

int nand_oob_size(void);

int nand_ecc_strength(void);

int nand_scramber_en(void);

void nand_drv_open(const uint32_t blk_size,
		const uint32_t page_size,
		const uint32_t ecc_strength,
		const uint32_t scrambler_en,
		const uint32_t oob_size);

int diag_Nand_Read_ID(uint32_t        id_buf, uint32_t * id_size);
int mv_nand_block_bad(loff_t ofs, int getchip);
int mv_nand_read_block(loff_t nand_start, unsigned char* data_buf, int data_size);
int mv_nand_read_block_withoob(loff_t nand_start, unsigned char* data_buf, int size);
int mv_nand_write_block(loff_t nand_start, const unsigned char* data_buf, int data_size);
int mv_nand_write_block_withoob(loff_t nand_start, const unsigned char* data_buf, int size);
int mv_nand_erase_multiblk(loff_t ofs, unsigned int blk_cnt);
int mv_nand_erase(loff_t ofs);
int mv_nand_mark_badblock(loff_t ofs, int getchip);

#endif //__NAND_DEV_PRIV_H__
