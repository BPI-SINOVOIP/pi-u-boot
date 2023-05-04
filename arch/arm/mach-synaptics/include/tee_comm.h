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

#ifndef _TZ_CMD_TEE_H_
#define _TZ_CMD_TEE_H_

#include "tee_internal_core_common.h"

#define tee_param	TEE_Param

/*
 * in order to make it simple, and sync with TEE,
 * we use fixed size command for communication.
 * each channel is 4kB, half for call in, and the other half for callback
 * and it's readable/writable in nw.
 * here, shm is temporary shared memory between nw/sw, which
 * is used for some light communications.
 *
 */
#define TEE_COMM_CHANNEL_SIZE		(2048)
#define TEE_COMM_HEADER_SIZE		(32)
#define TEE_COMM_PARAM_SIZE		\
	((TEE_COMM_CHANNEL_SIZE - TEE_COMM_HEADER_SIZE) / 2)
/* the basic parameter doesn't include the param_ext */
#define TEE_COMM_PARAM_BASIC_SIZE	(52)
#define TEE_COMM_PARAM_EXT_SIZE		\
	(TEE_COMM_PARAM_SIZE - TEE_COMM_PARAM_BASIC_SIZE)

//#define offsetof(TYPE, MEMBER)		((size_t)&(((TYPE *)0)->MEMBER))

struct tee_comm_param {
	uint32_t session_id;
	uint32_t cmd_id;
	uint32_t flags;
	uint32_t param_types;
	union tee_param params[4];
	uint32_t param_ext_size;
	uint8_t param_ext[TEE_COMM_PARAM_EXT_SIZE];
};

struct tee_comm {
	uint32_t	used;
	/* FIXME: to support 32-bit user-land APP and
	 * 32-bit EL1 TZ kernel, change void * to uint32_t,
	 * needs to refine later if to support 64-bit EL1
	 * TZ kernel and 64-bit user-land app.
	 */
	uint32_t	pa;
	uint32_t	va;
	uint8_t		reserved[TEE_COMM_HEADER_SIZE -
				3 * sizeof(uint32_t)];
	struct tee_comm_param	call_param;
	struct tee_comm_param	callback_param;
};

#if 0
typedef int (*tee_cmd_handler_user)(void *userdata,
		uint32_t cmd_id,
		uint32_t param_types,
		union tee_param params[4]);

typedef int (*tee_cmd_handler_sys)(void *userdata,
		uint32_t cmd_id,
		uint32_t param_types,
		union tee_param params[4],
		void *param_ext,
		uint32_t param_ext_size);
#endif

#endif /* _TZ_CMD_TEE_H_ */
