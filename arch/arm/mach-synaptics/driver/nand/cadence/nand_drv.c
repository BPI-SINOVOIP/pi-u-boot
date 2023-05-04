/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#include "nand_drv.h"
#include "errno.h"
#include "global.h"

#define UNUSED(var) do { (void)(var); } while(0)

#define NAND_READ_MULTIPAGE_MODE
#define NAND_WRITE_MULTIPAGE_MODE

#define MAX_RW_PAGE_CNT 256
#define MAX_ERASE_BLK_CNT 256

static blk_dev_nand_t instance;

#ifdef CONFIG_NAND_RANDOMIZER
#include "nand_randomizer.h"

#define RANDOMIZER_BUFF_SIZE 4096
unsigned char randomizer_buff[RANDOMIZER_BUFF_SIZE];
#endif

#define PAGE_BUF_SIZE (MAX_PAGE_SIZE + MAX_OOB_SIZE)
unsigned int page_buf[PAGE_BUF_SIZE / 4]; //make sure the address is four_bytes_aligned

static unsigned int timing_preset[NUM_SUPPORTED_DEVICES][5] =
{
	{   // onfi
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRH,         5)|
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRP,         9)|
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWH,         9)|
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWP,         9),

		TVAL2REG(TIMINGS0__tADL,                    39)|
		TVAL2REG(TIMINGS0__tCCS,                    99)|
		TVAL2REG(TIMINGS0__tWHR,                    23)|
		TVAL2REG(TIMINGS0__tRHW,                    39),

		TVAL2REG(TIMINGS1__tRHZ,                    39)|
		TVAL2REG(TIMINGS1__tWB,                     41)|
		TVAL2REG(TIMINGS1__tCWAW,                   255)|
		TVAL2REG(TIMINGS1__tVDLY,                   255),

		TVAL2REG(TIMINGS2__tFEAT,                   199)|
		TVAL2REG(TIMINGS2__CS_hold_time,            3)|
		TVAL2REG(TIMINGS2__CS_setup_time,           19),

		TVAL2REG(PHY_CTRL_REG__phony_dqs_timing,    9),
	},
	{   // toshiba 4KB page (TC58NVG2S0HTAI0)
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRH,         1)|     //10ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRP,         2)|     //12ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWH,         1)|     //10ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWP,         2),     //12ns

		TVAL2REG(TIMINGS0__tADL,                    1)|     //10ns
		TVAL2REG(TIMINGS0__tCCS,                    99)|
		TVAL2REG(TIMINGS0__tWHR,                    11)|    //60ns
		TVAL2REG(TIMINGS0__tRHW,                    5),     //30ns

		TVAL2REG(TIMINGS1__tRHZ,                    11)|    //60ns (max)
		TVAL2REG(TIMINGS1__tWB,                     21)|    //100ns (max)
		TVAL2REG(TIMINGS1__tCWAW,                   255)|
		TVAL2REG(TIMINGS1__tVDLY,                   255),

		TVAL2REG(TIMINGS2__tFEAT,                   199)|
		TVAL2REG(TIMINGS2__CS_hold_time,            0)|     //5ns
		TVAL2REG(TIMINGS2__CS_setup_time,           3),     //20ns

		TVAL2REG(PHY_CTRL_REG__phony_dqs_timing,    4),     //tRP, 2~6 work
	},
	{   // micron 2KB page (29F16G08CBACA)
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRH,         1)|     //10ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRP,         2)|     //12ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWH,         1)|     //10ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWP,         2),     //12ns

		TVAL2REG(TIMINGS0__tADL,                    13)|    //70ns
		TVAL2REG(TIMINGS0__tCCS,                    99)|
		TVAL2REG(TIMINGS0__tWHR,                    11)|    //60ns
		TVAL2REG(TIMINGS0__tRHW,                    19),    //100ns

		TVAL2REG(TIMINGS1__tRHZ,                    19)|    //100ns (max)
		TVAL2REG(TIMINGS1__tWB,                     21)|    //100ns (max)
		TVAL2REG(TIMINGS1__tCWAW,                   255)|
		TVAL2REG(TIMINGS1__tVDLY,                   255),

		TVAL2REG(TIMINGS2__tFEAT,                   199)|   //1000ns (max)
		TVAL2REG(TIMINGS2__CS_hold_time,            0)|     //5ns
		TVAL2REG(TIMINGS2__CS_setup_time,           3),     //20ns

		TVAL2REG(PHY_CTRL_REG__phony_dqs_timing,    4),     //3~6 work (5~8 work for 8KB samsung)
	},
	{   // sumsung 8KB page (K9GBG08U0A, MLC ecc requirement not supported, only as test purpose)
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRH,         1)|     //10ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRP,         2)|     //12ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWH,         1)|     //10ns
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWP,         2),     //12ns

		TVAL2REG(TIMINGS0__tADL,                    59)|    //300ns
		TVAL2REG(TIMINGS0__tCCS,                    99)|
		TVAL2REG(TIMINGS0__tWHR,                    23)|    //120ns
		TVAL2REG(TIMINGS0__tRHW,                    19),    //100ns

		TVAL2REG(TIMINGS1__tRHZ,                    19)|    //100ns (max)
		TVAL2REG(TIMINGS1__tWB,                     21)|    //100ns (max)
		TVAL2REG(TIMINGS1__tCWAW,                   255)|
		TVAL2REG(TIMINGS1__tVDLY,                   255),

		TVAL2REG(TIMINGS2__tFEAT,                   199)|   //1000ns (max)
		TVAL2REG(TIMINGS2__CS_hold_time,            0)|     //5ns
		TVAL2REG(TIMINGS2__CS_setup_time,           3),     //20ns

		TVAL2REG(PHY_CTRL_REG__phony_dqs_timing,    6),     //5~8 work
	},
	{   //dv (32G micron)
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRH,         1)|
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRP,         5)|
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWH,         1)|
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWP,         5),

		TVAL2REG(TIMINGS0__tADL,                    13)|
		TVAL2REG(TIMINGS0__tCCS,                    39)|
		TVAL2REG(TIMINGS0__tWHR,                    11)|
		TVAL2REG(TIMINGS0__tRHW,                    19),

		TVAL2REG(TIMINGS1__tRHZ,                    19)|
		TVAL2REG(TIMINGS1__tWB,                     19)|
		TVAL2REG(TIMINGS1__tCWAW,                   255)|
		TVAL2REG(TIMINGS1__tVDLY,                   255),

		TVAL2REG(TIMINGS2__tFEAT,                   199)|
		TVAL2REG(TIMINGS2__CS_hold_time,            1)|
		TVAL2REG(TIMINGS2__CS_setup_time,           3),

		TVAL2REG(PHY_CTRL_REG__phony_dqs_timing,    5),
	},
};

/* for the nand dma can't be byte-accessed,
 * we have to make sure both the address and size is 4-bytes-aligned.
 */
void * nand_dmamem_cpy(void * dst, const void * src, int n)
{
	unsigned int * d = dst;
	const unsigned int * s = src;

	if(((unsigned int)(uintptr_t)d & 0x3)
		|| ((unsigned int)(uintptr_t)s & 0x3)
		|| (n & 0x3)) {
		NAND_ERR("ERROR: please make sure your address and size is 4-bytes-aligned!!!\n");
		return NULL;
	}

	if(dst == src)
		return dst;

	while (n >= (int)sizeof(*d)) {
		*d++ = *s++;
		n -= sizeof(*d);
	}

	return(dst);
}

int diag_Nand_Set_Register(unsigned int offset, unsigned int value)
{
	NAND_DEBUG("Old: [0x%04x] = 0x%08X\n", offset, NFC_RD_REG(offset));
	NFC_WR_REG(offset, value);
	NAND_DEBUG("Write value 0x%08X\n", value);
	NAND_DEBUG("New: [0x%04x] = 0x%08X\n", offset, NFC_RD_REG(offset));
	return 0;
}

