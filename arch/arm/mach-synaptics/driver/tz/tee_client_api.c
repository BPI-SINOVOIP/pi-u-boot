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
#include "smc.h"
#include "tz_comm.h"
#include "tee_comm.h"
#include "tee_mgr_cmd.h"
#include "tee_ca_mgr_cmd.h"
#include "tee_ca_sys_cmd.h"
#include "tee_client_api.h"
#include "mem_init.h"

#define TEE_PARAM_TYPES(t0, t1, t2, t3)	\
	((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))

#define TEE_COMM_PARAM_BASIC_SIZE	(52)

uint32_t TEEC_CallParamToComm(union tee_param *commParam,
			TEEC_Parameter *param, uint32_t paramType)
{
	uint32_t t = paramType & 0x7;	/* covented paramType for communication */

	switch (paramType) {
	case TEEC_VALUE_INPUT:
	case TEEC_VALUE_OUTPUT:
	case TEEC_VALUE_INOUT:
		commParam->value.a = param->value.a;
		commParam->value.b = param->value.b;
		break;
	case TEEC_MEMREF_WHOLE:
		t = TEE_PARAM_TYPE_MEMREF_INOUT;
		commParam->memref.buffer = (uint32_t)((unsigned long)
				param->memref.parent->phyAddr);
		commParam->memref.size   = param->memref.parent->size;
		break;
	case TEEC_MEMREF_PARTIAL_INPUT:
	case TEEC_MEMREF_PARTIAL_OUTPUT:
	case TEEC_MEMREF_PARTIAL_INOUT:
		commParam->memref.buffer = (uint32_t)((unsigned long)
				(param->memref.parent->phyAddr +
				param->memref.offset));
		commParam->memref.size   = param->memref.size;
		break;
	}

	return t;
}

/*
 * it will fill cmd->param_types and cmd->params[4]
 */
TEEC_Result TEEC_CallOperactionToCommand(struct tee_comm_param *cmd,
				uint32_t cmdId, TEEC_Operation *operation)
{
	int i, t[4];

	if (!cmd) {
		printf("illigle arguments\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);

	cmd->cmd_id = cmdId;

	if (!operation) {
		cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
		goto out;
	}

	/* travel all the 4 parameters */
	for (i = 0; i < 4; i++) {
		int paramType = TEEC_PARAM_TYPE_GET(operation->paramTypes, i);
		t[i] = TEEC_CallParamToComm(&cmd->params[i],
					&operation->params[i], paramType);
	}

	cmd->param_types = TEE_PARAM_TYPES(t[0], t[1], t[2], t[3]);

out:
	//printf("invoke command 0x%08x\n", cmd->cmd_id);
	return TEEC_SUCCESS;
}

uint32_t TEEC_CallCommToParam(TEEC_Parameter *param,
                        union tee_param *commParam,
                        uint32_t paramType)
{
        uint32_t t = paramType & 0x7;   /* covented paramType for communication */
        switch (paramType) {
        case TEEC_VALUE_INPUT:
        case TEEC_VALUE_OUTPUT:
        case TEEC_VALUE_INOUT:
                param->value.a = commParam->value.a;
                param->value.b = commParam->value.b;
                break;
        case TEEC_MEMREF_TEMP_OUTPUT:
        case TEEC_MEMREF_TEMP_INOUT:
                param->tmpref.size = commParam->memref.size;
                break;
        case TEEC_MEMREF_WHOLE:
        case TEEC_MEMREF_PARTIAL_OUTPUT:
        case TEEC_MEMREF_PARTIAL_INOUT:
                param->memref.size = commParam->memref.size;
                break;
        }

        return t;
}

/*
 * it will copy data from cmd->params[4] to operation->params[4]
 */
TEEC_Result TEEC_CallCommandToOperaction(TEEC_Operation *operation,
                                struct tee_comm_param *cmd)
{
        int i;

        if (!operation)
                return TEEC_SUCCESS;

        if (!cmd) {
                printf("illigle arguments\n");
                return TEEC_ERROR_BAD_PARAMETERS;
        }

        /* travel all the 4 param */
        for (i = 0; i < 4; i++) {
                int paramType = TEEC_PARAM_TYPE_GET(operation->paramTypes, i);
                TEEC_CallCommToParam(&operation->params[i],
                                        &cmd->params[i],
                                        paramType);
        }

        return TEEC_SUCCESS;
}

TEEC_Result register_ta(void * pta, size_t size)
{
	TEEC_Result result = 1;
	union tz_smc_return ret;
	int i = 0;
	uint32_t status = 1;

	TEEC_Operation operation = {0};
	TEEC_SharedMemory tmp;
	struct tee_comm_param * cmd;
	struct tee_comm * comm = NULL;
	int t[4];
	uint32_t param;

	//init tz memory before call this function
	comm = (struct tee_comm *)(uintptr_t)malloc_ion_cacheable(sizeof(struct tee_comm));

	if(!comm ) {
		printf("register_ta: no enough memory! %ld\n", sizeof(struct tee_comm));
		goto done;
	}

	comm->pa = (uint32_t)((unsigned long)comm);
	comm->va = (uint32_t)((unsigned long)comm);

	tmp.buffer = pta;
	tmp.phyAddr = pta;
	tmp.size = size;
	tmp.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	tmp.allocated = true;
	tmp.operationCount = 0;

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_MEMREF_PARTIAL_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].memref.parent = &tmp;
	operation.params[0].memref.size = size;
	operation.params[0].memref.offset = 0;
	operation.params[1].value.a = 4;

	cmd = &comm->call_param;
	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);

	cmd->cmd_id = TAMGR_CMD_REGISTER;

	for (i = 0; i < 4; i++) {
		int paramType = TEEC_PARAM_TYPE_GET(operation.paramTypes, i);
		t[i] = TEEC_CallParamToComm(&cmd->params[i],
				&operation.params[i], paramType);
	}

	cmd->param_types = TEE_PARAM_TYPES(t[0], t[1], t[2], t[3]);

	param = (uint32_t)((unsigned long)comm->pa) + offsetof(struct tee_comm, call_param);

	flush_dcache_all();
	invalidate_dcache_all();
	do {
		status = __smc_tee(TZ_CMD_TEE_SYS, TZ_TASK_ID_MGR, param, 0, (unsigned long *)&ret);
	} while(status != TZ_COMM_RSP_DONE);

	if ((status & TZ_COMM_RSP_DONE) && !ret.res.result) {
		result = TEEC_SUCCESS;
		printf("register_ta:tz comm rsp done! %x, %lx\n", status, ret.res.result);
	} else {
		result = TEEC_ERROR_GENERIC;
		printf("register_ta:tz comm rsp error! %x, %lx\n", status, ret.res.result);
	}

done:
	return result;
}

