/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright � 2013-2020 Synaptics Incorporated. All rights reserved.
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
#define _GPIO_C_
/*
 * Programming sample:
 * 1. spi_master_init_iomapper(IOMAPPER_SPI_MASTER), init IOmapper
 * 2. GPIO_PinmuxInit(port), setting both Galois and IOmapper pinmux
 * 3.1 GPIO_IOmapperSetInOut(port, in), setting GPIO pin as in / out.
 * 3.2 GPIO_PortSetInOut(port, in), setting GPIO pin as in /out.
 * 4. GPIO_PortWrite, GPIOPortRead
 *
 * NOTE: GPIO_PinmuxInit shouldn't setup GPIO[13:10], it's SPI#0 for
 * programming IOmapper. Galois GPIO in / out setting is done in
 * GPIO_PortWrite / GPIOPortRead, just for campatible forward with Berlin.
 *
 * NOTE: for Berlin, #1.spi_master_init_iomapper and #3.1 is descarded.
 */
#include "apb_gpio.h"
#include "gpio.h"
#include <asm/io.h>
#include <stdio.h>
/****************************************************
 * FUNCTION: toggle GPIO port between high and low
 * PARAMS: port - GPIO port
 *         value - 1: high; 0: low
 * RETURN: 0 - succeed
 *        -1 - fail
 ***************************************************/
static int SOC_GPIO_PortWrite(int port, int value)
{
	unsigned int reg_ddr, reg_dr, reg_ctl;
	int ddr, dr, ctl;

	if((port >= 0) && (port < 32)){
		reg_ddr = APB_GPIO0_BASE + APB_GPIO_SWPORTA_DDR;
		reg_dr = APB_GPIO0_BASE + APB_GPIO_SWPORTA_DR;
		reg_ctl = APB_GPIO0_BASE + APB_GPIO_PORTA_CTL;
	} else if ((port >= 32) && (port < 64)){
		reg_ddr = APB_GPIO1_BASE + APB_GPIO_SWPORTA_DDR;
		reg_dr = APB_GPIO1_BASE + APB_GPIO_SWPORTA_DR;
		reg_ctl = APB_GPIO1_BASE + APB_GPIO_PORTA_CTL;
		port -= 32;
	} else if ((port >= 64) && (port < 96)){
		reg_ddr = APB_GPIO2_BASE + APB_GPIO_SWPORTA_DDR;
		reg_dr = APB_GPIO2_BASE + APB_GPIO_SWPORTA_DR;
		reg_ctl = APB_GPIO2_BASE + APB_GPIO_PORTA_CTL;
		port -= 64;
	} else
		return -1;

	printf("SOC_GPIO_PortWrite reg_ctl=0x%x \n", reg_ctl);
	/* software mode */
	ctl = readl(reg_ctl);
	ctl &= ~(1 << port);
	writel(ctl, reg_ctl);

	/* set port to output mode */
	printf("SOC_GPIO_PortWrite reg_ddr=0x%x \n", reg_ddr);
	ddr = readl(reg_ddr);
	ddr |= (1<<port);
	writel(ddr, reg_ddr);

	/* set port value */
	printf("SOC_GPIO_PortWrite reg_dr=0x%x \n", reg_dr);
	dr = readl(reg_dr);
	if (value){
		dr |= (1<<port);
	} else {
		dr &= ~(1<<port);
	}
	writel(dr, reg_dr);

	return 0;
}

/****************************************************
 * FUNCTION: read GPIO port status
 * PARAMS: port - GPIO port # (0 ~ 31)
 *         *value - pointer to port status
 * RETURN: 0 - succeed
 *        -1 - fail
 ***************************************************/
static int SOC_GPIO_PortRead(int port, int *value)
{
    unsigned int reg_ddr, reg_ext,reg_ctl;
    int ddr, ext,ctl;

	if((port >= 0) && (port < 32)){
		reg_ddr = APB_GPIO0_BASE + APB_GPIO_SWPORTA_DDR;
		reg_ext = APB_GPIO0_BASE + APB_GPIO_EXT_PORTA;
		reg_ctl = APB_GPIO0_BASE + APB_GPIO_PORTA_CTL;
	} else if ((port >= 32) && (port < 64)){
		reg_ddr = APB_GPIO1_BASE + APB_GPIO_SWPORTA_DDR;
		reg_ext = APB_GPIO1_BASE + APB_GPIO_EXT_PORTA;
		reg_ctl = APB_GPIO1_BASE + APB_GPIO_PORTA_CTL;
		port -= 32;
	} else if ((port >= 64) && (port < 96)){
		reg_ddr = APB_GPIO2_BASE + APB_GPIO_SWPORTA_DDR;
		reg_ext = APB_GPIO2_BASE + APB_GPIO_EXT_PORTA;
		reg_ctl = APB_GPIO2_BASE + APB_GPIO_PORTA_CTL;
		port -= 64;
	} else
		return -1;

	/* software mode */
	ctl = readl(reg_ctl);
	ctl &= ~(1 << port);
	writel(ctl, reg_ctl);

    /* set port to input mode */
    ddr = readl(reg_ddr);
    ddr &= ~(1<<port);
    writel(ddr, reg_ddr);

    /* get port value */
    ext = readl(reg_ext);
    if (ext & (1<<port))
        *value = 1;
    else
        *value = 0;

    return 0;
}

/****************************************************
 * FUNCTION: Set Galois GPIO pin as in or out
 * PARAMS: port - GPIO port
 * 		   in - 1: IN, 0: OUT
 * RETURN: 0 - succeed
 *        -1 - fail
 ***************************************************/