int diag_Nand_Config_Timing_All(unsigned int v0, unsigned int v1, unsigned int v2, unsigned int v3, unsigned int v4)
{
	diag_Nand_Set_Register(RA__ASYNC_TOGGLE_TIMINGS,    v0);
	diag_Nand_Set_Register(RA__TIMINGS0,                v1);
	diag_Nand_Set_Register(RA__TIMINGS1,                v2);
	diag_Nand_Set_Register(RA__TIMINGS2,                v3);
	diag_Nand_Set_Register(RA__PHY_CTRL_REG,            v4);
	return 0;
}

int diag_Nand_Config_Timing_Preset(unsigned int idx)
{
	NAND_DEBUG("Applying preset %d...\n", idx);
	return diag_Nand_Config_Timing_All(timing_preset[idx][0], timing_preset[idx][1], timing_preset[idx][2], timing_preset[idx][3], timing_preset[idx][4]);
}

static void fill_oob_data(uint8_t *oob_data, uint8_t *buf)
{
	uint32_t i;
	blk_dev_nand_t *h = &instance;

	if(buf) {
		for (i = 0; i < h->spare_bytes_per_page; i++)
			*oob_data++ = buf[i];
	} else {
		for (i = 0; i < h->spare_bytes_per_page; i++)
			*oob_data++ = 0xFF;
	}
}

static int32_t wait_until_thread_is_idle(
		blk_dev_nand_t *h,
		uint32_t thread_num)
{
	int32_t ret = 0;
	uint32_t timeout = h->timeout_val;

	if (timeout == 0)
		timeout = DEFAULT_TIMEOUT_COUNT;

	while (THREAD_IS_BUSY(thread_num) && --timeout)
		NFC_DELAY_US(h->delay_int_val);

	if (THREAD_IS_BUSY(thread_num) && (timeout == 0)) {
		NAND_ERR("%s(%d): Time out!\n", __FUNCTION__, __LINE__);
		ret = -ETIMEDOUT;
	}

	NFC_WR_FLD_NAME(INTR_STATUS__cmd_ignored, 0x1);

	return ret;
}

static int32_t program_command_registers(
		uint32_t cmd0,
		uint32_t cmd1,
		uint32_t cmd2,
		uint32_t cmd3)
{
	if (cmd0 == 0) {
		NAND_DEBUG("CMD0 must not be NULL!\n");
		return -EINVAL;
	}

	if (cmd1 != 0)
		NFC_WR_REG_NAME(CMD_REG1, cmd1);

	if (cmd2 != 0)
		NFC_WR_REG_NAME(CMD_REG2, cmd2);

	if (cmd3 != 0)
		NFC_WR_REG_NAME(CMD_REG3, cmd3);

	NFC_WR_REG_NAME(CMD_REG0, cmd0);

	return 0;
}

static int32_t check_cmd_ignored_flag(uint32_t thread_num)
{
	uint32_t val;

	NFC_RD_FLD_NAME(INTR_STATUS__cmd_ignored, val);
	val = (val >> thread_num) & 0x1;
	if (val == 0)
		return 0;

	NAND_DEBUG("Command is IGNORED.\n");
	return -EBUSY;
}

static int32_t check_operation_status_pio(
		blk_dev_nand_t *h,
		uint32_t thread_num,
		uint32_t status_check_map)
{
	nfc_status_field_t status;
	uint32_t timeout = h->timeout_val;

	NFC_WR_FLD_NAME(CMD_STATUS_PTR__thrd_status_sel, thread_num);

	if (timeout == 0)
		timeout = DEFAULT_TIMEOUT_COUNT;

	do {
		NFC_DELAY_US(h->delay_int_val);
		status.u[0] = NFC_RD_REG_NAME(CMD_STATUS);
	} while (status.pio.complete != 0x1 && --timeout);

	if (status.pio.complete != 0x1 && timeout == 0) {
		NAND_ERR("%s(%d): Time out!\n", __FUNCTION__, __LINE__);
		return -ETIMEDOUT;
	}

	if (status.u[0] & status_check_map) {
		NAND_ERR("%s(%d): Error occurs, status = 0x%x.\n", __FUNCTION__, __LINE__, status.u[0]);
		return -EIO;
	}
	return 0;
}

static int32_t polling_rbn(blk_dev_nand_t *h)
{
	uint32_t timeout, ready, cnt = 0;

	timeout = h->timeout_val;
	if (timeout == 0)
		timeout = DEFAULT_TIMEOUT_COUNT;

	do {
		NFC_DELAY_US(h->delay_int_val);
		NFC_RD_FLD_NAME(RBN_SETTINGS__Rbn, ready);

		if (ready == 0x1)
			cnt++;
		else
			cnt = 0;

	} while ((cnt < h->stable_cnt) && --timeout);

	if ((cnt < h->stable_cnt) && (timeout == 0)) {
		NAND_ERR("%s(%d): Time out!\n", __FUNCTION__, __LINE__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int32_t command_reset_pio(
		blk_dev_nand_t *h,
		uint32_t reset_type,
		uint32_t row_addr)
{
	int32_t ret = -ENODEV;
	nfc_cmd_0_reg_t cmd0;
	nfc_cmd_1_reg_t cmd1;

	cmd0.u[0] = 0x0;
	cmd1.u[0] = 0x0;

	cmd0.pio.reset.ct = 0x1;
	cmd0.pio.reset.trd_num = 0;
	cmd0.pio.reset.interrupt = 0;

	switch (reset_type) {
	case RESET_TYPE_ASYNC:
		cmd0.pio.reset.cmd_type = CT_ASYNC_RESET;
		break;
	case RESET_TYPE_LUN:
		cmd0.pio.reset.cmd_type = CT_LUN_RESET;
		cmd1.pio.reset.row_addr = row_addr;
		break;
	default:
		NAND_DEBUG("Invalid reset type.\n");
	}

	ret = wait_until_thread_is_idle(h, 0);
	if (ret != 0)
		return ret;

	ret = program_command_registers(cmd0.u[0], cmd1.u[0], 0, 0);
	if (ret != 0)
		return ret;

	ret = check_cmd_ignored_flag(0);
	if (ret != 0)
		return ret;

	ret = check_operation_status_pio(h, 0, ERR_ALL);
	if (ret != 0)
		return ret;

	ret = wait_until_thread_is_idle(h, 0);
	if (ret != 0)
		return ret;

	ret = polling_rbn(h);
	if (ret != 0)
		return ret;

	return ret;
}

static int32_t nand_reset(blk_dev_nand_t *h)
{
	int32_t ret = -ENODEV;

	ret =  command_reset_pio(h, RESET_TYPE_ASYNC, 0);

	return ret;
}

static void diag_nand_set_register(const uint32_t offset, const uint32_t value)
{
	NAND_DEBUG("%s, reg 0x%x val before set 0x%x\n", __func__, offset, NFC_RD_REG(offset));
	NFC_WR_REG(offset, value);
	NAND_DEBUG("%s, reg 0x%x val after set 0x%x\n", __func__, offset, NFC_RD_REG(offset));
}

static void diag_nand_config_timing_all(
		uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4)
{
	diag_nand_set_register(RA__ASYNC_TOGGLE_TIMINGS, v0);
	diag_nand_set_register(RA__TIMINGS0, v1);
	diag_nand_set_register(RA__TIMINGS1, v2);
	diag_nand_set_register(RA__TIMINGS2, v3);
	diag_nand_set_register(RA__PHY_CTRL_REG, v4);
}

static void set_default_timing(nand_timing_t *p)
{
	p->tRH = 5;
	p->tRP = 9;
	p->tWH = 9;
	p->tWP = 9;
	p->tADL = 39;
	p->tCCS = 99;
	p->tWHR = 23;
	p->tRHW = 39;
	p->tRHZ  = 39;
	p->tWB   = 40;
	p->tCWAW = 255;
	p->tVDLY = 255;
	p->tFEAT = 199;
	p->CS_hold_time = 3;
	p->CS_setup_time = 19;
	p->PHY_CTRL_REG__phony_dqs_timing = 9;
}

static int32_t nand_set_timing(blk_dev_nand_t *h)
{
	uint32_t v0, v1, v2, v3, v4;
	nand_timing_t *p = &h->timings;

	set_default_timing(p);

	v0 = TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRH, p->tRH) |
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tRP, p->tRP) |
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWH, p->tWH) |
		TVAL2REG(ASYNC_TOGGLE_TIMINGS__tWP, p->tWP);

	v1 = TVAL2REG(TIMINGS0__tADL, p->tADL) |
		TVAL2REG(TIMINGS0__tCCS, p->tCCS) |
		TVAL2REG(TIMINGS0__tWHR, p->tWHR) |
		TVAL2REG(TIMINGS0__tRHW, p->tRHW);

	v2 = TVAL2REG(TIMINGS1__tRHZ, p->tRHZ) |
		TVAL2REG(TIMINGS1__tWB, p->tWB) |
		TVAL2REG(TIMINGS1__tCWAW, p->tCWAW) |
		TVAL2REG(TIMINGS1__tVDLY, p->tVDLY);

	v3 = TVAL2REG(TIMINGS2__tFEAT, p->tFEAT) |
		TVAL2REG(TIMINGS2__CS_hold_time, p->CS_hold_time) |
		TVAL2REG(TIMINGS2__CS_setup_time, p->CS_setup_time);

	v4 = TVAL2REG(PHY_CTRL_REG__phony_dqs_timing, p->PHY_CTRL_REG__phony_dqs_timing);

	diag_nand_config_timing_all(v0, v1, v2, v3, v4);

	return 0;
}

