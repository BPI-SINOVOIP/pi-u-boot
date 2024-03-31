/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright � 2013-2018 Synaptics Incorporated. All rights reserved.
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
#include "global.h"
#include "SysMgr.h"
#include "Galois_memmap.h"
#include "watchdog_def.h"
#include "pinmux.h"
#include <asm/io.h>
#include <stdio.h>

#define		BITS_PER_PIN			3
#define     PINMUX_VALUE_MAX        ((1 << BITS_PER_PIN) -1)
#define		PINS_PER_REG			10
#define		PINMUX_CTRL_REG_NUM		4

static int pinmux_write_internal(unsigned int baseaddr, unsigned int index, unsigned int value)
{
	unsigned int addr, field, pinmux;

	addr = baseaddr + ((index / PINS_PER_REG) << 2);
	field = (index % PINS_PER_REG) * BITS_PER_PIN;

	pinmux = readl(addr); // read current pinmux

	pinmux &= ~(0x7 << field);
	pinmux |= (value << field);

	writel(pinmux, addr);
	return 0;
}

int pinmux_write(unsigned int index, unsigned int value)
{
	unsigned int addr = 0;
	if((index >= SOC_PINMUX_INDEX_START) && (index < SOC_PINMUX_INDEX_MAX)) {
		//SOC
		addr = MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_pinMuxCntlBus;
		return pinmux_write_internal(addr, (index - SOC_PINMUX_INDEX_START), value);
	}

	if((index >= AVIO_PINMUX_INDEX_START) && (index < AVIO_PINMUX_INDEX_MAX)) {
		//AVIO
		addr = MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_avioPinMuxCntlBus;
		return pinmux_write_internal(addr, (index - AVIO_PINMUX_INDEX_START), value);
	}

	if((index >= SM_PINMUX_INDEX_START) && (index < SM_PINMUX_INDEX_MAX)) {
		//SM
		addr = SOC_SM_SYS_CTRL_REG_BASE + RA_smSysCtl_smPinMuxCntlBus;
		return pinmux_write_internal(addr, (index - SM_PINMUX_INDEX_START), value);
	}

	printf("error when set pinmux %d, %d\n", index, value);
	return -1;
}



int pinmux_read(unsigned int index)
{
#if 0
	unsigned int addr, field, pinmux;

	if(index >= PINMUX_INDEX_MAX) {
		//dbg_printf("Pinmux Index error ! MAX number is %d",PINMUX_INDEX_MAX);
		return -1;
	}

	//if ((index == URT0_RXD) || (index == URT0_TXD))
		//dbg_printf("You are changing the UART pinmux!\n");

	addr = MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_pinMuxCntlBus + ((index / PINS_PER_REG) << 2);
	field = (index % PINS_PER_REG) * BITS_PER_PIN;

	pinmux = readl(addr); // read current pinmux

	pinmux = (pinmux >> field) & 0x7;

	return pinmux;
#endif
	return 0;
}

static void set_bits(uintptr_t reg_addr, unsigned int mask, int value)
{
	unsigned int reg_value;

	reg_value = read32((void *)reg_addr);
	reg_value &= ~(mask);
	reg_value |= (value&mask);
	write32((void *)reg_addr, reg_value);
}

void pinmux_TW1_sel_set(int id)
{
	set_bits(MEMMAP_CHIP_CTRL_REG_BASE + (BA_Gbl_chipCntl_TW1_SEL & (~3)),
		MSK32Gbl_chipCntl_TW1_SEL,
		((unsigned int)id) << LSb32Gbl_chipCntl_TW1_SEL);
}

void pinmux_TW2_sel_set(int id)
{
	set_bits(SOC_SM_SYS_CTRL_REG_BASE + (RA_smSysCtl_SM_PORT_SEL_CTRL & (~3)),
		MSK32smSysCtl_SM_PORT_SEL_CTRL_TW2,
		((unsigned int)id) << LSb32smSysCtl_SM_PORT_SEL_CTRL_TW2);
}
