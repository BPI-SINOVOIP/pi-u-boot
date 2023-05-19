/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Jie Zhang <Jie.Zhang@synaptics.com>
 */

#include <common.h>


uint64_t call_bm_process(uint64_t func_id, uint64_t arg1, uint64_t arg2,
		uint64_t arg3, uint64_t arg4, uint64_t arg5,
		uint64_t arg6)
{
	register uint64_t l_x0 asm("x0") = func_id;
	l_x0 = l_x0;
	register uint64_t l_x1 asm("x1") = arg1;
	l_x1 = l_x1;
	register uint64_t l_x2 asm("x2") = arg2;
	l_x2 = l_x2;
	register uint64_t l_x3 asm("x3") = arg3;
	l_x3 = l_x3;
	register uint64_t l_x4 asm("x4") = arg4;
	l_x4 = l_x4;
	register uint64_t l_x5 asm("x5") = arg5;
	l_x5 = l_x5;
	register uint64_t l_x6 asm("x6") = arg6;
	l_x6 = l_x6;

	asm volatile ("smc #0\n");
	register uint64_t ret asm("x0");
	return ret;
}
