/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright © 2013-2018 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
#include <common.h>
#include <asm/io.h>
#include <linux/types.h>
#include <malloc.h>
#include "SysMgr.h"
#include "fastboot_syna.h"
#include "genimg.h"

#define SOC_SM_SYS_CTRL_REG_BASE SOC_SM_MEMMAP_SMREG_BASE
#define SYNA_SM_MAX_FW_SIZE 0x20000

#define FIRMWARE_NAME "firmware"
#define	MAX_PAGESIZE 8192
#define SDIO_BLK_SIZE 512

#define IMAGE_TYPE_SM 0x34

int find_chunk(unsigned int id, struct img_header *buff)
{
	if (buff->chunk_num < MAX_IMAGE_CHUNK_ENTRY) {
		for (int i = 0; i < buff->chunk_num; i++) {
			if (id == buff->chunk[i].id)
				return i;
		}
	}

	return -1;
}

static int syna_suspend_sm(void)
{
	unsigned int reg = 0;

	reg = readl(SOC_SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_CPU_CTRL);
	reg &= ~(1 << LSb32smSysCtl_SM_CPU_CTRL_CPU_RST_GO);

	writel(reg, SOC_SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_CPU_CTRL);
	return 0;
}

static int syna_start_sm(void)
{
	unsigned int reg = 0;

	reg = readl(SOC_SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_CPU_CTRL);
	reg |= (1 << LSb32smSysCtl_SM_CPU_CTRL_CPU_RST_GO);

	writel(reg, SOC_SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_CPU_CTRL);
	return 0;
}

#ifdef CONFIG_SYNA_INCLUDE_SM_FW
#define SOC_ITCM_BASE 0xf7f80000

static int syna_load_sm(void)
{
	extern u8 __sm_fw_begin[];
	extern u8 __sm_fw_end[];
	u32 sm_fw_size = __sm_fw_end - __sm_fw_begin;

	if (sm_fw_size > SYNA_SM_MAX_FW_SIZE) {
		printf("ERROR: SM firmware image too big\n");
		return -1;
	}

	memcpy(SOC_ITCM_BASE, __sm_fw_begin, __sm_fw_end - __sm_fw_begin);

	return 0;
}
#else
static int syna_load_sm(void)
{
	int ret = -1, idx = -1;
	struct img_info *img_info;

	unsigned char *payload = NULL;
	unsigned char *header = NULL;
	struct img_header *img_hdr =  NULL;
	u32 img_size, img_offset, img_read_size;
	u32 img_read_offset = 0, aligned_offset = 0;

	payload = malloc_ion_noncacheable(SYNA_SM_MAX_FW_SIZE);
	if (!payload)
		goto out;

	header = malloc_ion_noncacheable(MAX_PAGESIZE);
	if (!header)
		goto out;

	fb_mmc_flash_read(FIRMWARE_NAME, header, MAX_PAGESIZE);
	img_info = (struct img_info *)header;
	if (img_info->magic != IMG_INFO_MAGIC) {
		printf("ERROR: incorrect magic in image info  0x%08x\n", img_info->magic);
		goto out;
	}

	img_hdr = (struct image_header *)(header + PREPEND_IMAGE_INFO_SIZE);

	/* Find out sm firmware chunk to get sm firmware image size and offset */
	idx = find_chunk(IMAGE_CHUNK_ID_SM_FW, img_hdr);
	if (idx >= 0) {
		img_offset = img_hdr->chunk[idx].offset;
		img_size = img_hdr->chunk[idx].size;
	} else {
		printf("ERROR: doesn't find SM firmware image.\n");
		goto out;
	}

	/* Add prepend subimage header size*/
	img_offset += PREPEND_IMAGE_INFO_SIZE;

	/* Align start offset and size to 512 bytes */
	img_read_offset = (img_offset / SDIO_BLK_SIZE) * SDIO_BLK_SIZE;
	img_read_size = (img_size + 2 * SDIO_BLK_SIZE - 1) / SDIO_BLK_SIZE * SDIO_BLK_SIZE;
	aligned_offset = img_offset - img_read_offset;

	/* Read Image to Non-Secure Buffer from flash*/
	fb_mmc_flash_read_from_offset(FIRMWARE_NAME, payload, img_read_size, img_read_offset);

	/* Verify Image, Skip aligned data */
	if (tz_nw_verify_image(5, payload + aligned_offset, img_size, payload, img_size, IMAGE_TYPE_SM) <= 0) {
		printf("ERROR: Verify SM firmware image failed\n");
		goto out;
	}

	ret = 0;

out:
	if (header)
		free_ion_noncacheable(header);

	if (payload)
		free_ion_noncacheable(payload);

	return ret;
}
#endif

int syna_init_sm(void)
{
	syna_suspend_sm();

	if (syna_load_sm()) {
		printf("ERROR: Failed to load SM firmware\n");
		return -1;
	}

	syna_start_sm();

	return 0;
}
