// SPDX-License-Identifier: GPL-2.0
/*
 * spacemit_k1x_efuse.c - Spacemit k1-x efuse
 *
 * Copyright (c) 2024 Spacemit Co., Ltd.
 *
 */

#include <asm/io.h>
#include <command.h>
#include <display_options.h>
#include <dm.h>
#include <clk.h>
#include <reset-uclass.h>
#include <linux/delay.h>
#include <misc.h>
#include <power/regulator.h>

// #define EFUSE_DEBUG

#define EFUSE_UUID_OFFSET	(0x104)  /* bank 0,chip UUID */
#define EFUSE_HUK_OFFSET	(0x124)  /* bank 1,Hardware Unique Key */
#define EFUSE_SSK_OFFSET	(0x144)  /* bank 2,Secret Symmetric Key */
#define EFUSE_CDPKH_OFFSET	(0x280)  /* bank 3,Chip Debug Public Key Hash */
#define EFUSE_ROTPKH_OFFSET	(0x2A0)  /* bank 4,Root of Trust Public Key Hash */
#define EFUSE_ARCN_OFFSET	(0x2C0)  /* bank 5,Anti-rollback Counter Number */
#define EFUSE_HWLOCK_OFFSET	(0x164)  /* bank 6,hwlock and lcs */
#define EFUSE_BANK7_OFFSET	(0x190)  /* bank 7, reserved */
#define EFUSE_BANK8_OFFSET	(0x200)  /* bank 8, reserved */
#define EFUSE_BANK9_OFFSET	(0x220)  /* bank 9, reserved */
#define EFUSE_BANK10_OFFSET	(0x240)  /* bank 10, reserved */
#define EFUSE_BANK11_OFFSET	(0x260)  /* bank 11, reserved */
#define GEU_CONFIG		(0x004)
#define EFUSE_PROG_VAL1		(0x038)
#define EFUSE_PROG_VAL2		(0x048)
#define EFUSE_STATUS		(0x184)
#define EFUSE_SCLK_DIV_CNTR	(0x3FC)

#define FUSE_BANK_WORDS		(256/32)
#define FUSE_BANK_BYTES		(256/8)
#define FUSE_MAX_BANK_NUM	(12)

/* bits definitions for register REG_GEU_FUSE_STATUS */
#define FUSE_READY		BIT(1)
#define FUSE_BURN_DONE		BIT(0)

/* life cycle  state */
struct lcs_config {
	unsigned cm:3;		/* 3bits,chip manufacturing state,LSB */
	unsigned dm:3;		/* 3bits,device manufacturing state */
	unsigned sp:3;		/* 3bits,secure product state */
	unsigned rma:3;		/* 3bits,RMA Label */
	unsigned dft:8;		/* 8bits,DFT password change */
	unsigned reserved:12;	/* 12bits,reserved,MSB */
};

/*efuse data, bank0 is regarded as uuid[32] */
struct efuse_data_core {
	uint8_t apcp_config[32];	/* bank 0,16bits top config,80bits app top config,160bits Manufacturing Parameters */
	uint8_t huk[32];		/* bank 1,256bits Hardware Unique Key (HUK) from SoC Vendor */
	uint8_t ssk[32];		/* bank 2,256bits Secret Symmetric Key (SSK) from OEM Vendor */
	uint8_t cdpkh[32];		/* bank 3,256bits Chip Debug Public Key Hash (CDPKH) from SoC/OEM Vendor */
	uint8_t rotpkh[32];		/* bank 4,256bits Root of Trust Public Key Hash (ROTPKH) from OEM Vendor */
	uint8_t arcnns[28];		/* bank 5,224bits Anti-Rollback Counter Number for Non-Secure Image (ARCN-NS) */
	uint8_t arcnse[4];		/* bank 5,32bits  Anti-rollback Counter Number for Secure Image (ARCN-Sec) */
	uint8_t hwlock[2];		/* bank 6,16bits  hardware lock */
	uint8_t reserved[2];		/* bank 6,16bits  reserved */
	struct lcs_config lcs;		/* bank 6,32bits  life cycle state */
	uint8_t reserved1[8];		/* bank 6,64bits */
	uint8_t reserved2[16];		/* bank 6,bottom half,128bits,hotfix code */
	uint8_t manu_param[32];		/* bank 7, manufactory infomation */
};

/*efuse data, bank0 is regarded as uuid[32] */
struct efuse_data {
	struct efuse_data_core efuse_core;
	uint8_t bank8[32];		/* bank 8 */
	uint8_t bank9[32];		/* bank 9 */
	uint8_t bank10[32];		/* bank 10 */
	uint8_t bank11[32];		/* bank 11 */
};

