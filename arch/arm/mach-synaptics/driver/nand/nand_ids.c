/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#include "nand.h"
#define SZ_1				0x00000001
#define SZ_2				0x00000002
#define SZ_4				0x00000004
#define SZ_8				0x00000008
#define SZ_16				0x00000010
#define SZ_32				0x00000020
#define SZ_64				0x00000040
#define SZ_128				0x00000080
#define SZ_256				0x00000100
#define SZ_512				0x00000200

#define SZ_1K				0x00000400
#define SZ_2K				0x00000800
#define SZ_4K				0x00001000
#define SZ_8K				0x00002000
#define SZ_16K				0x00004000
#define SZ_32K				0x00008000
#define SZ_64K				0x00010000
#define SZ_128K				0x00020000
#define SZ_256K				0x00040000
#define SZ_512K				0x00080000

#define SZ_1M				0x00100000
#define SZ_2M				0x00200000
#define SZ_4M				0x00400000
#define SZ_8M				0x00800000
#define SZ_16M				0x01000000
#define SZ_32M				0x02000000
#define SZ_64M				0x04000000
#define SZ_128M				0x08000000
#define SZ_256M				0x10000000
#define SZ_512M				0x20000000

#define SZ_1G				0x40000000
#define SZ_2G				0x80000000

struct nand_device nand_flash_ids[] = {
	{"TC58NVG0S3E 1G 3.3V 8-bit",
		{ .id = {0x98, 0xd1, 0x90, 0x15, 0x76, 0x14, 0x01, 0x00} },
		  SZ_2K, SZ_128, SZ_128K, 8, 64},
	{"TC58NVG1S3H 2G 3.3V 8-bit",
		{ .id = {0x98, 0xda, 0x90, 0x15, 0x76, 0x16, 0x08, 0x00} },
		  SZ_2K, SZ_256, SZ_128K, 8, 128},
	{"TC58NVG2S0F 4G 3.3V 8-bit",
		{ .id = {0x98, 0xdc, 0x90, 0x26, 0x76, 0x15, 0x01, 0x08} },
		  SZ_4K, SZ_512, SZ_256K, 8, 224},
	{"TC58NVG2S0H 4G 3.3V 8-bit",
		{ .id = {0x98, 0xdc, 0x90, 0x26, 0x76, 0x16, 0x08, 0x00} },
		  SZ_4K, SZ_512, SZ_256K, 8, 256},
	{"TH58NVG2S3H 4G 3.3V 8-bit",
		{ .id = {0x98, 0xdc, 0x91, 0x15, 0x76, 0x16, 0x08, 0x00} },
		  SZ_2K, SZ_512, SZ_128K,8, 128},
	{"TC58NVG3S0F 8G 3.3V 8-bit",
		{ .id = {0x98, 0xd3, 0x90, 0x26, 0x76, 0x15, 0x02, 0x08}},
		  SZ_4K, SZ_1K, SZ_256K, 8, 232},
	{"TC58NVG5D2 32G 3.3V 8-bit",
		{ .id = {0x98, 0xd7, 0x94, 0x32, 0x76, 0x56, 0x09, 0x00} },
		  SZ_8K, SZ_4K, SZ_1M, 8, 640},
	{"TC58NVG6D2 64G 3.3V 8-bit",
		{ .id = {0x98, 0xde, 0x94, 0x82, 0x76, 0x56, 0x04, 0x20} },
		  SZ_8K, SZ_8K, SZ_2M, 8, 640},
	{"SDTNRGAMA 64G 3.3V 8-bit",
		{ .id = {0x45, 0xde, 0x94, 0x93, 0x76, 0x50} },
		  SZ_16K, SZ_8K, SZ_4M, 6, 1280},
	{"H27UCG8T2ATR-BC 64G 3.3V 8-bit",
		{ .id = {0xad, 0xde, 0x94, 0xda, 0x74, 0xc4} },
		  SZ_8K, SZ_8K, SZ_2M, 6, 640},
	{"H27QCG8T2E5R‐BCF 64G 3.3V 8-bit",
		{ .id = {0xad, 0xde, 0x14, 0xa7, 0x42, 0x4a} },
		  SZ_16K, SZ_8K, SZ_4M, 6, 1664},
	{"MT29F4G08ABAFAH4 4G 3.3V 8-bit",
		{ .id = {0x2c, 0xdc, 0x80, 0xa6, 0x62, 0x00, 0x00, 0x00} },
		  SZ_4K, SZ_512, SZ_256K, 8, 256},
	{"MT29F4G08ABAEAWP 4G 3.3V 8-bit",
		{ .id = {0x2c, 0xdc, 0x90, 0xa6, 0x54, 0x00, 0x00, 0x00} },
		  SZ_4K, SZ_512, SZ_256K, 8, 256},
	{"MT29F4G08ABBFAH4 4G 1.8V 8-bit",
		{ .id = {0x2c, 0xac, 0x80, 0x26, 0x62, 0x00, 0x00, 0x00} },
		  SZ_4K, SZ_512, SZ_256K, 8, 256},
	{0}
};

struct nand_manufacturers nand_manuf_ids[] = {
	{NAND_MFR_TOSHIBA, "Toshiba"},
	{NAND_MFR_SAMSUNG, "Samsung"},
	{NAND_MFR_FUJITSU, "Fujitsu"},
	{NAND_MFR_NATIONAL, "National"},
	{NAND_MFR_RENESAS, "Renesas"},
	{NAND_MFR_STMICRO, "ST Micro"},
	{NAND_MFR_HYNIX, "Hynix"},
	{NAND_MFR_MICRON, "Micron"},
	{NAND_MFR_AMD, "AMD/Spansion"},
	{NAND_MFR_MACRONIX, "Macronix"},
	{NAND_MFR_EON, "Eon"},
	{NAND_MFR_SANDISK, "SanDisk"},
	{NAND_MFR_INTEL, "Intel"},
	{NAND_MFR_ATO, "ATO"},
	{0x0, "Unknown"}
};
