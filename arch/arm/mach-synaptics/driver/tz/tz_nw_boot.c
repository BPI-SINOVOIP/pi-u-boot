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

#include <stdarg.h>
#include "smc.h"
#include "tz_boot_cmd.h"
#include "tz_nw_boot.h"

//the first parameter is the number of other parameters
int tz_nw_verify_image(int num, ...)
{
	unsigned long param[6], result[4];
	int i = 0, argnum = 0;
	va_list params;

	argnum = (num > 6) ? 6 : num;

	va_start(params, num);
	for(i = 0; i < argnum; i++) {
		param[i] = va_arg(params, unsigned long);
	}
	va_end(params);

	__smc6(SMC_FUNC_TOS_CRYPTO_VERIFY_IMAGE, param, result);

	if (result[0] == 0)
		return result[1];
	else
		return result[0];
}
#if 0
int tz_nw_verify_image(const void *src, unsigned long size, void *dst)
{
	unsigned long param[6], result[4];
	param[0] = (unsigned long)src;
	param[1] = size;
	param[2] = (unsigned long)dst;
	param[3] = size;

	__smc6(SMC_FUNC_TOS_CRYPTO_VERIFY_IMAGE, param, result);

	if (result[0] == 0)
		return result[1];
	else
		return 0;
}
#endif

unsigned int tz_nw_usb_console_put_char(char c)
{
	return __smc(SMC_FUNC_SIP_USB_CONSOLE_PUT_CHAR, c, 0, 0);
}

unsigned int tz_nw_usb_load_image(unsigned int type, char *name, unsigned int name_len, unsigned int address)
{
	unsigned long param[6];
	param[0] = type;
	param[1] = (unsigned long)name;
	param[2] = name_len;
	param[3] = address;
	return __smc6(SMC_FUNC_SIP_USB_LOAD_IMAGE, param, 0);
}

int tz_nw_enter_boot_stage(int stage, int mode)
{
	return __smc(SMC_FUNC_TOS_BOOT_STAGE, stage, mode, 0);
}

unsigned long tz_nw_get_mem_region_count(
		unsigned long attr_mask, unsigned long attr_val)
{
	return __smc(SMC_FUNC_TOS_MEM_REGION_COUNT,
			attr_mask, attr_val, 0);
}

unsigned long tz_nw_get_mem_region_list(void *region, unsigned long max_num,
			unsigned long attr_mask, unsigned long attr_val)
{
	unsigned long param[6];
	param[0] = (unsigned long)region;
	param[1] = max_num;
	param[2] = attr_mask;
	param[3] = attr_val;
	return __smc6(SMC_FUNC_TOS_MEM_REGION_LIST, param, 0);
}

unsigned long tz_nw_rpmb_config_enable(void *fastcall_param, unsigned long param_length)
{
	return __smc(TOS_BOOT(TOS_BOOT_GENERAL_FASTCALL), fastcall_param, param_length, 0);
}

unsigned long tz_nw_rpmb_ops(void *fastcall_param, unsigned long param_length)
{
	return __smc(TOS_BOOT(TOS_BOOT_GENERAL_FASTCALL), fastcall_param, param_length, 0);
}

int tz_nw_outer_cache_enable(void)
{
	return __smc(SMC_FUNC_TOS_OUTER_CACHE_ENABLE, 0, 0, 0);
}

void tz_nw_outer_cache_disable(void)
{
	__smc(SMC_FUNC_TOS_OUTER_CACHE_DISABLE, 0, 0, 0);
}

void tz_nw_outer_cache_resume(void)
{
	__smc(SMC_FUNC_TOS_OUTER_CACHE_RESUME, 0, 0, 0);
}

int tz_nw_cpu_idle(void)
{
	return __smc(SMC_FUNC_PSCI_CPU_SUSPEND, 0, 0, 0);
}

int tz_nw_cpu_boot(int cpu)
{
	return __smc(SMC_FUNC_PSCI_CPU_ON, cpu, 0, 0);
}