static const uint32_t efuse_bank_register_offset[] = {
	EFUSE_UUID_OFFSET,
	EFUSE_HUK_OFFSET,
	EFUSE_SSK_OFFSET,
	EFUSE_CDPKH_OFFSET,
	EFUSE_ROTPKH_OFFSET,
	EFUSE_ARCN_OFFSET,
	EFUSE_HWLOCK_OFFSET,
	EFUSE_BANK7_OFFSET,
	EFUSE_BANK8_OFFSET,
	EFUSE_BANK9_OFFSET,
	EFUSE_BANK10_OFFSET,
	EFUSE_BANK11_OFFSET,
};

struct spacemit_efuse_plat {
	void __iomem *reg_base;
	struct reset_ctl_bulk resets;
	struct clk_bulk clks;
	struct efuse_data efuse;
	int efuse_need_reload;
	int efuse_power_flag;
	struct udevice *regulator;
};

static inline int se_clock_on(struct udevice *dev)
{
	int ret;
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);

	ret = clk_enable_bulk(&plat->clks);
	if (ret) {
		pr_err("Failed to enable clk: %d\n", ret);
		return ret;
	}

	ret = reset_deassert_bulk(&plat->resets);
	if (ret) {
		pr_err("Failed to reset: %d\n", ret);
	}

	return ret;
}

static inline __maybe_unused int se_clock_off(struct udevice *dev)
{
	int ret;
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);

	ret = reset_assert_bulk(&plat->resets);
	if (ret) {
		pr_err("Failed to assert: %d\n", ret);
		return ret;
	}

	return ret;
}

static inline __maybe_unused int efuse_power_on(struct udevice *dev)
{
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);

	if (NULL != plat->regulator) {
		pr_info("Power on regulatore device %s\n", plat->regulator->name);
		regulator_set_value(plat->regulator, 1800000);
		return regulator_set_enable(plat->regulator, true);
	}
	else
		return -EIO;
}

static inline __maybe_unused int efuse_power_off(struct udevice *dev)
{
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);

	if (NULL != plat->regulator) {
		pr_info("Power off regulatore device %s\n", plat->regulator->name);
		return regulator_set_enable(plat->regulator, false);
	}
	else
		return -EIO;
}

/* load efuse data from efuse bank reg_offset address */
void efuse_load_all(struct udevice *dev)
{
	uint32_t reg_offset;
	uint32_t *buffer, bank, i, n = 0;
	uint32_t *bank_reg_offset = (uint32_t *)efuse_bank_register_offset;
	uint32_t bank_num = ARRAY_SIZE(efuse_bank_register_offset);
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);
	void __iomem *reg_base = plat->reg_base;

	buffer = (uint32_t *)&plat->efuse;

	se_clock_on(dev);
	for (bank = 0; bank < bank_num; bank++) {
		reg_offset = bank_reg_offset[bank];
		for (i = 0; i < FUSE_BANK_WORDS; i++) {
			buffer[n++] = readl(reg_base + reg_offset + i * 4);
		}
	}
	se_clock_off(dev);
}

int efuse_reload(struct udevice *dev)
{
	int timeout;
	uint32_t value;
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);
	void __iomem *reg_base = plat->reg_base;

	se_clock_on(dev);

	/* trigger GEU update */
	writel(0x0c424000, reg_base + GEU_CONFIG);
	mdelay(200);
	writel(0x0c024000, reg_base + GEU_CONFIG);

	timeout = 3000;
	while (1) {
		value = readl(reg_base + EFUSE_STATUS);
		if (value & FUSE_READY) {
			break;
		}

		timeout--;
		mdelay(1);
		if (!timeout) {
			log_err("efuse operation timeout.\n");
			se_clock_off(dev);
			return -2;
		}
	}

	se_clock_off(dev);
	return 0;
}

int efuse_read_bank(struct udevice *dev, int offset, void *buf, int size)
{
	uint8_t *ptr;
	int bank_end;
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);

	bank_end = (offset + size + FUSE_BANK_BYTES - 1) / FUSE_BANK_BYTES;

	if ((!buf) || bank_end > 16 || offset < 0) {
		log_err("invalid parameters. %d: %d\n", offset, size);
		return -1;
	}

	if (plat->efuse_need_reload) {
		efuse_reload(dev);
		efuse_load_all(dev);
		plat->efuse_need_reload = 0;
	}

	ptr = (uint8_t *)&(plat->efuse);
	memcpy(buf, ptr + offset, size);

	return 0;
}

