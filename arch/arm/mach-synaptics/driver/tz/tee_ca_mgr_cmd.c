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
#include "smc.h"
#include "tee_common.h"
#include "tee_client_type.h"
#include "tee_client_const.h"
#include "tee_ca_mgr_cmd.h"
#include "tee_mgr_cmd.h"
#include "tz_comm.h"
#include "tee_comm.h"

TEE_Result TAMgr_CreateInstance(
	struct tee_comm*	comm,
	const TEE_UUID*		destination,
	uint32_t*		returnOrigin,
	uint32_t*		taskId)
{
	struct tee_comm_param *cmd;
	TAMgrCmdCreateInstanceParamExt *p;
	uint32_t param;
	union tz_smc_return ret;
	uint32_t status = 1;

	cmd = &comm->call_param;
	p = (void *)cmd->param_ext;
	param = (uint32_t)((unsigned long)comm->pa) +
			offsetof(struct tee_comm, call_param);

	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TAMGR_CMD_CREATE_INSTANCE;
	cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	cmd->param_ext_size = sizeof(TAMgrCmdCreateInstanceParamExt);
	memcpy(&p->destination, destination, sizeof(p->destination));

	flush_dcache_all();
	invalidate_dcache_all();
	do {
		/* invoke command */
		status = __smc_tee(TZ_CMD_TEE_SYS, TZ_TASK_ID_MGR, param, 0, (unsigned long *)&ret);
	} while(status != TZ_COMM_RSP_DONE);

	if ((status & TZ_COMM_RSP_DONE) && !ret.res.result) {
		*taskId = p->taskId;
		printf("TAMgr_CreateInstance:tz comm rsp done! %x, %lx, %x\n", status, ret.res.result, p->taskId);
	} else {
		printf("TAMgr_CreateInstance:tz comm rsp error! %x, %lx\n", status, ret.res.result);
	}

	return ret.res.result;
}

TEE_Result TAMgr_DestroyInstance(
	struct tee_comm*	comm,
	uint32_t		taskId,
	uint32_t*		returnOrigin)
{
	struct tee_comm_param *cmd;
	uint32_t param;
	union tz_smc_return ret;
	uint32_t status = 1;

	cmd = &comm->call_param;
	param = (uint32_t)((unsigned long)comm->pa) +
			offsetof(struct tee_comm, call_param);
	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TAMGR_CMD_DESTROY_INSTANCE;
	cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	cmd->params[0].value.a = taskId;
	flush_dcache_all();
	invalidate_dcache_all();
	do {
		/* invoke command */
		status = __smc_tee(TZ_CMD_TEE_SYS, TZ_TASK_ID_MGR, param, 0, (unsigned long *)&ret);
	} while(status != TZ_COMM_RSP_DONE);

	if ((status & TZ_COMM_RSP_DONE) && !ret.res.result) {
		printf("TAMgr_DestroyInstance:tz comm rsp done! %x, %lx\n", status, ret.res.result);
	} else {
		printf("TAMgr_DestroyInstance:tz comm rsp error! %x, %lx\n", status, ret.res.result);
	}

	return ret.res.result;
}
