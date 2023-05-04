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
#include "tee_sys_cmd.h"
#include "tee_ca_sys_cmd.h"
#include "tz_comm.h"
#include "tee_comm.h"

extern TEEC_Result TEEC_CallOperactionToCommand(struct tee_comm_param *cmd,
		uint32_t cmdId, TEEC_Operation *operation);
/*
 * shm/param_ext: TAMgrCmdOpenSessionParam
 * params[0-3]: params.
 */
TEEC_Result TASysCmd_OpenSession(
	struct tee_comm*	comm,
	uint32_t		taskId,
	uint32_t		connectionMethod,
	const void*		connectionData,
	TEEC_Operation*		operation,
	uint32_t*		returnOrigin,
	uint32_t*		sessionId)
{
	TEEC_Result result = TEEC_SUCCESS;
	struct tee_comm_param *cmd;
	uint32_t param, group = 0;
	TASysCmdOpenSessionParamExt *p;
	union tz_smc_return ret;
	uint32_t status = 1;

	if (!comm || !taskId) {
		printf("invalid comm(0x%08x) or taskId(0x%08x)\n",
				(uint32_t)(uintptr_t)comm, taskId);
		result = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	cmd = &comm->call_param;
	p = (TASysCmdOpenSessionParamExt *)cmd->param_ext;
	param = (uint32_t)((unsigned long)comm->pa) +
			offsetof(struct tee_comm, call_param);

	switch(connectionMethod) {
	case TEEC_LOGIN_PUBLIC:
	case TEEC_LOGIN_USER:
	case TEEC_LOGIN_APPLICATION:
	case TEEC_LOGIN_USER_APPLICATION:
		break;

	case TEEC_LOGIN_GROUP:
	case TEEC_LOGIN_GROUP_APPLICATION:
		if(connectionData == NULL) {
			printf("connection method requires valid group\n");
			result = TEEC_ERROR_BAD_PARAMETERS;
			goto out;
		} else {
			group = *(uint32_t *)connectionData;
		}
		break;
	default:
		printf("invalid connectionMethod (0x%08x)\n", connectionMethod);
		result = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	/* pack the command */
	TEEC_CallOperactionToCommand(cmd, TASYS_CMD_OPEN_SESSION, operation);
	cmd->param_ext_size = sizeof(TASysCmdOpenSessionParamExt);
	/* for NW, it doesn't have a UUID */
	memset(&p->client, 0, sizeof(p->client));
	p->login = connectionMethod;
	p->group = group;

	printf("invoke command 0x%08x\n", cmd->cmd_id);

	flush_dcache_all();
	invalidate_dcache_all();
	do {
		/* invoke command */
		status = __smc_tee(TZ_CMD_TEE_SYS, taskId, param, 0, (unsigned long *)&ret);
	} while(status != TZ_COMM_RSP_DONE);

	if ((status & TZ_COMM_RSP_DONE) && !ret.res.result) {
		result = TEEC_SUCCESS;
		*sessionId = p->sessionId;
		printf("TASysCmd_OpenSession:tz comm rsp done! %x, %lx, %x\n", status, ret.res.result, p->sessionId);
	} else {
		result = TEEC_ERROR_GENERIC;
		printf("TASysCmd_OpenSession:tz comm rsp error! %x, %lx\n", status, ret.res.result);
	}

out:
	return result;
}

/*
 * params[0]: value.a = sessionId.
 */
TEEC_Result TASysCmd_CloseSession(
	struct tee_comm*	comm,
	uint32_t		taskId,
	uint32_t		sessionId,
	uint32_t*		returnOrigin,
	bool*			instanceDead)
{

	TEEC_Result res = TEEC_SUCCESS;
	struct tee_comm_param *cmd;
	uint32_t param;
	union tz_smc_return ret;
	uint32_t status = 1;

	cmd = &comm->call_param;
	param = (uint32_t)((unsigned long)comm->pa) +
			offsetof(struct tee_comm, call_param);

	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TASYS_CMD_CLOSE_SESSION;
	cmd->session_id = sessionId;
	cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	flush_dcache_all();
	invalidate_dcache_all();
	do {
		/* invoke command */
		status = __smc_tee(TZ_CMD_TEE_SYS, taskId, param, 0, (unsigned long *)&ret);
	} while(status != TZ_COMM_RSP_DONE);

	if ((status & TZ_COMM_RSP_DONE) && !ret.res.result) {
		res = TEEC_SUCCESS;
		*instanceDead = cmd->params[0].value.a;
		printf("TASysCmd_CloseSession:tz comm rsp done! %x, %lx\n", status, ret.res.result);
	} else {
		res = TEEC_ERROR_GENERIC;
		printf("TASysCmd_CloseSession:tz comm rsp error! %x, %lx\n", status, ret.res.result);
	}

	return res;
}
