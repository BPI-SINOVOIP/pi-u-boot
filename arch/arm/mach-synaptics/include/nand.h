/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#ifndef __NAND_H
#define __NAND_H

#define __packed	__attribute__((packed))

#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_SAMSUNG	0xec
#define NAND_MFR_FUJITSU	0x04
#define NAND_MFR_NATIONAL	0x8f
#define NAND_MFR_RENESAS	0x07
#define NAND_MFR_STMICRO	0x20
#define NAND_MFR_HYNIX		0xad
#define NAND_MFR_MICRON		0x2c
#define NAND_MFR_AMD		0x01
#define NAND_MFR_MACRONIX	0xc2
#define NAND_MFR_EON		0x92
#define NAND_MFR_SANDISK	0x45
#define NAND_MFR_INTEL		0x89
#define NAND_MFR_ATO		0x9b

#define NAND_MAX_ID_LEN 8

struct nand_device {
	char *name;
	union {
		struct {
			unsigned char mfr_id;
			unsigned char dev_id;
		};
		unsigned char id[NAND_MAX_ID_LEN];
	};
	unsigned int page_size;
	unsigned int chip_size;
	unsigned int erase_size;
	unsigned short id_len;
	unsigned short oob_size;
};

struct nand_manufacturers {
	int id;
	char *name;
};

extern struct nand_device nand_flash_ids[];
extern struct nand_manufacturers nand_manuf_ids[];
#endif
