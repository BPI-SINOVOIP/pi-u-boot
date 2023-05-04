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

#ifndef _TEE_MGR_CMD_H_
#define _TEE_MGR_CMD_H_

#define TAMGR_UUID	{0x9ef9656c, 0x8f49, 0x11e3,	\
			{0x90, 0x1c, 0x78, 0x2b, 0xcb, 0x5c, 0xf3, 0xe3}}

enum TAMgrCmd {
	TAMGR_CMD_SUSPEND,
	TAMGR_CMD_RESUME,
	TAMGR_CMD_REGISTER,
	/* Create Instance
	 * input: destination UUID
	 * output: created taskId
	 */
	TAMGR_CMD_CREATE_INSTANCE,
	/* Destroy Instance
	 * input: taskId to destroy in params[0].value.a
	 * note: should NOT need call it explictly
	 */
	TAMGR_CMD_DESTROY_INSTANCE,
	TAMGR_CMD_MAX
};

typedef struct {
	TEE_UUID destination;	/* input: desitination TA uuid */
	uint32_t taskId;	/* output: created taskId */
} TAMgrCmdCreateInstanceParamExt;

#endif /* _TEE_MGR_CMD_H_ */