static void config_data_transfer_settings(
		uint32_t offset,
		uint32_t sector_cnt,
		uint32_t sector_size,
		uint32_t last_sector_size)
{
	nfc_reg_transfer_cfg_0_t cfg0;
	nfc_reg_transfer_cfg_1_t cfg1;

	cfg0.u[0] = NFC_RD_REG_NAME(TRANSFER_CFG_0);
	cfg1.u[0] = NFC_RD_REG_NAME(TRANSFER_CFG_1);

	NAND_DEBUG("%s, before\n", __func__);
	DUMP_FIELD_HEX(cfg0, offset);
	DUMP_FIELD_DEC(cfg0, sector_cnt);
	DUMP_FIELD_HEX(cfg1, sector_size);
	DUMP_FIELD_HEX(cfg1, last_sector_size);

	cfg0.offset = offset;
	cfg0.sector_cnt = sector_cnt;
	cfg1.sector_size = sector_size;
	cfg1.last_sector_size = last_sector_size;

	NFC_WR_REG_NAME(TRANSFER_CFG_0, cfg0.u[0]);
	NFC_WR_REG_NAME(TRANSFER_CFG_1, cfg1.u[0]);

	cfg0.u[0] = NFC_RD_REG_NAME(TRANSFER_CFG_0);
	cfg1.u[0] = NFC_RD_REG_NAME(TRANSFER_CFG_1);

	NAND_DEBUG("%s, after\n", __func__);
	DUMP_FIELD_HEX(cfg0, offset);
	DUMP_FIELD_DEC(cfg0, sector_cnt);
	DUMP_FIELD_HEX(cfg1, sector_size);
	DUMP_FIELD_HEX(cfg1, last_sector_size);
}

static void config_device_layout(
		uint32_t blk_addr_idx,
		uint32_t num_lun,
		uint32_t page_per_block,
		uint32_t multi_lun_en)
{
	nfc_reg_nf_dev_layout_t reg;

	reg.u[0] = NFC_RD_REG_NAME(NF_DEV_LAYOUT);
	NAND_DEBUG("%s, before\n", __func__);
	DUMP_FIELD_DEC(reg, blk_addr_idx);
	DUMP_FIELD_DEC(reg, LN);
	DUMP_FIELD_DEC(reg, lun_en);
	DUMP_FIELD_DEC(reg, PPB);

	reg.blk_addr_idx = blk_addr_idx;
	reg.LN = num_lun;
	reg.PPB = page_per_block;
	reg.lun_en = multi_lun_en;

	NFC_WR_REG_NAME(NF_DEV_LAYOUT, reg.u[0]);

	reg.u[0] = NFC_RD_REG_NAME(NF_DEV_LAYOUT);
	NAND_DEBUG("%s, after\n", __func__);
	DUMP_FIELD_DEC(reg, blk_addr_idx);
	DUMP_FIELD_DEC(reg, LN);
	DUMP_FIELD_DEC(reg, lun_en);
	DUMP_FIELD_DEC(reg, PPB);
}

static void config_ecc_settings(
		uint32_t ecc_en,
		uint32_t correction_strength,
		uint32_t scrambler_en,
		uint32_t erase_det_en,
		uint32_t erase_det_lvl)
{
	nfc_reg_ecc_config_0_t cfg0;
	nfc_reg_ecc_config_1_t cfg1;
	cfg0.u[0] = NFC_RD_REG_NAME(ECC_CONFIG_0);
	cfg1.u[0] = NFC_RD_REG_NAME(ECC_CONFIG_1);
	NAND_DEBUG("%s, before\n", __func__);
	DUMP_FIELD_DEC(cfg0, corr_str);
	DUMP_FIELD_DEC(cfg0, scrambler_en);
	DUMP_FIELD_DEC(cfg0, erase_det_en);
	DUMP_FIELD_DEC(cfg0, ecc_enable);
	DUMP_FIELD_DEC(cfg1, erase_det_lvl);

	cfg0.corr_str = correction_strength;
	cfg0.scrambler_en = scrambler_en;
	cfg0.erase_det_en = erase_det_en;
	cfg0.ecc_enable = ecc_en;
	cfg1.erase_det_lvl = erase_det_lvl;

	NFC_WR_REG_NAME(ECC_CONFIG_0, cfg0.u[0]);
	NFC_WR_REG_NAME(ECC_CONFIG_1, cfg1.u[0]);

	NAND_DEBUG("%s, after\n", __func__);
	cfg0.u[0] = NFC_RD_REG_NAME(ECC_CONFIG_0);
	cfg1.u[0] = NFC_RD_REG_NAME(ECC_CONFIG_1);
	DUMP_FIELD_DEC(cfg0, corr_str);
	DUMP_FIELD_DEC(cfg0, scrambler_en);
	DUMP_FIELD_DEC(cfg0, erase_det_en);
	DUMP_FIELD_DEC(cfg0, ecc_enable);
	DUMP_FIELD_DEC(cfg1, erase_det_lvl);
}

static void config_cache_settings(unsigned int wr_en, unsigned int rd_en)
{
	nfc_reg_cache_config_t reg;
	reg.u[0] = NFC_RD_REG_NAME(CACHE_CONFIG);
	NAND_DEBUG("%s, before\n", __func__);
	DUMP_FIELD_DEC(reg, cache_wr_en);
	DUMP_FIELD_DEC(reg, cache_rd_en);

	reg.cache_wr_en = wr_en;
	reg.cache_rd_en = rd_en;

	NFC_WR_REG_NAME(CACHE_CONFIG, reg.u[0]);

	reg.u[0] = NFC_RD_REG_NAME(CACHE_CONFIG);
	NAND_DEBUG("%s, after\n", __func__);
	DUMP_FIELD_DEC(reg, cache_wr_en);
	DUMP_FIELD_DEC(reg, cache_rd_en);
}

