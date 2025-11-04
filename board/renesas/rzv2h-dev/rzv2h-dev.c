#include <common.h>
#include <cpu_func.h>
#include <image.h>
#include <init.h>
#include <malloc.h>
#include <netdev.h>
#include <dm.h>
#include <dm/platform_data/serial_sh.h>
#include <asm/processor.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <asm/arch/rmobile.h>
#include <asm/arch/rcar-mstp.h>
#include <asm/arch/sh_sdhi.h>
#include <i2c.h>
#include <mmc.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

#define	PFC_BASE			0x10410000
#define	CPG_BASE			0x10420000

#define PWPR				(PFC_BASE + 0x3C04)

#define	P_2A				(PFC_BASE + 0x002A)
#define	PM_2A				(PFC_BASE + 0x0154)
#define	PMC_2A				(PFC_BASE + 0x022A)
#define PFC_PMC26			(PFC_BASE + 0x0226)
#define PFC_PFC26			(PFC_BASE + 0x0498)
#define PFC_PMC29			(PFC_BASE + 0x0229)
#define PFC_PFC29			(PFC_BASE + 0x04A4)

#define	PMC_20				(PFC_BASE + 0x0220)
#define	PFC_20				(PFC_BASE + 0x0480)
#define PMC_23				(PFC_BASE + 0x0223)
#define PFC_23				(PFC_BASE + 0x048C)
#define	PMC_24				(PFC_BASE + 0x0224)
#define	PFC_24				(PFC_BASE + 0x0490)

#define PWPR_REGWE_A			BIT(6)
#define	PWPR_REGWE_B			BIT(5)

#define	CPG_CLKON_9			(CPG_BASE + 0x0624)
#define	CPG_RST_9			(CPG_BASE + 0x0924)
#define	CPG_RST_10			(CPG_BASE + 0x0928)

/* CPG */
#define CPG_BASE			0x10420000
#define CPG_CLKON_ETH0			(CPG_BASE + 0x062C)
#define CPG_CLKMON_ETH0			(CPG_BASE + 0x0814)
#define CPG_RESET_ETH			(CPG_BASE + 0x092C)
#define CPG_RESETMON_ETH		(CPG_BASE + 0x0A14)

#define CPG_RST_USB			(CPG_BASE + 0x0928)
#define CPG_RSTMON4_USB			(CPG_BASE + 0x0A10)
#define CPG_RSTMON5_USB			(CPG_BASE + 0x0A14)
#define CPG_CLKON_USB			(CPG_BASE + 0x062C)
#define CPG_CLKMON_USB			(CPG_BASE + 0x0814)

#define PFC_OEN				(PFC_BASE + 0x3C40)
#define PFC_PWPR			(PFC_BASE + 0x3C04)

/* USB */
#define USBPHY20_BASE			(0x15830000)
#define USBPHY21_BASE			(0x15840000)
#define USBPHY20_RESET			(USBPHY20_BASE + 0x000u)
#define USBPHY21_RESET			(USBPHY21_BASE + 0x000u)

#define USB20_BASE			(0x15800000)
#define USB21_BASE			(0x15810000)
#define USBF_BASE			(0x15820000)

#define COMMCTRL			0x800
#define HcRhDescriptorA			0x048
#define LPSTS				0x102
#define USB2_PHY_UTMICTRL2		0xb04
#define USB2_PHY_RESET			0x000
#define USB2_PHY_OTGR			0x600


