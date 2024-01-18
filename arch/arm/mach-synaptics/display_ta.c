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
#include <linux/types.h>
#include <malloc.h>
#include <string.h>
#include "genimg.h"
#include "fastboot_syna.h"
#include "tz_comm.h"
#include "tee_comm.h"
#include "tee_client_api.h"
#include "mem_region.h"
#include "tz_nw_boot.h"
#include "mem_region_userdata.h"
#include "mem_init.h"

//#define DEBUG


#define MAX_PAGESIZE    8192

#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
	((unsigned int)(char)(ch0) | ((unsigned int)(char)(ch1) << 8) | \
	((unsigned int)(char)(ch2) << 16) | ((unsigned int)(char)(ch3) << 24))

#define IMAGE_CHUNK_ID_FASTLOGO		MAKE_FOURCC('L', 'O', 'G', 'O')
#define IMAGE_CHUNK_ID_DHUB			MAKE_FOURCC('D', 'H', 'U', 'B')

#define IMAGE_HEADER_MAGIC			MAKE_FOURCC('I', 'M', '*', 'H')

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_AB)
#define BL_A_NAME			"bl_a"
#define BL_B_NAME			"bl_b"
#else
#define BL_N_NAME			"bl_normal"
#define BL_R_NAME			"bl_recovery"
#endif

static unsigned char * buf = NULL;
static unsigned char * logotabuf = NULL;
static unsigned int logotasize = 0;
static unsigned char * dhubtabuf = NULL;
static unsigned int dhubtasize = 0;
static struct img_header * ta_header = NULL;

static inline unsigned get_aligned(unsigned address, unsigned page_size) {
	return (address + page_size - 1) / page_size * page_size;
}

int load_ta(void)
{
	int i, to_read = 0;
	struct img_info * img_info;
	struct chunk_param * chunk;
	unsigned int ta_offset = 0, ta_size = 0;
	unsigned char * p = NULL;
	const char * partition = NULL;

#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_AB)
	if(get_current_slot() == BOOTSEL_A)
		partition = BL_A_NAME;
	else
		partition = BL_B_NAME;
#else
	partition = BL_N_NAME;
#endif

	//FIXME: find bl partition

	// read prepend image info to find the offset of preload_ta
	to_read = MAX_PAGESIZE;
	buf = malloc_ion_cacheable(to_read);
	if(buf == NULL) {
		printf("load_ta: no enough memory %d\n", to_read);
		goto nullmem;
	}
	fb_mmc_flash_read(partition, buf, to_read);
	img_info = (struct img_info *)buf;
	if(img_info->magic != IMG_INFO_MAGIC) {
		printf("ERROR: incorrect magic in image info  0x%08x\n", img_info->magic);
		goto failure;
	}

	/* find out the start offset of preload tas */
	ta_offset = img_info->image_offset+ img_info->image_size;
	ta_offset = get_aligned(ta_offset, 512);

	buf = malloc_ion_cacheable(MAX_PAGESIZE);
	if(buf == NULL) {
		printf("load_ta: no enough memory %d\n", to_read);
		goto nullmem;
	}
	fb_mmc_flash_read_from_offset(partition, buf, to_read, ta_offset);
	ta_header = (struct img_header *)(buf);

	if(ta_header->header_magic_num != IMAGE_HEADER_MAGIC) {
		printf("invalid TA header_magic_num\n");
		goto failure;
	}

	ta_size = ta_header->chunk[ta_header->chunk_num - 1].offset + ta_header->chunk[ta_header->chunk_num - 1].size;

	buf = malloc_ion_cacheable(ta_size);
	if(buf == NULL) {
		printf("load_ta: no enough memory %d\n", ta_size);
		goto nullmem;
	}
	fb_mmc_flash_read_from_offset(partition, buf, ta_size, ta_offset);

	for(i = 0; i < ta_header->chunk_num; i++) {
		chunk = &(ta_header->chunk[i]);
		//only load fastlogo ta
		if(IMAGE_CHUNK_ID_FASTLOGO == chunk->id) {
			printf("####find ta LOGO  %x\n", chunk->id);

			logotasize = chunk->size;
			logotabuf = buf + chunk->offset;

		} else if(IMAGE_CHUNK_ID_DHUB == chunk->id) {

			printf("####find ta DHUB  %x\n", chunk->id);
			dhubtabuf = chunk->size;
			dhubtabuf = buf + chunk->offset;
		}
	}

	return 0;
failure:

nullmem:
	return -1;
}

#define TEEC_BUFF_SIZE (1 << 20)
int reg_preload_tas(void)
{
	TEEC_Result result = TEEC_SUCCESS;

	get_mem_region_list_from_tz();

	if(load_ta()) {
		printf("failed to load ta\n");
		return -1;
	}

	if(logotasize && logotabuf){
		result = register_ta(logotabuf, logotasize);
		if (result != TEEC_SUCCESS) {
			printf("register LOGO TA failed, ret = 0x%08x\n", result);
			return -2;
		}
		printf("register LOGO TA success, ret = 0x%08x\n", result);
	}

	if(dhubtasize && dhubtabuf){
		result = register_ta(dhubtabuf, dhubtasize);
		if (result != TEEC_SUCCESS) {
			printf("register DHUB TA failed, ret = 0x%08x\n", result);
			return -2;
		}
		printf("register DHUB TA success, ret = 0x%08x\n", result);
	}

	return 0;
}