static int32_t nand_dev_setup(blk_dev_nand_t *h)
{
	uint32_t ppb, log2ppb, tmp, corr_str, erase_det_en;

	assert(h->page_size);
	assert(h->blk_size);

	ppb = h->blk_size/ h->page_size;
	h->ecc_size = h->ecc_strength * 15 / 8;

	switch (h->page_size / 1024) {
	case 2:
		h->sect_cnt = 1;
		h->sect_size = 0x0;
		break;
	case 4:
		h->sect_cnt = 2;
		h->sect_size = 0x800;
		break;
	case 8:
		h->sect_cnt = 4;
		h->sect_size = 0x800;
		break;
	default:
		NAND_DEBUG("Page size not supported.\n");
		return -EIO;
	}

	h->last_sect_size = 0x800 + h->spare_bytes_per_page;

	for (log2ppb = 0, tmp = 1; tmp < ppb; tmp <<= 1, log2ppb++);

	corr_str = h->ecc_strength >> 3;
	if (corr_str != 0)
		corr_str--;
	if (h->scrambler_en == 0)
		erase_det_en = 1;
	if (h->spare_bytes_per_page == 0)
		erase_det_en = 0;

	config_data_transfer_settings(0, h->sect_cnt, h->sect_size, h->last_sect_size);
	config_device_layout(log2ppb, 1, ppb, 0);
	config_ecc_settings(h->ecc_en, corr_str, h->scrambler_en, erase_det_en, 0);
	diag_Nand_Config_Timing_Preset(1); // 1 for TOSHIBA_TC58NVG2S0HTAI0
	config_cache_settings(1, 1);
	NFC_WR_REG_NAME(DMA_SETTINGS, 0x100FF);
	//nand_reset(h);

	return 0;
}

static void nand_enable_interrupts(uint32_t trderrmap)
{
	NFC_WR_REG_NAME(TRD_ERROR_INTR_EN, trderrmap);
}

static int nand_randmize_page(blk_dev_nand_t *h,unsigned int page_addr,
					const unsigned char *data_src, const unsigned char *oob_src,
					unsigned char *data_dst, unsigned char *oob_dst)
{
#ifdef CONFIG_NAND_RANDOMIZER
	unsigned int randomized_length = 0;
	int randomized = mv_nand_addr_randomized(page_addr);

	if(!randomized) {
		if(data_src && data_dst)
			memcpy(data_dst, data_src, h->page_size);

		if (oob_dst && oob_src)
			memcpy(oob_dst, oob_src, h->spare_bytes_per_page);
		return 0;
	}

	randomized_length = mv_nand_randomizer_randomize_page(page_addr,
		data_src, oob_src, data_dst, oob_dst);
	if (oob_src && oob_dst) {
		int i;
		for (i = 0; i < h->spare_bytes_per_page; i++) {
			if(oob_src[i] != 0xff)
				break;
		}
		if (i == h->spare_bytes_per_page) {
			memset(oob_dst, 0xff, h->spare_bytes_per_page);
		}
	}

	return randomized_length > 0 ? 1 : 0;
#else
	UNUSED(h);
	UNUSED(page_addr);
	UNUSED(data_src);
	UNUSED(data_dst);
	UNUSED(oob_src);
	UNUSED(oob_dst);
	return 0;
#endif
}

static void nand_read_data( blk_dev_nand_t *h,
		uint32_t page_addr,
		const void *src,
		void *dest,
		void *oob_data)
{
	nfc_status_field_t status;
#ifdef CONFIG_NAND_RANDOMIZER
	unsigned char * t_buf = (unsigned char *)page_buf;
	unsigned char * t_oob_buf = t_buf + h->page_size;
	memset(t_buf, 0xff, PAGE_BUF_SIZE);
#else
	unsigned char * t_buf = dest;
	unsigned char * t_oob_buf = oob_data;

	if (!oob_data)
		t_oob_buf = (unsigned char *)page_buf + h->page_size;
#endif

	//Note: nand dma buffer can't be byte-accessed.
	nand_dmamem_cpy(t_buf, src, h->page_size);
	nand_dmamem_cpy(t_oob_buf, src + h->page_size, h->spare_bytes_per_page);

	status.u[0] = NFC_RD_REG_NAME(CMD_STATUS);
	if (status.pio.erased_page) {
		if (oob_data)
			memset(oob_data, 0xff, h->spare_bytes_per_page);
		if (dest)
			memset(dest, 0xff, h->page_size);
	} else {
		nand_randmize_page(h, page_addr, t_buf, oob_data ? t_oob_buf: NULL,
									dest, oob_data);
	}
	NAND_DEBUG("read page 0x%x\n", page_addr);
}

static void nand_write_data( blk_dev_nand_t *h,
		uint32_t page_addr,
		const void *src,
		void *dest,
		void *oob_data)
{
#ifdef CONFIG_NAND_RANDOMIZER
	unsigned char * t_buf = (unsigned char *)page_buf;
	unsigned char * t_oob_buf = t_buf + h->page_size;
	memset(t_buf, 0xff, PAGE_BUF_SIZE);
#else
	unsigned char * t_buf = (unsigned char *)src;
	unsigned char * t_oob_buf = oob_data;

	if (!oob_data)
		t_oob_buf = (unsigned char *)page_buf + h->page_size;
#endif

	nand_randmize_page(h, page_addr, src, oob_data, t_buf, t_oob_buf);

	//Note: nand dma buffer can't be byte-accessed.
	nand_dmamem_cpy(dest, t_buf, h->page_size);
	nand_dmamem_cpy(dest + h->page_size, t_oob_buf, h->spare_bytes_per_page);
}

static int sdma_access_multipage(
		blk_dev_nand_t *h,
		const uint32_t page_addr,
		const uint32_t mem_addr,
		unsigned int total_size,
		int include_oob,
		const int32_t direction)
{
	nfc_reg_intr_status_t reg_st;
	nfc_reg_sdma_size_t reg_size;
	uint32_t t, t_bk, curr_mem_addr = mem_addr;
	uint32_t tot_size = total_size;
	uint8_t oob_data[32];
	int32_t page_cnt = 0;

	t = h->timeout_val == 0 ? DEFAULT_TIMEOUT_COUNT : h->timeout_val;
	t_bk = t;

	if(!include_oob) // if don't include oob(not yaffs), fill with 0xff.
		fill_oob_data(oob_data, NULL);

	while(tot_size != 0) {
		t = t_bk;
		/*
		Before host master access the Slave DMA interface it needs to check if
		data transfer is allowed. This is done by software polling the
		sdma_trigg bit in the intr_status (0x0110) register.
		*/
		do {
			reg_st.u[0] = NFC_RD_REG_NAME(INTR_STATUS);
			NFC_DELAY_US(h->delay_int_val);
		} while ((reg_st.sdma_trigg == 0x0) && --t);

		if ((reg_st.sdma_trigg == 0x0) && (t == 0)) {
			NAND_ERR("%s(%d): Time out waiting sdma_trigg.\n", __FUNCTION__, __LINE__);
			return -ETIMEDOUT;
		}

		/*
		After host discovered that data transfer is allowed (using interrupts
		or software polling) it should read the sdma_size and the sdma_trd_num
		registers. The sdma_size (0x0440) register provides byte aligned data
		block size that need to be transfered. The sdma_trd_num (0x0444)
		register identifies command associated with this data transfer and
		allows to select valid address and direction for data transfer.
		*/
		reg_size.u[0] = NFC_RD_REG_NAME(SDMA_SIZE);
		if (reg_size.sdma_size > NFC_SDMA_PORT_SIZE) {
			NAND_ERR("%s(%d): sdma_size (0x%x) is larger than total port size (0x%x).\n",
					__FUNCTION__, __LINE__,
					reg_size.sdma_size, NFC_SDMA_PORT_SIZE);
			return -EIO;
		}

		/*
		Before starting data transmission, host must clear the sdma_trigg flag
		by writing 1. After clearing this flag host can execute data
		transmission on slave DMA interface.
		*/

		NFC_WR_REG_NAME(INTR_STATUS, reg_st.u[0]);
		assert(reg_size.sdma_size == h->page_size + h->spare_bytes_per_page);
		NAND_DEBUG("%s, sdma size %d\n", __func__, reg_size.sdma_size);
		NAND_DEBUG("curr_mem_addr 0x%x\n", curr_mem_addr);
		if (direction == NAND_READ)
			if(include_oob)
				nand_read_data(h, (page_addr + page_cnt * h->page_size), (const void *)NFC_SDMA_PORT_ADDR,
						(void *)(uintptr_t)curr_mem_addr, (void *)(uintptr_t)(curr_mem_addr + h->page_size));
			else
				nand_read_data(h, (page_addr + page_cnt * h->page_size), (const void *)NFC_SDMA_PORT_ADDR,
						(void *)(uintptr_t)curr_mem_addr, oob_data);
		else if (direction == NAND_WRITE)
			if(include_oob)
				nand_write_data(h, (page_addr + page_cnt * h->page_size), (const void *)(uintptr_t)curr_mem_addr,
						(void *)(uintptr_t)NFC_SDMA_PORT_ADDR, (void *)(uintptr_t)(curr_mem_addr + h->page_size));
			else
				nand_write_data(h, (page_addr + page_cnt * h->page_size), (const void *)(uintptr_t)curr_mem_addr,
						(void *)(uintptr_t)NFC_SDMA_PORT_ADDR, oob_data);
		else
			assert(0);

		page_cnt ++;
		curr_mem_addr += h->page_size;
		tot_size -= h->page_size;
		tot_size -= h->spare_bytes_per_page;
		if(include_oob) {
			curr_mem_addr += h->spare_bytes_per_page;
		}
		NAND_DEBUG("total size 0x%x\n", tot_size);
	}

	return 0;
}