/* write  data to efuse, buffer point to 256bit bank data*/
int efuse_write_bank_core(struct udevice *dev, uint32_t bank_index, uint32_t *buffer)
{
	uint32_t value, i = 0;
	uint32_t select_fuse_b = 0;
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);
	void __iomem *reg_base = plat->reg_base;

	se_clock_on(dev);
	value = (readl(reg_base + EFUSE_STATUS) >> 14) & 0x7fff;
	if (value & (0x1 << bank_index)) {
		log_err("bank_index(%d) locked\n", bank_index);
		se_clock_off(dev);
		return -2;
	}

	/*
	------------------------------------------
	 initial fuse block num: bank_index
	------------------------------------------
	SCLK high level timer must between 11000ns and 13000ns
	EFUSE_SCLK_DIV_CNTR only takes effect when geu_config[27:25] = 0x7
	*/

	// 1S/(104MHz/0x9c4)=24.04us divded by 2 (high level) is 12.02us
	writel(0x9c4, reg_base + EFUSE_SCLK_DIV_CNTR);
	log_debug("GEU_CONFIG is 0x%x\n", readl(reg_base + GEU_CONFIG));
	for (i = 0; i < FUSE_BANK_WORDS; i++) {
		writel(buffer[i], reg_base + (EFUSE_PROG_VAL1 + i * 4));
	}

	for (select_fuse_b = 0; select_fuse_b < 2; select_fuse_b++) {
		log_debug("select_fuse_b is 0x%x start\n", select_fuse_b);
		/*CLOCK_DIVIDER:7|bank_index|HIGH_VOLT_ENABLE|BURN_FUSE_ENABLE|ENABLE_SOFT_FUSE_PROG|SEL_FUSE_B*/
		writel(0x0e030000 | (select_fuse_b << 13) | ((bank_index % 16) << 18), reg_base + GEU_CONFIG); // 0x0e0b6000
		while (1) {
			value = readl(reg_base + EFUSE_STATUS);
			log_debug("EFUSE_STATUS(expect FUSE_BURN_DONE) is 0x%x\n", value);
			if (value & FUSE_BURN_DONE)
				break;
		}

		/*CLOCK_DIVIDER:6|HIGH_VOLT_ENABLE|ENABLE_SOFT_FUSE_PROG*/
		writel(0x0c024000, reg_base + GEU_CONFIG); // write cfg of GEU to finish burn
		mdelay(100);

		/*CLOCK_DIVIDER:6|FUSE_SOFTWARE_RESET|HIGH_VOLT_ENABLE|ENABLE_SOFT_FUSE_PROG*/
		writel(0x0c424000, reg_base + GEU_CONFIG); // write FUSE_SOFTWARE_RESET to start update efuse value to regs
		mdelay(200);
		writel(0x0c024000, reg_base + GEU_CONFIG); // write cfg of GEU to finish update
		while (1) {
			value = readl(reg_base + EFUSE_STATUS);
			log_debug("EFUSE_STATUS(expect FUSE_READY) is 0x%x\n", value);
			if (value & FUSE_READY)
				break;
		}
		log_debug("select_fuse_b is 0x%x end\n", select_fuse_b);
	}

	se_clock_off(dev);
	plat->efuse_need_reload = 1;

	return 0;
}

int efuse_write_bank(struct udevice *dev, int offset, const void *buf, int size)
{
	int bank_start, bank_end, i, j, byte_offset, byte_size;
	uint8_t efuse_data[FUSE_BANK_BYTES];
	uint8_t efuse_cmp_data[FUSE_BANK_BYTES];

	byte_offset = offset % FUSE_BANK_BYTES;
	bank_start = offset / FUSE_BANK_BYTES;
	bank_end = (offset + size + FUSE_BANK_BYTES - 1) / FUSE_BANK_BYTES;

	if ((!buf) || bank_end > FUSE_MAX_BANK_NUM || bank_start < 8) {
		log_err("efuse_write_bank: invalid parameters. %d: %d\n", offset, size);
		return -1;
	}

	for (i = 0; bank_start < bank_end; bank_start++) {
		// read original efuse data
		efuse_read_bank(dev, bank_start * FUSE_BANK_BYTES,
						efuse_data, FUSE_BANK_BYTES);

		memcpy(efuse_cmp_data, efuse_data, FUSE_BANK_BYTES);
		byte_size = min(size, FUSE_BANK_BYTES - byte_offset);
		for (j = 0; j < size; j++) {
			efuse_data[byte_offset + j] |=  ((uint8_t*)buf)[i + j];
		}

		// no need to program if data is NOT changed
		if (0 != memcmp(efuse_cmp_data, efuse_data, FUSE_BANK_BYTES)) {
			efuse_write_bank_core(dev, bank_start, (uint32_t*)efuse_data);
#ifdef EFUSE_DEBUG
			print_buffer(0, efuse_data, 1, FUSE_BANK_BYTES, 16);
#endif
			efuse_read_bank(dev, bank_start * FUSE_BANK_BYTES,
							efuse_cmp_data, FUSE_BANK_BYTES);
			if (0 != memcmp(efuse_cmp_data, efuse_data, FUSE_BANK_BYTES)) {
				print_buffer(0, efuse_cmp_data, 1, FUSE_BANK_BYTES, 16);
				log_err("Efuse write fail\n");
				return -2;
			}
		}

		byte_offset = 0;
		i += byte_size;
	}

	return 0;
}