TEEC_Result TEEC_OpenSession (
		TEEC_Session*		session,
		const TEEC_UUID*	destination,
		uint32_t		connectionMethod,
		const void*		connectionData,
		TEEC_Operation*		operation,
		uint32_t*		returnOrigin)
{
	TEEC_Result res = TEEC_SUCCESS;
	uint32_t taskId = 0, sessionId = 0;

	if((session == NULL) || (destination == NULL)) {
		printf("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	memset(session, 0, sizeof(*session));

	session->comm = (struct tee_comm *)(uintptr_t)malloc_ion_cacheable(sizeof(struct tee_comm));

	session->comm->pa = (uint32_t)((unsigned long)session->comm);
        session->comm->va = (uint32_t)((unsigned long)session->comm);

	if (!session->comm){
		printf("fail to get comm channel\n");
		res = TEE_ERROR_COMMUNICATION;
		goto fail_out;
	}

	/* create the instance first */
	res = TAMgr_CreateInstance(session->comm, destination,
					returnOrigin, &taskId);

	if (res != TEEC_SUCCESS) {
		printf("fail to create instance, result=0x%08x\n", res);
		goto fail_out;
	}

	res = TASysCmd_OpenSession(session->comm, taskId,
					connectionMethod, connectionData,
					operation, returnOrigin, &sessionId);

	if (res != TEEC_SUCCESS) {
		printf("fail to open session, result=0x%08x\n", res);
		goto fail_out;
	}

	session->operationCount = 0;
	session->taskId = taskId;
	session->sessionId = sessionId;
	session->serviceId = *destination;

	return res;

fail_out:
	if (taskId > 0)
		TAMgr_DestroyInstance(session->comm, taskId, NULL);

	/* reset the session context */
	memset(session, 0, sizeof(*session));

	return res;
}

void TEEC_CloseSession (
		TEEC_Session*		session)
{
	TEEC_Result res;
	uint32_t origin;
	bool instanceDead = false;

	if(!session || !session->comm ||
		0 == session->taskId || 0 == session->sessionId) {
		printf("invalid session handle\n");
		return;
	}

	if(session->operationCount > 0) {
		printf("%d Pending operations\n",
				session->operationCount);
		return;
	}

	res = TASysCmd_CloseSession(session->comm,
				session->taskId, session->sessionId,
				&origin, &instanceDead);

	if (res != TEEC_SUCCESS) {
		printf("fail to close session, result=0x%08x\n", res);
		return;
	}

	/* destroy instance is not necessary, TEE framework will do it
	 * automatically.
	 */
	if (instanceDead)
		TAMgr_DestroyInstance(session->comm,
					session->taskId, NULL);

	/* reset the session context */
	memset(session, 0, sizeof(*session));
}


TEEC_Result TEEC_InvokeCommand(
		TEEC_Session*	session,
		uint32_t	commandID,
		TEEC_Operation*	operation)
{
	TEEC_Result res = TEEC_SUCCESS;
	struct tee_comm_param *comm;
	uint32_t param;
	union tz_smc_return ret;
	uint32_t status = 1;

	if(!session || !session->comm || 0 == session->taskId || 0 == session->sessionId) {
		printf("invalid session handle\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (operation)
		operation->started = true;

	session->operationCount++;

	comm = &session->comm->call_param;
	/* pack the command */
	TEEC_CallOperactionToCommand(comm,
			commandID, operation);

	comm->session_id = session->sessionId;

	if (operation)
		operation->session = session;

	param = (uint32_t)((unsigned long)session->comm->pa) + offsetof(struct tee_comm, call_param);

	flush_dcache_all();
	invalidate_dcache_all();
	do {
		status = __smc_tee(TZ_CMD_TEE_USER, session->taskId, param, 0, (unsigned long *)&ret);
	} while(status != TZ_COMM_RSP_DONE);

	if ((status & TZ_COMM_RSP_DONE) && !ret.res.result) {
		res = TEEC_SUCCESS;
		//printf("TEEC_InvokeCommand:tz comm rsp done! %x, %x\n", status, ret.res.result);
		TEEC_CallCommandToOperaction(operation, &session->comm->call_param);
	} else {
		res = TEEC_ERROR_GENERIC;
		printf("TEEC_InvokeCommand)(%d/%x):tz comm rsp error! %x, %lx\n", commandID, commandID, status, ret.res.result);
	}

	session->operationCount--;

	return res;
}