static uint32_t nand_access_pio_multipage(blk_dev_nand_t *h,
		const uint32_t page_addr,
		void *memory_addr,
		unsigned int page_cnt,
		int include_oob,
		const int32_t direction)
{
	uint32_t ret = 0;
	nfc_cmd_0_reg_t cmd0;
	nfc_cmd_1_reg_t cmd1;
	nfc_cmd_2_reg_t cmd2;
	unsigned int total_size = page_cnt * (h->page_size + h->spare_bytes_per_page);

	NAND_DEBUG("%s, nand %d addr 0x%x\n", __func__,direction, page_addr);
	NAND_DEBUG("mem 0x%p, of size %d\n", memory_addr, h->page_size);

	if(page_cnt > MAX_RW_PAGE_CNT) {
		NAND_DEBUG("Cannot read/write more than %d pages in PIO mode.\n", 0xff);
		return 1;
	}

	nand_enable_interrupts(0);

	cmd0.u[0] = 0x0;
	cmd1.u[0] = 0x0;
	cmd2.u[0] = 0x0;

	cmd0.pio.read.ct = 0x1;
	cmd0.pio.read.trd_num = 0;
	cmd0.pio.read.dma_sel = DMA_SEL_SLAVE;
	cmd0.pio.read.interrupt = 0;
	if (direction == NAND_READ)
		cmd0.pio.read.cmd_type = CT_READ(page_cnt);
	else if (direction == NAND_WRITE)
		cmd0.pio.read.cmd_type = CT_PROGRAM(page_cnt);
	else
		assert(0);
	cmd1.pio.read.row_addr = page_addr / h->page_size;
	cmd2.pio.read.mem_addr_ptr_l = (uint32_t)(uintptr_t)memory_addr;

	ret = wait_until_thread_is_idle(h, 0);
	if (ret != 0)
		return 0;

	ret = program_command_registers(cmd0.u[0], cmd1.u[0], cmd2.u[0], 0);
	if (ret != 0)
		return 0;

	ret = check_cmd_ignored_flag(0);
	if (ret != 0)
		return 0;

	ret = sdma_access_multipage(h, page_addr, (uint32_t)(uintptr_t)memory_addr, total_size, include_oob, direction);
	if (ret != 0)
		return 0;

	ret = check_operation_status_pio(h, 0, ERR_ALL_EXCEPT_ECC_MAX);
	if (ret != 0)
		return 0;

	ret = wait_until_thread_is_idle(h, 0);
	if (ret != 0)
		return 0;

	ret = polling_rbn(h);
	if (ret != 0)
		return 0;

	NAND_DEBUG("done\n");
	return total_size;
}

static int sdma_access(
		blk_dev_nand_t *h,
		const uint32_t page_addr,
		const uint32_t mem_addr,
		void * oob_data,
		const int32_t direction)
{
	nfc_reg_intr_status_t reg_st;
	nfc_reg_sdma_size_t reg_size;
	uint32_t t, curr_mem_addr = mem_addr;

	t = h->timeout_val == 0 ? DEFAULT_TIMEOUT_COUNT : h->timeout_val;
	do {
		reg_st.u[0] = NFC_RD_REG_NAME(INTR_STATUS);
		NFC_DELAY_US(h->delay_int_val);
	} while ((reg_st.sdma_trigg == 0x0) && --t);

	if ((reg_st.sdma_trigg == 0x0) && (t == 0)) {
		NAND_ERR("%s(%d): Time out waiting sdma_trigg.\n", __FUNCTION__, __LINE__);
		return -ETIMEDOUT;
	}

	reg_size.u[0] = NFC_RD_REG_NAME(SDMA_SIZE);
	if (reg_size.sdma_size > NFC_SDMA_PORT_SIZE) {
		NAND_DEBUG("%s(%d): sdma_size (0x%x) is larger than total port size (0x%x).\n",
				__FUNCTION__, __LINE__,
				reg_size.sdma_size, NFC_SDMA_PORT_SIZE);
		return -EIO;
	}

	NFC_WR_REG_NAME(INTR_STATUS, reg_st.u[0]);
	assert(reg_size.sdma_size == h->page_size + h->spare_bytes_per_page);
	NAND_DEBUG("%s, sdma size %d\n", __func__, reg_size.sdma_size);
	NAND_DEBUG("curr_mem_addr 0x%x\n", curr_mem_addr);
	if (direction == NAND_READ)
		nand_read_data(h, page_addr, (const void *)NFC_SDMA_PORT_ADDR,
				(void *)(uintptr_t)curr_mem_addr, oob_data);
	else if (direction == NAND_WRITE)
		nand_write_data(h, page_addr, (const void *)(uintptr_t)curr_mem_addr,
				(void *)(uintptr_t)NFC_SDMA_PORT_ADDR, oob_data);
	else
		assert(0);

	return 0;
}

static uint32_t nand_access_pio(blk_dev_nand_t *h,
		const uint32_t page_addr,
		void *memory_addr,
		void *oob_data,
		const int32_t direction)
{
	uint32_t ret = 0;
	nfc_cmd_0_reg_t cmd0;
	nfc_cmd_1_reg_t cmd1;
	nfc_cmd_2_reg_t cmd2;

	NAND_DEBUG("%s, nand %d addr 0x%x\n", __func__,direction, page_addr);
	NAND_DEBUG("mem 0x%p, of size %d\n", memory_addr, h->page_size);

	nand_enable_interrupts(0);

	cmd0.u[0] = 0x0;
	cmd1.u[0] = 0x0;
	cmd2.u[0] = 0x0;

	cmd0.pio.read.ct = 0x1;
	cmd0.pio.read.trd_num = 0;
	cmd0.pio.read.dma_sel = DMA_SEL_SLAVE;
	cmd0.pio.read.interrupt = 0;
	if (direction == NAND_READ)
		cmd0.pio.read.cmd_type = CT_READ(1);
	else if (direction == NAND_WRITE)
		cmd0.pio.read.cmd_type = CT_PROGRAM(1);
	else
		assert(0);
	cmd1.pio.read.row_addr = page_addr / h->page_size;
	cmd2.pio.read.mem_addr_ptr_l = (uint32_t)(uintptr_t)memory_addr;

	ret = wait_until_thread_is_idle(h, 0);
	if (ret != 0)
		return 0;

	ret = program_command_registers(cmd0.u[0], cmd1.u[0], cmd2.u[0], 0);
	if (ret != 0)
		return 0;

	ret = check_cmd_ignored_flag(0);
	if (ret != 0)
		return 0;

	ret = sdma_access(h, page_addr, (uint32_t)(uintptr_t)memory_addr, oob_data, direction);
	if (ret != 0)
		return 0;

	ret = check_operation_status_pio(h, 0, ERR_ALL_EXCEPT_ECC_MAX);
	if (ret != 0)
		return 0;

	ret = wait_until_thread_is_idle(h, 0);
	if (ret != 0)
		return 0;

	ret = polling_rbn(h);
	if (ret != 0)
		return 0;

	return h->page_size;
}

