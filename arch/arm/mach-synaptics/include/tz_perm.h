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
#ifndef _TZ_PERM_H_
#define _TZ_PERM_H_

/*
 * slave
 */
/* trustzone access permission bits */
#define TZ_NRW		0x0	/* non-secure read/write	*/
#define TZ_NNW		0x1	/* non-secure no-write		*/
#define TZ_NNR		0x2	/* non-secure no-read		*/
#define TZ_NWO		TZ_NNR
#define TZ_NRO		TZ_NNW
#define TZ_NNA		(TZ_NNW | TZ_NNR)

#define TZ_SRW		0x0	/* secure read/write		*/
#define TZ_SNW		0x4	/* secure no-write		*/
#define TZ_SNR		0x8	/* secure no-read		*/
#define TZ_SWO		TZ_SNR
#define TZ_SRO		TZ_SNW
#define TZ_SNA		(TZ_SNW | TZ_SNR)

/* trustzone access permission */
#define TZ_SRW_NRW	(TZ_SRW | TZ_NRW)
#define TZ_SRW_NRO	(TZ_SRW | TZ_NRO)
#define TZ_SRW_NWO	(TZ_SRW | TZ_NWO)
#define TZ_SRW_NNA	(TZ_SRW | TZ_NNA)
#define TZ_SRO_NNA	(TZ_SRO | TZ_NNA)
#define TZ_SWO_NNA	(TZ_SWO | TZ_NNA)
#define TZ_SNA_NNA	(TZ_SNA | TZ_NNA)

/* trustzone access permission bit mask */
#define TZ_M_NW		TZ_NNW	/* non-secure write mask	*/
#define TZ_M_NR		TZ_NNR	/* non-secure read mask		*/
#define TZ_M_NRW	(TZ_M_NW | TZ_M_NR)
#define TZ_M_SW		TZ_SNW	/* secure write mask		*/
#define TZ_M_SR		TZ_SNR	/* secure read mask		*/
#define TZ_M_SRW	(TZ_M_SW | TZ_M_SR)

/* trustzone access permission check */
#define TZ_IS_NNR(perm)	(((perm) & TZ_M_NR) == TZ_NNR)
#define TZ_IS_NR(perm)	(!TZ_IS_NNR(perm))
#define TZ_IS_NNW(perm)	(((perm) & TZ_M_NW) == TZ_NNW)
#define TZ_IS_NW(perm)	(!TZ_IS_NNW(perm))
#define TZ_IS_NNA(perm)	(((perm) & TZ_M_NRW) == TZ_NNA)
#define TZ_IS_NRW(perm)	(!TZ_IS_NNA(perm))

#define TZ_IS_SNR(perm)	(((perm) & TZ_M_SR) == TZ_SNR)
#define TZ_IS_SR(perm)	(!TZ_IS_SNR(perm))
#define TZ_IS_SNW(perm)	(((perm) & TZ_M_SW) == TZ_SNW)
#define TZ_IS_SW(perm)	(!TZ_IS_SNW(perm))
#define TZ_IS_SNA(perm)	(((perm) & TZ_M_SRW) == TZ_SNA)
#define TZ_IS_SRW(perm)	(!TZ_IS_SNA(perm))

/*
 * master
 */
/* master permissions */
#define TZM_WD		0x0	/* write by default permission	*/
#define TZM_WS		0x1	/* write as secure		*/
#define TZM_WN		0x3	/* write as non-secure		*/
#define TZM_WR		0x10	/* write as restricted		*/
#define TZM_RD		0x0	/* read by default permission	*/
#define TZM_RS		0x4	/* read as secure		*/
#define TZM_RN		0xc	/* read as non-secure		*/
#define TZM_RR		0x20	/* read as restricted		*/

#define TZM_D		(TZM_WD | TZM_RD)
#define TZM_S		(TZM_WS | TZM_RS)
#define TZM_N		(TZM_WN | TZM_RN)
#define TZM_R		(TZM_WR | TZM_RR)

#define TZM_NA		-1	/* no access			*/

/* master permission masks */
#define TZM_M_WO	0x1	/* write override bit		*/
#define TZM_M_WN	0x2	/* write non-secure bit		*/
#define TZM_M_WR	0x10	/* write restricted bit		*/
#define TZM_M_W		0x13	/* write mask bits		*/
#define TZM_M_RO	0x4	/* write override bit		*/
#define TZM_M_RN	0x8	/* write non-secure bit		*/
#define TZM_M_RR	0x20	/* read restricted bit		*/
#define TZM_M_R		0x2c	/* read mask bits		*/

#endif	/* _TZ_PERM_H_ */
