/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Jie Zhang <Jie.Zhang@synaptics.com>
 *
 */

#ifndef _BM_GENERIC_H_
#define _BM_GENERIC_H_

#define SMC_FUNC_ID_MASK_NUMBER     (0x0000ffff)
#define SMC_FUNC_ID_STD             (0x04000000)
#define SMC_FUNC_ID_UNKNOWN         (0xffffffff)
#define BM_STDCALL(number)          (SMC_FUNC_ID_STD | ((number) & SMC_FUNC_ID_MASK_NUMBER))
#define BM_CMD_LOAD_USB_IMAGE        9
#define BM_CMD_TURN_OFF_USB_DEVICE   10

extern uint64_t call_bm_process(uint64_t func_id, uint64_t arg1, uint64_t arg2,
		uint64_t arg3, uint64_t arg4, uint64_t arg5,
		uint64_t arg6);

#endif