static uint32_t nand_write_page(
		blk_dev_nand_t *h,
		const uint32_t page_addr,
		void *memory_addr,
		void *oob_data)
{
	return nand_access_pio(h, page_addr, memory_addr, oob_data, NAND_WRITE);
}

static uint32_t nand_read_page(
		blk_dev_nand_t *h,
		const uint32_t page_addr,
		void *memory_addr,
		void *oob_data)
{
	return nand_access_pio(h, page_addr, memory_addr, oob_data, NAND_READ);
}

static uint32_t nand_write_multipage(
		blk_dev_nand_t *h,
		const uint32_t page_addr,
		void *memory_addr,
		unsigned int page_cnt,
		int include_oob)
{
	return nand_access_pio_multipage(h, page_addr, memory_addr, page_cnt, include_oob, NAND_WRITE);
}

static uint32_t nand_read_multipage(
		blk_dev_nand_t *h,
		const uint32_t page_addr,
		void *memory_addr,
		unsigned int page_cnt,
		int include_oob)
{
	return nand_access_pio_multipage(h, page_addr, memory_addr, page_cnt, include_oob, NAND_READ);
}

void reset_nand_controller(void)
{
	unsigned int regval;

	regval = hal_read32(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_perifReset);
	regval = (regval & ~MSK32Gbl_perifReset_nfcSysSyncReset &
			~MSK32Gbl_perifReset_nfcRegSyncReset)
		| (1 << LSb32Gbl_perifReset_nfcSysSyncReset)
		| (1 << LSb32Gbl_perifReset_nfcRegSyncReset);
	hal_write32(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_perifReset,regval);

	do {
		regval = hal_read32(MEMMAP_CHIP_CTRL_REG_BASE +
				RA_Gbl_PERIF + RA_PERIF_NAND_STATUS);
	} while(!(regval & 0x1));

	hal_write32(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_perifReset, 0x0);
}

static int32_t nand_ctrl_reset(blk_dev_nand_t *h)
{
	int32_t ret = -ENODEV;

	reset_nand_controller();

	ret = nand_reset(h);
	if (ret != 0)
		return ret;

	ret = nand_set_timing(h);
	if (ret != 0)
		return ret;

	ret = nand_dev_setup(h);

	return ret;
}

static int check_operation_status_GEN(
		unsigned int thread_num,
		unsigned int timeout,
		unsigned int status_check_map)
{
	int ret =               0;
	nfc_status_field_t      status;
	unsigned int            controller_busy;
	unsigned int            timeout_bak = timeout;

	NFC_WR_FLD_NAME(CMD_STATUS_PTR__thrd_status_sel, thread_num);
	NAND_DEBUG("Checking status for thread %d...\n", thread_num);
	NAND_DEBUG("Waiting last operation status...");
	if (timeout == 0)
	{
		timeout = 0xffffffff;
	}
	do
	{
		NFC_DELAY_US(DEFAULT_DELAY_INTERVAL);
		status.u[0] = NFC_RD_REG_NAME(CMD_STATUS);
	} while (status.gen.complete != 0x1 && --timeout);
	if (status.gen.complete != 0x1 && timeout == 0)
	{
		NAND_ERR("%s(%d): Time out!\n", __FUNCTION__, __LINE__);
		return 1;
	}
	else
	{
		NAND_DEBUG("Completed.\n");
	}
	if (status.u[0] & status_check_map)
	{
		NAND_ERR("%s(%d): Error occurs:\n", __FUNCTION__, __LINE__);
		DUMP_FIELD_DEC(status.gen, cmd_err);
		DUMP_FIELD_DEC(status.gen, ecc_err);
		DUMP_FIELD_DEC(status.gen, max_err);
		DUMP_FIELD_DEC(status.gen, erased_page);
		DUMP_FIELD_DEC(status.gen, fail);
		DUMP_FIELD_DEC(status.gen, bus_err);
		ret |= 1;
	}
	/*
	Please note that in Generic Work Mode selected thread finishes its work
	when appropriate command request is sent to the Mini-controller module. It
	may take some additional time to process the command by Mini-controller and
	send it to the NF device. If host needs to make sure that all commands were
	finished on NF interface, it can check controller operation status by
	polling the ctrl_busy bit in the ctrl_status (0x0118) register.
	*/
	NAND_DEBUG("Waiting command to finish on NF interface...");
	timeout = timeout_bak;
	if (timeout == 0)
	{
		timeout = 0xffffffff;
	}
	do
	{
		NFC_DELAY_US(DEFAULT_DELAY_INTERVAL);
		NFC_RD_FLD_NAME(CTRL_STATUS__ctrl_busy, controller_busy);
	} while (controller_busy && --timeout);
	if (controller_busy && (timeout == 0))
	{
		NAND_ERR("%s(%d): Time out!\n", __FUNCTION__, __LINE__);
		return 1;
	}
	else
	{
		NAND_DEBUG("Completed.\n");
	}
	return ret;
}

static int command_generic(
		unsigned int        interrupt_en,
		unsigned int        thread_num,
		unsigned long long  parameter)
{
	nfc_cmd_0_reg_t                         cmd0;
	nfc_cmd_layout_t                        cmd23;
	int ret =                               0;

	cmd0.u[0] =                             0x0;
	cmd0.gen.interrupt =                    interrupt_en;
	cmd0.gen.trd_num =                      thread_num;
	cmd0.gen.ct =                           0x3;

	cmd23.ul =        parameter;

	ret |= program_command_registers(cmd0.u[0], 0, cmd23.u[0], cmd23.u[1]);
	ret |= check_cmd_ignored_flag(thread_num);

	return ret;
}

int diag_Nand_Read_ID(uint32_t        id_buf, uint32_t * id_size)
{
	blk_dev_nand_t *h = &instance;
	unsigned int id_sz = 8;
	unsigned int i;
	int ret = 0;

	// set default value before nand init
	h->timeout_val = DEFAULT_TIMEOUT_COUNT;
	h->delay_int_val = DEFAULT_DELAY_INTERVAL;
	h->stable_cnt = 2;
	h->page_size = 1024; // before we get real page size, set it to 1024
						 // in order to copy data from sdma buff

	nand_ctrl_reset(h);

	nand_enable_interrupts(0);

	ret |= polling_rbn(h);
	ret |= wait_until_thread_is_idle(h, 0);
	ret |= command_generic(0, 0,
			CMD_PARAM(INST_TYPE,            IT_READ_ID          )|
			CMD_PARAM(TWB_ACTIVE,           0                   )|
			CMD_PARAM(JEDEC_SUPP,           0                   )|
			CMD_PARAM(ADDR0,                0x00                ));
	ret |= check_operation_status_GEN(0, h->timeout_val, ERR_ALL_GEN);

	ret |= command_generic(0, 0,
			CMD_PARAM(INST_TYPE,            IT_DATA_SEQ         )|
			CMD_PARAM(TWB_ACTIVE,           0                   )|
			CMD_PARAM(JEDEC_SUPP,           0                   )|
			CMD_PARAM(DIRECTION,            DIRECTION_READ      )|
			CMD_PARAM(ECC_EN,               0                   )|
			CMD_PARAM(SCRAMBLER_EN,         0                   )|
			CMD_PARAM(ERASED_PAGE_DET_EN,   0                   )|
			CMD_PARAM(SECTOR_SIZE,          0                   )|  //
			CMD_PARAM(SECTOR_CNT,           1                   )|  //
			CMD_PARAM(LAST_SECTOR_SIZE,     id_sz               )|  // read 2 bytes
			CMD_PARAM(CORR_CAP,             0                   ));
	// if not reading page, just give address 0 to avoid doing randomier
	ret |= sdma_access(h, 0, id_buf, (void *)(uint64_t)(id_buf+h->page_size), NAND_READ);
	ret |= check_operation_status_GEN(0, h->timeout_val, ERR_ALL_GEN);
	ret |= wait_until_thread_is_idle(h, 0);
	ret |= polling_rbn(h);
	*id_size = id_sz;
	NAND_DEBUG("ID:\n");
	for (i=0; i<id_sz; i++)
	{
		NAND_DEBUG("%02X ", *U2PC(id_buf++));
	}
	NAND_DEBUG("\n");
	return ret;
}

