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

#ifndef _TZ_NW_BOOT_H_
#define _TZ_NW_BOOT_H_

#include "tz_boot_cmd.h"

/*
 * Bellow are for Bootloader
 */

//int tz_nw_verify_image(const void *src, unsigned long size, void *dst);
int tz_nw_verify_image(int num, ...);
int tz_nw_enter_boot_stage(int stage, int mode);

/** Get the memory region count
 *
 * @param attr_mask	attribute mask to filter memory regions.
 * 			can be the flags in MR_M_*. 0 to get all.
 * 			only (region.attr & attr_mask == attr_val) is filtered.
 * @param attr_val	attribute value to filter memory regions.
 * 			use MR_ATTR() to generate it. ignored if attr_mask=0.
 *
 * @return		matched region number.
 */
unsigned long tz_nw_get_mem_region_count(
		unsigned long attr_mask, unsigned long attr_val);

/** Retrieve the memory region list.
 *
 * @param region	buffer to store the retrieved regions.
 * @param max_mum	max count can be retrieved (count region).
 * @param attr_mask	attribute mask to filter memory regions.
 * 			can be the flags in MR_M_*. 0 to get all.
 * 			only (region.attr & attr_mask == attr_val) is filtered.
 * @param attrVal	attribute value to filter memory regions.
 * 			use MR_ATTR() to generate it. ignored if attr_mask=0.
 *
 * @return		retrieved region number. if max_num < total
 *			matched region number, then only max_mum would be copied,
 *			and return total matched region_number.
 */
unsigned long tz_nw_get_mem_region_list(
		void *region, unsigned long max_num,
		unsigned long attr_mask, unsigned long attr_val);

unsigned long tz_nw_rpmb_config_enable(void *fastcall_param, unsigned long parame_length);

/*
 * Bellow are for Linux
 */
int tz_nw_outer_cache_enable(void);
void tz_nw_outer_cache_disable(void);
void tz_nw_outer_cache_resume(void);

int tz_nw_cpu_boot(int cpu);
int tz_nw_cpu_idle(void);

#endif /* _TZ_NW_BOOT_H_ */
