/*
 * Copyright (c) 2013-2014 Marvell International Ltd. and its affiliates.
 * All rights reserved.
 *
 * This software file (the "File") is owned and distributed by Marvell
 * International Ltd. and/or its affiliates ("Marvell") under the following
 * licensing terms.
 *
 * Marvell Commercial License Option
 * ------------------------------------
 * If you received this File from Marvell and you have entered into a
 * commercial license agreement (a "Commercial License") with Marvell, the
 * File is licensed to you under the terms of the applicable Commercial
 * License.
 *
 * Marvell GPL License Option
 * ------------------------------------
 * If you received this File from Marvell, you may opt to use, redistribute
 * and/or modify this File in accordance with the terms and conditions of the
 * General Public License Version 2, June 1991 (the "GPL License"), a copy of
 * which is available along with the File in the license.txt file or by writing
 * to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE
 * EXPRESSLY DISCLAIMED. The GPL License provides additional details about this
 * warranty disclaimer.
 */

	.text

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
	.arch_extension sec
#endif

/*
 * Note: a DSB is required before call smc. It's to avoid any "pending"
 *       imprecise abort to bother the other world.
 *       It's not to ensure the memory access completed, which is already
 *       guanteed by hardware.
 */

	.globl __smc_cmd2
__smc_cmd2:
	push	{r3}

	dsb
	smc	#0

	pop	{ip}
	cmp	ip, #0
	stmne	ip!, {r1-r2}

	bx	lr

	.globl __smc_cmd3
__smc_cmd3:
	dsb
	smc	#0

	ldr	ip, [sp]
	cmp	ip, #0
	stmne	ip!, {r1-r3}

	bx	lr

	.globl __smc3
__smc3:
	dsb
	smc	#0

	ldr	ip, [sp]
	cmp	ip, #0
	stmne	ip!, {r0-r3}

	bx	lr

	.globl __smc6
__smc6:
	mov	ip, r2
	push	{r4-r6, ip}

	/* load parameters to registers */
	mov	ip, r1
	ldm	ip!, {r1-r6}

	dsb
	smc	#0

	pop	{r4-r6, ip}
	cmp	ip, #0
	stmne	ip!, {r0-r3}

	bx	lr

	.globl __smc
__smc:
	dsb
	smc	#0
	bx	lr