void nand_drv_open(const uint32_t blk_size,
		const uint32_t page_size,
		const uint32_t ecc_strength,
		const uint32_t scrambler_en,
		const uint32_t oob_size)
{
	blk_dev_nand_t *h = &instance;

	h->blk_size = blk_size;
	h->page_size = page_size;
	h->spare_bytes_per_page = oob_size;
	if(ecc_strength == 0)
		h->ecc_en = 0;
	else
		h->ecc_en = 1;
	h->ecc_strength = ecc_strength;
	h->scrambler_en = scrambler_en;
	h->timeout_val = DEFAULT_TIMEOUT_COUNT;
	h->delay_int_val = DEFAULT_DELAY_INTERVAL;
	h->stable_cnt = 2;
	NAND_DEBUG("blk_size = %d\n", h->blk_size);
	NAND_DEBUG("page_size = %d\n", h->page_size);
	NAND_DEBUG("spare_bytes_per_page = %d\n", h->spare_bytes_per_page);
	NAND_DEBUG("ecc_en = %d\n", h->ecc_en);
	NAND_DEBUG("ecc_strength = %d\n", h->ecc_strength);
	NAND_DEBUG("scrambler_en = %d\n", h->scrambler_en);

	assert(h->page_size <= MAX_PAGE_SIZE);
	assert(h->spare_bytes_per_page <= MAX_OOB_SIZE);

	nand_ctrl_reset(h);

#ifdef CONFIG_NAND_RANDOMIZER
	mv_nand_randomizer_init(h->blk_size, h->page_size, h->spare_bytes_per_page,
							randomizer_buff, RANDOMIZER_BUFF_SIZE);
	NAND_DEBUG("enable nand randomizer\n");
#endif
}

static int32_t is_bad_page(uint32_t page_addr)
{
	blk_dev_nand_t *h = &instance;
	uint8_t * page = (uint8_t *)page_buf;

	assert(h->page_size + h->spare_bytes_per_page < MAX_PAGE_SIZE + MAX_OOB_SIZE);

	uint32_t ret = nand_read_page(h, page_addr, page, &page[h->page_size]);
	if (ret != h->page_size) {
		NAND_DEBUG("nand_read_page return %d\n", ret);
		return 1;
	}

	uint8_t bad_marker = page[h->page_size];
	if((bad_marker & 0xff)  != 0xff){
		NAND_DEBUG("page 0x%x bad marker is 0x%x\n", page_addr, bad_marker);
		return 1;
	}

	return 0;
}

int mv_nand_block_bad(loff_t ofs, __attribute__((unused)) int getchip)
{
	blk_dev_nand_t *h = &instance;
	uint32_t blk_addr = ofs / h->blk_size * h->blk_size;

	//if (nand_dev_setup(h))
	//	return 1;

	int32_t ret = is_bad_page(blk_addr);
	if (ret != 0)
		return 1;

	ret = is_bad_page(blk_addr + h->page_size);

	return ret;
}

int mv_nand_read_block_generic(loff_t nand_start, unsigned char* data_buf, int size, int is_oob)
{
	blk_dev_nand_t *h = &instance;
	uint8_t oob_data[32], *buf;
	int32_t nand_addr, page_offset;
	uint32_t i, ret, page_cnt, data_size, read_size = 0;
	uint8_t page[MAX_PAGE_SIZE + MAX_OOB_SIZE];
	uint32_t real_pagesize = h->page_size;

	//if (nand_dev_setup(h))
	//	return 1;

	if(is_oob)
		real_pagesize = h->page_size + h->spare_bytes_per_page;

	data_size = size;
	if ((nand_start % h->page_size) != 0) {
		nand_addr = ((nand_start + h->page_size) / h->page_size - 1) * h->page_size;
		ret = nand_read_page(h, nand_addr, page, &page[h->page_size]);
		if (ret != h->page_size) {
			NAND_DEBUG("nand_read_page return %d\n", ret);
			return -1;
		}

		page_offset = nand_start % h->page_size;
		read_size = real_pagesize - page_offset;
		if (read_size > data_size)
			read_size = data_size;
		memcpy(data_buf, &page[page_offset], read_size);
		data_buf += read_size;
		data_size -= read_size;
		nand_start = nand_addr + h->page_size;
	}

	page_cnt = data_size / real_pagesize;
	read_size = real_pagesize;
	for (i = 0; i < page_cnt; i++) {
		buf = ((uint32_t)(uintptr_t)data_buf % 4 == 0) ? (uint8_t *)data_buf : page;
		//ret = nand_read_page(h, nand_start, buf, oob_data);
		if(is_oob)
			ret = nand_read_page(h, nand_start, buf, buf + h->page_size);
		else
			ret = nand_read_page(h, nand_start, buf, oob_data);
		if (ret != h->page_size) {
			NAND_DEBUG("nand_read_page return %d\n", ret);
			return -1;
		}

		if (buf != (uint8_t *)data_buf)
			memcpy(data_buf, buf, read_size);
		nand_start += h->page_size;
		data_buf += read_size;
		data_size -= read_size;
	}

	//FIXME: if is_oob = 1 and data_size > page_size data_size < page_size + oob_size
	if (data_size != 0) {
		ret = nand_read_page(h, nand_start, page, &page[h->page_size]);
		if (ret != h->page_size) {
			NAND_DEBUG("nand_read_page return %d\n", ret);
			return -1;
		}

		memcpy(data_buf, page, data_size);
	}

	return size;
}

int mv_nand_read_block_generic_multipage(loff_t nand_start, unsigned char* data_buf, int size, int is_oob)
{
	blk_dev_nand_t *h = &instance;
	int32_t nand_addr, page_offset;
	uint32_t ret, page_cnt, data_size, read_size = 0;
	uint8_t page[MAX_PAGE_SIZE + MAX_OOB_SIZE];
	uint32_t real_pagesize = h->page_size;
	uint32_t pn_read = 0;

	//if (nand_dev_setup(h))
	//	return 1;

	if(is_oob)
		real_pagesize = h->page_size + h->spare_bytes_per_page;

	data_size = size;
	if ((nand_start % h->page_size) != 0) {
		nand_addr = ((nand_start + h->page_size) / h->page_size - 1) * h->page_size;
		ret = nand_read_page(h, nand_addr, page, &page[h->page_size]);
		if (ret != h->page_size) {
			NAND_DEBUG("nand_read_page return %d\n", ret);
			return -1;
		}

		page_offset = nand_start % h->page_size;
		read_size = real_pagesize - page_offset;
		if (read_size > data_size)
			read_size = data_size;
		memcpy(data_buf, &page[page_offset], read_size);
		data_buf += read_size;
		data_size -= read_size;
		nand_start = nand_addr + h->page_size;
	}

	page_cnt = data_size / real_pagesize;
	read_size = real_pagesize;
	for (; page_cnt > 0;) {
		pn_read = (page_cnt > MAX_RW_PAGE_CNT) ? MAX_RW_PAGE_CNT : page_cnt;
		nand_read_multipage(h, nand_start, data_buf, pn_read, is_oob);
		nand_start += (h->page_size * pn_read);
		data_buf += (read_size * pn_read);
		page_cnt -= pn_read;
		data_size -= (read_size * pn_read);
	}

	//FIXME: if is_oob = 1 and data_size > page_size data_size < page_size + oob_size
	if (data_size != 0) {
		ret = nand_read_page(h, nand_start, page, &page[h->page_size]);
		if (ret != h->page_size) {
			NAND_DEBUG("nand_read_page return %d\n", ret);
			return -1;
		}

		memcpy(data_buf, page, data_size);
	}

	return size;
}