void s_init(void)
{
	*(volatile u32 *)PWPR |= (PWPR_REGWE_A | PWPR_REGWE_B);

#if CONFIG_TARGET_RZV2H_DEV
	*(volatile u8 *)PMC_2A   &= ~(0x03<<4);	/* PA5,PA4 port	*/
	*(volatile u8 *)P_2A      = (*(volatile u32 *)P_2A  & ~(0x03<<4)) | (0x01 <<5); /* PA5=1,PA4=0		*/
	*(volatile u16 *)PM_2A    = (*(volatile u32 *)PM_2A & ~(0x0f<<8)) | (0x0c <<8); /* PA5,PA4 output	*/
#endif
#if CONFIG_TARGET_RZV2H_EVK_ALPHA
	/* SD1  */
	*(volatile u8 *)PMC_2A   &= ~(0x03 << 2);/* PA3,PA2 port */
	*(volatile u8 *)P_2A      = (*(volatile u32 *)P_2A  & ~(0x03<<2)) | (0x01 <<3); /* PA3=1,PA2=0		*/
	*(volatile u16 *)PM_2A    = (*(volatile u32 *)PM_2A & ~(0x0f<<4)) | (0x0a <<4); /* PA3,PA2 output	*/

	/* I2C3	*/
	*(volatile u32 *)PFC_23  = (*(volatile u32 *)PFC_23 & 0x00FFFFFF) | (0x01 << 28) | (0x01 << 24);
	*(volatile u8 *)PMC_23   |= (0x03) << 6;	/* P37,P36 multiplexed function	*/

	/* I2C6	*/
	*(volatile u32 *)PFC_24  = (*(volatile u32 *)PFC_24 & 0xFF00FFFF) | (0x01 << 20) | (0x01 << 16);
	*(volatile u8 *)PMC_24   |= (0x03) << 4;	/* P45,P44 multiplexed function	*/

	/* I2C7	*/
	*(volatile u32 *)PFC_24  = (*(volatile u32 *)PFC_24 & 0x00FFFFFF) | (0x01 << 28) | (0x01 << 24);
	*(volatile u8 *)PMC_24   |= (0x03) << 6;	/* P45,P44 multiplexed function	*/

	/* I2C3	*/
	*(volatile u32 *)CPG_CLKON_9 = 0x00800080;
	*(volatile u32 *)CPG_RST_9   = 0x08000800;
	/* I2C6	*/
	*(volatile u32 *)CPG_CLKON_9 = 0x04000400;
	*(volatile u32 *)CPG_RST_9   = 0x40004000;
	/* I2C7	*/
	*(volatile u32 *)CPG_CLKON_9 = 0x08000800;
	*(volatile u32 *)CPG_RST_9   = 0x80008000;
#endif
	/* I2C8 */
	*(volatile u32 *)PFC_20  = (*(volatile u32 *)PFC_20 & 0x00FFFFFF) | (0x01 << 28) | (0x01 << 24);
	*(volatile u8 *)PMC_20   |= (0x03) << 6;	/* P07,P06 multiplexed function	*/

	*(volatile u32 *)PWPR &= ~(PWPR_REGWE_A | PWPR_REGWE_B);

	*(volatile u32 *)CPG_CLKON_9 = 0x00080008;
	*(volatile u32 *)CPG_RST_10  = 0x00010001;

	// Use PLL clock for clk_tx_i only for RGMII mode
	// Wite OEN reg. OEN0 bit "0" for output direction
	*(volatile u32 *)(PFC_OEN) &= ~(0x00000001);
	while((*(volatile u32 *)(PFC_OEN) & 0x00000001) != 0x0)
		;

	/* Enable aclk_csr, aclk, tx, rx, tx_180, rx_180 for ETH0 */
	*(volatile u32 *)(CPG_CLKON_ETH0) = 0x3F003F00;
	while((*(volatile u32 *)(CPG_CLKMON_ETH0) & 0x3F000000) != 0x3F000000)
		;

	/* Reset ETH 0 */
	*(volatile u32 *)(CPG_RESET_ETH) = 0x00010000;
	while((*(volatile u32 *)(CPG_RESETMON_ETH) & 0x00000002) == 0x0)
		;

	*(volatile u32 *)(CPG_RESET_ETH) = 0x00010001;
	while((*(volatile u32 *)(CPG_RESETMON_ETH) & 0x00000002) != 0x0)
		;
}

static void _usbphy_init(void)
{
	/* Overwrite SLEEPM/SUSPENDM signals by USB2PHY Control */
	(*(volatile u32 *)(USBPHY20_BASE + USB2_PHY_UTMICTRL2)) = 0x00000303;
	(*(volatile u32 *)(USBPHY21_BASE + USB2_PHY_UTMICTRL2)) = 0x00000303;

	/* Assert USB2PHY reset */
	(*(volatile u32 *)(USBPHY20_BASE + USB2_PHY_RESET)) = 0x00000206;
	(*(volatile u32 *)(USBPHY21_BASE + USB2_PHY_RESET)) = 0x00000206;

	/* Delay 10us */
	udelay(10);

	/* De-Assert USB2PHY reset */
	(*(volatile u32 *)(USBPHY20_BASE + USB2_PHY_RESET)) = 0x00000200;
	(*(volatile u32 *)(USBPHY21_BASE + USB2_PHY_RESET)) = 0x00000200;

	/* Release overwrites of SLEEPM/SUSMENDM signals, and RESET signal */
	(*(volatile u32 *)(USBPHY20_BASE + USB2_PHY_UTMICTRL2)) = 0x00000003;
	(*(volatile u32 *)(USBPHY20_BASE + USB2_PHY_RESET)) = 0;

	(*(volatile u32 *)(USBPHY21_BASE + USB2_PHY_UTMICTRL2)) = 0x00000003;
	(*(volatile u32 *)(USBPHY21_BASE + USB2_PHY_RESET)) = 0;

	/* Activate VBUS Valid comparator */
	(*(volatile u32 *)(USBPHY20_BASE + USB2_PHY_OTGR)) = 0x00000909;
	(*(volatile u32 *)(USBPHY21_BASE + USB2_PHY_OTGR)) = 0x00000909;
}