static int SOC_GPIO_PortSetInOut(int port, int in)
{
	unsigned int reg_ddr, reg_ctl;
	int ddr, ctl;

	if((port >= 0) && (port < 32)){
		reg_ddr = APB_GPIO0_BASE + APB_GPIO_SWPORTA_DDR;
		reg_ctl = APB_GPIO0_BASE + APB_GPIO_PORTA_CTL;
	} else if ((port >= 32) && (port < 64)){
		reg_ddr = APB_GPIO1_BASE + APB_GPIO_SWPORTA_DDR;
		reg_ctl = APB_GPIO1_BASE + APB_GPIO_PORTA_CTL;
		port -= 32;
	} else if ((port >= 64) && (port < 96)){
		reg_ddr = APB_GPIO2_BASE + APB_GPIO_SWPORTA_DDR;
		reg_ctl = APB_GPIO2_BASE + APB_GPIO_PORTA_CTL;
		port -= 64;
	} else
		return -1;

	/* software mode */
	ctl = readl(reg_ctl);
	ctl &= ~(1 << port);
	writel(ctl, reg_ctl);

	/* set port to output mode */
	ddr = readl(reg_ddr);
	if (in)
		ddr &= ~(1 << port);
	else
		ddr |= (1 << port);
	writel(ddr, reg_ddr);

	return 0;
}

/****************************************************
 * FUNCTION: toggle SM_GPIO port between high and low
 * PARAMS: port - SM_GPIO port #
 *         value - 1: high; 0: low
 * RETURN: 0 - succeed
 *        -1 - fail
 ***************************************************/
int SM_GPIO_PortWrite(int port, int value)
{
	unsigned int reg_ddr, reg_dr, reg_ctl;
	int ddr, dr, ctl;

	if((port >= 0) && (port <= 21)){
		reg_ddr = SM_APB_GPIO0_BASE + APB_GPIO_SWPORTA_DDR;
		reg_dr = SM_APB_GPIO0_BASE + APB_GPIO_SWPORTA_DR;
		reg_ctl = SM_APB_GPIO0_BASE + APB_GPIO_PORTA_CTL;
	} else
		return -1;

	/* software mode */
	ctl = readl(reg_ctl);
	ctl &= ~(1 << port);
	writel(ctl, reg_ctl);

	/* set port to output mode */
	ddr = readl(reg_ddr);
	ddr |= (1<<port);
	writel(ddr, reg_ddr);

	/* set port value */
	dr = readl(reg_dr);
	if (value){
		dr |= (1<<port);
	} else {
		dr &= ~(1<<port);
	}
	writel(dr, reg_dr);
	return 0;
}

/****************************************************
 * FUNCTION: read SM_GPIO port status
 * PARAMS: port - SM_GPIO port #
 *         *value - pointer to port status
 * RETURN: 0 - succeed
 *        -1 - fail
 ***************************************************/
int SM_GPIO_PortRead(int port, int *value)
{
    unsigned int reg_ddr, reg_ext, reg_ctl;
    int ddr, ext, ctl;

    if((port >= 0) && (port <= 21))
    {
        reg_ddr = SM_APB_GPIO0_BASE + APB_GPIO_SWPORTA_DDR;
        reg_ext = SM_APB_GPIO0_BASE + APB_GPIO_EXT_PORTA;
		reg_ctl = SM_APB_GPIO0_BASE + APB_GPIO_PORTA_CTL;
    } else
        return -1;

	/* software mode */
	ctl = readl(reg_ctl);
	ctl &= ~(1 << port);
	writel(ctl, reg_ctl);

    /* set port to input mode */
    ddr = readl(reg_ddr);
    ddr &= ~(1<<port);
    writel(ddr, reg_ddr);

    /* get port value */
    ext= readl(reg_ext);
    if (ext & (1<<port))
        *value = 1;
    else
        *value = 0;
    return 0;
}


/****************************************************
 * FUNCTION: Set Galois SM_GPIO pin as in or out
 * PARAMS: port - SM_GPIO port # (0 ~ 11)
 * 		   in - 1: IN, 0: OUT
 * RETURN: 0 - succeed
 *        -1 - fail
 ***************************************************/
int SM_GPIO_PortSetInOut(int port, int in)
{
	unsigned int reg_ddr, reg_ctl;
	int ddr, ctl;

	if((port >= 0) && (port <= 21)){
		reg_ddr = SM_APB_GPIO0_BASE + APB_GPIO_SWPORTA_DDR;
		reg_ctl = SM_APB_GPIO0_BASE + APB_GPIO_PORTA_CTL;
	} else
		return -1;

	/* software mode */
	ctl = readl(reg_ctl);
	ctl &= ~(1 << port);
	writel(ctl, reg_ctl);

	/* set port to output mode */
	ddr = readl(reg_ddr);
	if (in)
		ddr &= ~(1 << port);
	else
		ddr |= (1 << port);
	writel(ddr, reg_ddr);

	return 0;
}

#define NORMAL_GPIO_MAP(x) (x + 0)
int GPIO_PortWrite(int port, int value)
{
	return SOC_GPIO_PortWrite(NORMAL_GPIO_MAP(port), value);
}

int GPIO_PortRead(int port, int *value)
{
	return SOC_GPIO_PortRead(NORMAL_GPIO_MAP(port), value);
}

int GPIO_PortSetInOut(int port, int in)
{
	return SOC_GPIO_PortSetInOut(NORMAL_GPIO_MAP(port), in);
}