int efuse_lock_bank(struct udevice *dev, uint32_t bank_index)
{
	uint32_t efuse_data[FUSE_BANK_WORDS];

	if (bank_index > FUSE_MAX_BANK_NUM || bank_index < 8) {
		log_err("efuse_lock_bank: invalid parameters. %d\n", bank_index);
		return -1;
	}

	memset((uint8_t *)efuse_data, 0x0, sizeof(efuse_data));
	efuse_data[0] |= 1 << bank_index;
	/*bank 6, 16bits  hardware lock*/
	return efuse_write_bank_core(dev, 6, (uint32_t *)efuse_data);
}

static int spacemit_efuse_read(struct udevice *dev, int offset, void *buf, int size)
{
	return efuse_read_bank(dev, offset, buf, size);
}

static __maybe_unused int spacemit_efuse_program(struct udevice *dev, int offset, const void *buf, int size)
{
	int ret;

	ret = efuse_power_on(dev);
	if (0 == ret){
		ret = efuse_write_bank(dev, offset, buf, size);
		efuse_power_off(dev);
	}
	return ret;
}

static const struct misc_ops spacemit_efuse_ops = {
	.read = spacemit_efuse_read,
#if !defined(CONFIG_SPL_BUILD)
	.write = spacemit_efuse_program,
#endif
};

static struct udevice* find_efuse_regulator(void)
{
	int ret;
	const char *name;
	char *path, *path_origin, *regulator_name;
	struct udevice *rdev = NULL;

	name = fdt_get_alias(gd->fdt_blob, "efuse_power");
	if (NULL == name) {
		pr_err("fail to get alias node efuse_power\n");
		return NULL;
	}

	path_origin = strdup(name);
	path = path_origin;
	// search the last node in the path
	while (NULL != path) {
		regulator_name = strsep(&path, "/");
	}
	pr_debug("Find regulator %s\n", regulator_name);

	ret = regulator_get_by_devname(regulator_name, &rdev);
	if (ret) {
		pr_err("fail to get regulatore device %s\n", regulator_name);
	}

	free(path_origin);
	return rdev;
}

static int spacemit_efuse_of_to_plat(struct udevice *dev)
{
	int ret;
	struct spacemit_efuse_plat *plat = dev_get_plat(dev);

	plat->reg_base = dev_read_addr_ptr(dev);
	plat->efuse_need_reload = 1;

	ret = clk_get_bulk(dev, &plat->clks);
	if (ret) {
		pr_err("Can't get clk: %d\n", ret);
		return ret;
	}

	ret = reset_get_bulk(dev, &plat->resets);
	if (ret) {
		pr_err("Can't get reset: %d\n", ret);
		return ret;
	}

	plat->regulator = find_efuse_regulator();
	return 0;
}

static const struct udevice_id spacemit_efuse_ids[] = {
	{ .compatible = "spacemit,k1x-efuse" },
	{}
};

U_BOOT_DRIVER(spacemit_k1x_efuse) = {
	.name = "spacemit_k1x_efuse",
	.id = UCLASS_MISC,
	.of_match = spacemit_efuse_ids,
	.of_to_plat = spacemit_efuse_of_to_plat,
	.plat_auto = sizeof(struct spacemit_efuse_plat),
	.ops = &spacemit_efuse_ops,
};

#if defined(EFUSE_DEBUG) && !defined(CONFIG_SPL_BUILD)
static int dump_efuses(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct udevice *dev;
	uint8_t fuses[FUSE_BANK_BYTES];
	int ret, i;

	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
			DM_DRIVER_GET(spacemit_k1x_efuse), &dev);
	if (ret) {
		pr_err("%s: no misc-device found\n", __func__);
		return 0;
	}

	for (i = 0; i < FUSE_MAX_BANK_NUM; i++) {
		ret = misc_read(dev, i * FUSE_BANK_BYTES, fuses, sizeof(fuses));
		if (ret < 0) {
			pr_err("%s: misc_read failed\n", __func__);
			return 0;
		}

		pr_info("efuse bank %d:\n", i);
		print_buffer(0, fuses, 1, FUSE_BANK_BYTES, 16);
	}

	return 0;
}

U_BOOT_CMD(
	dump_efuses, 1, 1, dump_efuses,
	"Dump the content of the efuses",
	""
);
#endif