int mv_nand_read_block(loff_t nand_start, unsigned char* data_buf, int size)
{
#ifdef NAND_READ_MULTIPAGE_MODE
	return mv_nand_read_block_generic_multipage(nand_start, data_buf, size, 0);
#else
	return mv_nand_read_block_generic(nand_start, data_buf, size, 0);
#endif
}

int mv_nand_read_block_withoob(loff_t nand_start, unsigned char* data_buf, int size)
{
#ifdef NAND_READ_MULTIPAGE_MODE
	return mv_nand_read_block_generic_multipage(nand_start, data_buf, size, 1);
#else
	return mv_nand_read_block_generic (nand_start, data_buf, size, 1);
#endif
}

int nand_page_size(void)
{
	return instance.page_size;
}

int nand_block_size(void)
{
	return instance.blk_size;
}

int nand_oob_size(void)
{
	return instance.spare_bytes_per_page;
}

int nand_ecc_strength(void)
{
	return instance.ecc_strength;
}

int nand_scramber_en(void)
{
	return instance.scrambler_en;
}

int mv_nand_write_block_generic(loff_t nand_start, const unsigned char* data_buf, int size, int is_oob)
{
	uint32_t i, page_cnt;
	blk_dev_nand_t *h = &instance;
	uint8_t *buf = (uint8_t *)data_buf;
	uint32_t real_pagesize = h->page_size;

	if(is_oob)
		real_pagesize = h->page_size  + h->spare_bytes_per_page;

	if(nand_start % h->page_size != 0) {
		NAND_DEBUG("write start address should be aligned with page size\n");
		return 1;
	}
	if(size % real_pagesize != 0) {
		NAND_DEBUG("the size of data should be aligned with page size(+oob if oob is enabled)\n");
		return 1;
	}

	//if (nand_dev_setup(h))
	//	return 1;

	page_cnt = size / real_pagesize;

	for (i = 0; i < page_cnt; i++) {
		uint32_t ret = 0;
		if(is_oob)
			ret = nand_write_page(h, nand_start, buf, (buf + h->page_size));
		else {
			ret = nand_write_page(h, nand_start, buf, NULL);
		}
		if (ret != h->page_size) {
			NAND_DEBUG("nand_write_page return %d\n", ret);
			return -1;
		}
		nand_start += h->page_size;
		buf += real_pagesize;
	}

	return size;
}

int mv_nand_write_block_generic_multipage(loff_t nand_start, const unsigned char* data_buf, int size, int is_oob)
{
	uint32_t page_cnt;
	blk_dev_nand_t *h = &instance;
	uint8_t *buf = (uint8_t *)data_buf;
	uint32_t real_pagesize = h->page_size;
	uint32_t pn_write = 0;

	if(is_oob)
		real_pagesize = h->page_size  + h->spare_bytes_per_page;

	if(nand_start % h->page_size != 0) {
		NAND_DEBUG("write start address should be aligned with page size\n");
		return 1;
	}
	if(size % real_pagesize != 0) {
		NAND_DEBUG("the size of data should be aligned with page size(+oob if oob is enabled)\n");
		return 1;
	}

	//if (nand_dev_setup(h))
	//	return 1;

	page_cnt = size / real_pagesize;

	for (; page_cnt > 0;) {
		pn_write = (page_cnt > MAX_RW_PAGE_CNT) ? MAX_RW_PAGE_CNT : page_cnt;
		nand_write_multipage(h, nand_start, buf, pn_write, is_oob);
		nand_start += (h->page_size * pn_write);
		data_buf += (real_pagesize * pn_write);
		page_cnt -= pn_write;
	}

	return size;
}

int mv_nand_write_block(loff_t nand_start, const unsigned char* data_buf, int size)
{
#ifdef NAND_WRITE_MULTIPAGE_MODE
	return mv_nand_write_block_generic_multipage(nand_start, data_buf, size, 0);
#else
	return mv_nand_write_block_generic(nand_start, data_buf, size, 0);
#endif
}

int mv_nand_write_block_withoob(loff_t nand_start, const unsigned char* data_buf, int size)
{
#ifdef NAND_WRITE_MULTIPAGE_MODE
	return mv_nand_write_block_generic_multipage(nand_start, data_buf, size, 1);
#else
	return mv_nand_write_block_generic(nand_start, data_buf, size, 1);
#endif
}

int mv_nand_erase_multiblk(loff_t ofs, unsigned int blk_cnt)
{
	uint32_t ret = 0;
	nfc_cmd_0_reg_t cmd0;
	nfc_cmd_1_reg_t cmd1;
	blk_dev_nand_t *h = &instance;
	int32_t row_addr = ofs / h->page_size;

	if(blk_cnt > MAX_ERASE_BLK_CNT) {
		NAND_DEBUG("Cannot erase more than %d blks.\n", MAX_ERASE_BLK_CNT);
		return 1;
	}

	cmd0.u[0] =                 0x0;
	cmd1.u[0] =                 0x0;
	cmd0.pio.erase.ct =         0x1;
	cmd0.pio.erase.trd_num =    0;
	cmd0.pio.erase.interrupt =  0;
	cmd0.pio.erase.cmd_type =   CT_BLOCK_ERASE(blk_cnt);
	cmd1.pio.erase.row_addr =   row_addr;

	ret = wait_until_thread_is_idle(h, 0);
	if (ret != 0)
		return -1;

	ret = program_command_registers(cmd0.u[0], cmd1.u[0], 0, 0);
	if (ret != 0)
		return -1;

	ret = check_cmd_ignored_flag(0);
	if (ret != 0)
		return -1;

	ret = check_operation_status_pio(h, 0, ERR_ALL);
	if (ret != 0)
		return -1;

	ret = wait_until_thread_is_idle(h, 0);
	if (ret != 0)
		return -1;

	ret = polling_rbn(h);
	if (ret != 0)
		return -1;

	return 0;
}

int mv_nand_erase(loff_t ofs)
{
	return mv_nand_erase_multiblk(ofs, 1);
}

static int32_t mark_bad_page(uint32_t page_addr)
{
	blk_dev_nand_t *h = &instance;
	uint8_t page[MAX_PAGE_SIZE + MAX_OOB_SIZE];

	assert(h->page_size + h->spare_bytes_per_page < MAX_PAGE_SIZE + MAX_OOB_SIZE);

	page[h->page_size] = 0x00;
	uint32_t ret = nand_write_page(h, page_addr, page, &page[h->page_size]);
	if (ret != h->page_size) {
		NAND_DEBUG("nand_write_page return %d\n", ret);
		return 1;
	}

	return 0;
}

int mv_nand_mark_badblock(loff_t ofs, __attribute__((unused)) int getchip)
{
	blk_dev_nand_t *h = &instance;
	uint32_t blk_addr = ofs / h->blk_size * h->blk_size;

	//if (nand_dev_setup(h))
	//	return 1;

	int32_t ret = mark_bad_page(blk_addr);
	if (ret != 0)
		return 1;

	ret = mark_bad_page(blk_addr + h->page_size);

	return ret;
}
