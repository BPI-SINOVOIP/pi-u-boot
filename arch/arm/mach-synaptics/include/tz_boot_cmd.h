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

#ifndef _TZ_BOOT_CMD_H_
#define _TZ_BOOT_CMD_H_

#include "smc.h"

enum tz_boot_func_id {
	/* param: none;
	 * return: version number */
	SMC_FUNC_TOS_BOOT_VERSION		= TOS_BOOT(0x00),
	/* param: {stage, mode};
	 * return: error code */
	SMC_FUNC_TOS_BOOT_STAGE			= TOS_BOOT(0x10),
	/* param: {attr_mask, attr_val};
	 * return: count */
	SMC_FUNC_TOS_MEM_REGION_COUNT		= TOS_BOOT(0x20),
	/* param: {attr_mask, attr_val, region, max_num};
	 * return: count */
	SMC_FUNC_TOS_MEM_REGION_LIST,
	/* param: none;
	 * return: error code */
	SMC_FUNC_TOS_OUTER_CACHE_ENABLE		= TOS_BOOT(0x30),
	/* param: none;
	 * return: error code */
	SMC_FUNC_TOS_OUTER_CACHE_DISABLE,
	/* param: none;
	 * return: error code */
	SMC_FUNC_TOS_OUTER_CACHE_RESUME,
	/* param: {src, src_len, dst, dst_len};
	 * return: {errcode,dec_len} */
	SMC_FUNC_TOS_CRYPTO_VERIFY_IMAGE	= TOS_BOOT(0x40),

	SMC_FUNC_SIP_USB_CONSOLE_PUT_CHAR  = SIP_FASTCALL(0x10),

	SMC_FUNC_SIP_USB_LOAD_IMAGE        = SIP_FASTCALL(0x20),
};

enum tz_boot_stage {
	TZ_BOOT_STAGE_ROMCODE,
	TZ_BOOT_STAGE_SYSINIT,
	TZ_BOOT_STAGE_TRUSTZONE,
	TZ_BOOT_STAGE_BOOTLOADER,
	TZ_BOOT_STAGE_LINUX,
	TZ_BOOT_STAGE_ANDROID
};

enum tz_boot_mode {
	TZ_BOOT_MODE_NORMAL,
	TZ_BOOT_MODE_RECOVERY
};

#endif /* _TZ_BOOT_CMD_H_ */