static void _reset_usb2(void)
{
	/* Reset for USBTEST */
	(*(volatile u32 *)CPG_RST_USB) |= 0x80008000;
	while((*(volatile u32 *)(CPG_RSTMON5_USB) & 0x00000001) != 0x0);

	/* Reset for USB2 host 0 and 1 */
	(*(volatile u32 *)CPG_RST_USB) |= 0x30003000;
	while((*(volatile u32 *)(CPG_RSTMON4_USB) & 0x60000000) != 0x0);
}

static void board_usb_init(void)
{
	/* Reset USB*/
	_reset_usb2();

	/* Enable clock for USB */
	(*(volatile u32 *)CPG_CLKON_USB) = 0x00F800F8;
	while((*(volatile u32 *)(CPG_CLKMON_USB) & 0x00F80000) != 0x00F80000);

	/* Setup  */
	/* Disable GPIO Write Protect */
	(*(volatile u32 *)PFC_PWPR) |= (0x1u << 6);

#if CONFIG_TARGET_RZV2H_DEV
	/* Set P6_0 as Func.15 for VBUSEN */
	(*(volatile u32 *)PFC_PMC26) |= (0x1u << 0);
	(*(volatile u32 *)PFC_PFC26) &= ~(0xF << 0);
	/* Function mode 15 */
	(*(volatile u32 *)PFC_PFC26) |= (0xF << 0);

	/* Set P6_1 as Func.15 for OVRCUR */
	(*(volatile u32 *)PFC_PMC26) |= (0x1u << 1);
	(*(volatile u32 *)PFC_PFC26) &= ~(0xF << 4);
	/* Function mode 15 */
	(*(volatile u32 *)PFC_PFC26) |= (0xF << 4);

	/* Set P6_2 as Func.15 for VBUSEN */
	(*(volatile u32 *)PFC_PMC26) |= (0x1u << 2);
	(*(volatile u32 *)PFC_PFC26) &= ~(0xF << 8);
	/* Function mode 15 */
	(*(volatile u32 *)PFC_PFC26) |= (0xF << 8);

	/* Set P6_3 as Func.15 for OVRCUR */
	(*(volatile u32 *)PFC_PMC26) |= (0x1u << 3);
	(*(volatile u32 *)PFC_PFC26) &= ~(0xF << 12);
	/* Function mode 15 */
	(*(volatile u32 *)PFC_PFC26) |= (0xF << 12);
#endif /* CONFIG_TARGET_RZV2H_DEV */

#if CONFIG_TARGET_RZV2H_EVK_ALPHA
        /* Set P9_5 as Func.14 for VBUSEN */
        /* Control mode (multiplexed function) */
        (*(volatile u32 *)PFC_PMC29) |= (0x1u << 5);
        (*(volatile u32 *)PFC_PFC29) &= ~(0xF << 20);
        /* Function mode 15 */
        (*(volatile u32 *)PFC_PFC29) |= (0x0E << 20);

        /* Set P9_6 as Func.14 for OVRCUR */
        /* Control mode (multiplexed function) */
        (*(volatile u32 *)PFC_PMC29) |= (0x1u << 6);
        (*(volatile u32 *)PFC_PFC29) &= ~(0xF << 24);
        /* Function mode 14 */
        (*(volatile u32 *)PFC_PFC29) |= (0x0E << 24);

        /* Set P6_6 as Func.14 for VBUSEN */
        /* Control mode (multiplexed function) */
         (*(volatile u32 *)PFC_PMC26) |= (0x1u << 6);
         (*(volatile u32 *)PFC_PFC26) &= ~(0xF << 24);
        /* Function mode 14 */
         (*(volatile u32 *)PFC_PFC26) |= (0x0E << 24);

        /* Set P6_7 as Func.14 for OVRCUR */
        /* Control mode (multiplexed function) */
        (*(volatile u32 *)PFC_PMC26) |= (0x1u << 7);
        (*(volatile u32 *)PFC_PFC26) &= ~(0xF << 28);
        /* Function mode 15 */
        (*(volatile u32 *)PFC_PFC26) |= (0x0E << 28);
#endif /* CONFIG_TARGET_RZV2H_EVK_ALPHA */

	/* Enable Write protect */
	(*(volatile u32 *)PFC_PWPR) &= ~(0x1u << 6);

	/* Initialize phy */
	_usbphy_init();

	/*USB0 is HOST*/
	(*(volatile u32 *)(USB20_BASE + COMMCTRL)) = 0;

	/*USB1 is HOST*/
	(*(volatile u32 *)(USB21_BASE + COMMCTRL)) = 0;

	/* Set USBPHY normal operation (Function only) */
	(*(volatile u16 *)(USBF_BASE + LPSTS)) |= (0x1u << 14);

	/* Overcurrent is not supported */
	(*(volatile u32 *)(USB20_BASE + HcRhDescriptorA)) |= (0x1u << 12);
	(*(volatile u32 *)(USB21_BASE + HcRhDescriptorA)) |= (0x1u << 12);
}

int board_early_init_f(void)
{

	return 0;
}

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	board_usb_init();

	return 0;
}

void reset_cpu(void)
{

}
