// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <common.h>
#include <backlight.h>
#include <dm.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <dm/device_compat.h>
#include <linux/delay.h>
#include <power/regulator.h>
#include <i2c.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>


// #define _Test_Pattern_
// #define _uart_debug_
// #define _read_edid_
// #define _Msa_Active_Only_
#define _eDP_2G7_
// #define _eDP_1G62_
#define _link_train_enable_

struct edp_panel_priv {
	struct udevice *reg;
	struct udevice *backlight;

	struct gpio_desc reset_gpio;
	struct gpio_desc standby_gpio;

	struct gpio_desc enable_gpio;
	struct gpio_desc bl_gpio;
};

static const struct display_timing default_timing = {
	.pixelclock.typ		= 142850000,
	.hactive.typ		= 1920,
	.hfront_porch.typ	= 48,
	.hback_porch.typ	= 200,
	.hsync_len.typ		= 32,
	.vactive.typ		= 1080,
	.vfront_porch.typ	= 3,
	.vback_porch.typ	= 31,
	.vsync_len.typ		= 6,
};

static int edp_panel_i2c_write(struct udevice *dev, uint addr, uint8_t data)
{
	uint8_t valb;
	int err;
	valb = data;

	err = dm_i2c_write(dev, addr, &valb, 1);
	return err;
}

static int edp_panel_i2c_read(struct udevice *dev, uint8_t addr, uint8_t *data)
{
	uint8_t valb;
	int err;

	err = dm_i2c_read(dev, addr, &valb, 1);
	if (err)
		return err;

	*data = (int)valb;
	return 0;
}

static u8 LT8911EXB_IIC_Read_byte(struct udevice *dev, u8 reg)
{
	uint8_t valb;
	int err;

	err = dm_i2c_read(dev, reg, &valb, 1);
	if (err)
		return err;
	udelay(100);

	return (int)valb;

}

static void LT8911EXB_IIC_Write_byte(struct udevice *dev,u8 reg, u8 val)
{
	dm_i2c_write(dev, reg, &val, 1);
	udelay(100);
}

enum {
	hfp = 0,
	hs,
	hbp,
	hact,
	htotal,
	vfp,
	vs,
	vbp,
	vact,
	vtotal,
	pclk_10khz
};

bool	ScrambleMode = 0;
bool	flag_mipi_on = 0;

#ifdef _read_edid_ // read eDP panel EDID

u8		EDID_DATA[128] = { 0 };
u16		EDID_Timing[11] = { 0 };

bool	EDID_Reply = 0;
#endif

#define _1080P_eDP_Panel_
#define _MIPI_Lane_ 4   // 3, 2, 1

#define _MIPI_data_PN_Swap_En	0xF0
#define _MIPI_data_PN_Swap_Dis	0x00

#define _MIPI_data_PN_ _MIPI_data_PN_Swap_Dis

#define _No_swap_		0x00    // 3210 default
#define _MIPI_data_3210_	0       // default
#define _MIPI_data_0123_	21
#define _MIPI_data_2103_	20

#define _MIPI_data_sequence_ _No_swap_

#define _Nvid 0         // 0: 0x0080,default
static int Nvid_Val[] = { 0x0080, 0x0800 };

#ifdef _1080P_eDP_Panel_

#define eDP_lane	2
#define PCR_PLL_PREDIV	0x40

//According to the timing of the Mipi signal, modify the following parameters:
static int MIPI_Timing[] =
// hfp,	hs,	hbp,	hact,	htotal,	vfp,	vs,	vbp,	vact,	vtotal,	pixel_CLK/10000
//-----|---|------|-------|--------|-----|-----|-----|--------|--------|---------------
{48, 32, 200, 1920, 2200, 3, 6, 31, 1080, 1120, 14285};     // boe config for linux

//#define _6bit_ // eDP panel Color Depth，262K color
#define _8bit_ // eDP panel Color Depth，16.7M color

#endif

void Reset_LT8911EXB(struct udevice *dev)
{
	struct edp_panel_priv *priv = dev_get_priv(dev);

	pr_debug("%s: device %s\n", __func__, dev->name);

	dm_gpio_set_value(&priv->reset_gpio, 1);
	mdelay(20);
	dm_gpio_set_value(&priv->reset_gpio, 0);
	mdelay(20);
	dm_gpio_set_value(&priv->reset_gpio, 1);
	mdelay(50);
}

void LT8911EX_ChipID(struct udevice *dev)                                          // read Chip ID
{
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 );                                //register bank
	LT8911EXB_IIC_Write_byte(dev, 0x08, 0x7f );

#ifdef _uart_debug_
	pr_info( "\r\nLT8911EXB chip ID: 0x%x", LT8911EXB_IIC_Read_byte(dev, 0x00 ) );    // 0x17
	pr_info( ",0x%x", LT8911EXB_IIC_Read_byte(dev, 0x01 ) );                          // 0x05
	pr_info( ",0x%x", LT8911EXB_IIC_Read_byte(dev, 0x02 ) );                           // 0xE0
#endif
}

void LT8911EXB_read_edid(struct udevice *dev)
{
#ifdef _read_edid_
	u8 reg, i, j;
//	bool	aux_reply, aux_ack, aux_nack, aux_defer;
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
	LT8911EXB_IIC_Write_byte(dev, 0x00, 0x20 ); //Soft Link train
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte(dev, 0x2a, 0x01 );

	/*set edid offset addr*/
	LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x40 ); //CMD
	LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x00 ); //addr[15:8]
	LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x50 ); //addr[7:0]
	LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x00 ); //data lenth
	LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x00 ); //data lenth
	LT8911EXB_IIC_Write_byte(dev, 0x2c, 0x00 ); //start Aux read edid

#ifdef _uart_debug_
	pr_info( "\r\n" );
	pr_info( "\r\nRead eDP EDID......" );
#endif

	mdelay( 20 );                         //more than 10ms
	reg = LT8911EXB_IIC_Read_byte(dev, 0x25);

	pr_debug( "\r\nRead eDP EDID.reg = %02x.....\n", reg);
	if( ( reg & 0x0f ) == 0x0c )
	{
		for( j = 0; j < 8; j++ )
		{
			if( j == 7 )
			{
				LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x10 ); //MOT
			}else
			{
				LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x50 );
			}

			LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x00 );
			LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x50 );
			LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x0f );
			LT8911EXB_IIC_Write_byte(dev, 0x2c, 0x00 ); //start Aux read edid
			mdelay( 50 );                         //more than 50ms

			if( LT8911EXB_IIC_Read_byte(dev, 0x39 ) == 0x31 )
			{
				LT8911EXB_IIC_Read_byte(dev, 0x2b );
				for( i = 0; i < 16; i++ )
				{
					EDID_DATA[j * 16 + i] = LT8911EXB_IIC_Read_byte(dev, 0x2b );
				}

				EDID_Reply = 1;
			}else
			{
				EDID_Reply = 0;
				return;
			}
		}

		EDID_Timing[hfp] = (EDID_DATA[0x41] & 0xC0) * 4 + EDID_DATA[0x3e];
		EDID_Timing[hs] = (EDID_DATA[0x41] & 0x30) * 16 + EDID_DATA[0x3f];
		EDID_Timing[hbp] = ((EDID_DATA[0x3a] & 0x0f) * 0x100 + EDID_DATA[0x39]) - ((EDID_DATA[0x41] & 0x30) * 16 + EDID_DATA[0x3f]) - ((EDID_DATA[0x41] & 0xC0 ) * 4 + EDID_DATA[0x3e]);
		EDID_Timing[hact] = (EDID_DATA[0x3a] & 0xf0) * 16 + EDID_DATA[0x38];
		EDID_Timing[htotal] = (EDID_DATA[0x3a] & 0xf0) * 16 + EDID_DATA[0x38] + ((EDID_DATA[0x3a] & 0x0f) * 0x100 + EDID_DATA[0x39]);
		EDID_Timing[vfp] = (EDID_DATA[0x41] & 0x0c) * 4 + (EDID_DATA[0x40] & 0xf0 ) / 16;
		EDID_Timing[vs] = (EDID_DATA[0x41] & 0x03) * 16 + (EDID_DATA[0x40] & 0x0f );
		EDID_Timing[vbp] = ((EDID_DATA[0x3d] & 0x03) * 0x100 + EDID_DATA[0x3c]) - ((EDID_DATA[0x41] & 0x03) * 16 + (EDID_DATA[0x40] & 0x0f)) - ((EDID_DATA[0x41] & 0x0c) * 4 + (EDID_DATA[0x40] & 0xf0 ) / 16);
		EDID_Timing[vact] = (EDID_DATA[0x3d] & 0xf0) * 16 + EDID_DATA[0x3b];
		EDID_Timing[vtotal] = (EDID_DATA[0x3d] & 0xf0 ) * 16 + EDID_DATA[0x3b] + ((EDID_DATA[0x3d] & 0x03) * 0x100 + EDID_DATA[0x3c]);
		EDID_Timing[pclk_10khz] = EDID_DATA[0x37] * 0x100 + EDID_DATA[0x36];
		pr_info( "eDP Timing = { H_FP / H_pluse / H_BP / H_act / H_tol / V_FP / V_pluse / V_BP / V_act / V_tol / D_CLK };" );
		pr_info( "eDP Timing = { %d       %d       %d     %d     %d     %d      %d       %d     %d    %d    %d };\n",
				(u32)EDID_Timing[hfp],(u32)EDID_Timing[hs],(u32)EDID_Timing[hbp],(u32)EDID_Timing[hact],(u32)EDID_Timing[htotal],
				(u32)EDID_Timing[vfp],(u32)EDID_Timing[vs],(u32)EDID_Timing[vbp],(u32)EDID_Timing[vact],(u32)EDID_Timing[vtotal],(u32)EDID_Timing[pclk_10khz]);
	}

	return;

#endif
}

static int lt8911exb_i2c_test(struct udevice *dev)                            //void LT8911EX_ChipID( void )
{
    u8 retry = 0;
    uint8_t chip_id_h = 0, chip_id_m = 0, chip_id_l = 0;
    int ret = -EAGAIN;

    pr_debug("%s: device %s \n", __func__, dev->name);

    while(retry++ < 3) {
        ret = edp_panel_i2c_write(dev, 0xff, 0x81);
        if(ret < 0) {
            pr_info("LT8911EXB i2c test write addr:0xff failed\n");
            continue;
        }

        ret = edp_panel_i2c_write(dev, 0x08, 0x7f);
        if(ret < 0) {
            pr_info("LT8911EXB i2c test write addr:0x08 failed\n");
            continue;
        }

        edp_panel_i2c_read(dev, 0x00, &chip_id_l);
        edp_panel_i2c_read(dev, 0x01, &chip_id_m);
        edp_panel_i2c_read(dev, 0x02, &chip_id_h);
        // LT8911EXB i2c test success chipid: 0xe0517
        pr_debug("LT8911EXB i2c test success chipid: 0x%x%x%x\n", chip_id_h, chip_id_m, chip_id_l);

        ret = 0;
        break;
    }

    return ret;
}

void LT8911EXB_MIPI_Video_Timing(struct udevice *dev)                                    // ( struct video_timing *video_format )
{
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xd0 );
	LT8911EXB_IIC_Write_byte(dev, 0x0d, (u8)( MIPI_Timing[vtotal] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x0e, (u8)( MIPI_Timing[vtotal] % 256 ) );    //vtotal
	LT8911EXB_IIC_Write_byte(dev, 0x0f, (u8)( MIPI_Timing[vact] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x10, (u8)( MIPI_Timing[vact] % 256 ) );      //vactive

	LT8911EXB_IIC_Write_byte(dev, 0x11, (u8)( MIPI_Timing[htotal] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x12, (u8)( MIPI_Timing[htotal] % 256 ) );    //htotal
	LT8911EXB_IIC_Write_byte(dev, 0x13, (u8)( MIPI_Timing[hact] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x14, (u8)( MIPI_Timing[hact] % 256 ) );      //hactive

	LT8911EXB_IIC_Write_byte(dev, 0x15, (u8)( MIPI_Timing[vs] % 256 ) );        //vsa
	LT8911EXB_IIC_Write_byte(dev, 0x16, (u8)( MIPI_Timing[hs] % 256 ) );        //hsa
	LT8911EXB_IIC_Write_byte(dev, 0x17, (u8)( MIPI_Timing[vfp] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x18, (u8)( MIPI_Timing[vfp] % 256 ) );       //vfp

	LT8911EXB_IIC_Write_byte(dev, 0x19, (u8)( MIPI_Timing[hfp] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x1a, (u8)( MIPI_Timing[hfp] % 256 ) );       //hfp

	#ifdef _uart_debug_
	pr_info("------\n");
	pr_info("MIPI_Timing[vtotal] / 256 = %d\n", MIPI_Timing[vtotal] / 256);
	pr_info("MIPI_Timing[vtotal]  256 = %d\n", MIPI_Timing[vtotal] % 256);
	pr_info("MIPI_Timing[vact] / 256 = %d\n", MIPI_Timing[vact] / 256);
	pr_info("MIPI_Timing[vact]  256 = %d\n", MIPI_Timing[vact] % 256);
	pr_info("MIPI_Timing[htotal] / 256 = %d\n", MIPI_Timing[htotal] / 256);
	pr_info("MIPI_Timing[htotal]  256 = %d\n", MIPI_Timing[htotal] % 256);
	pr_info("MIPI_Timing[hact] / 256 = %d\n", MIPI_Timing[hact] / 256);
	pr_info("MIPI_Timing[hact]  256 = %d\n", MIPI_Timing[hact] % 256);

	pr_info("MIPI_Timing[vs]  256 = %d\n", MIPI_Timing[vs] % 256);
	pr_info("MIPI_Timing[hs]  256 = %d\n", MIPI_Timing[hs] % 256);

	pr_info("MIPI_Timing[vfp] / 256 = %d\n", MIPI_Timing[vfp] / 256);
	pr_info("MIPI_Timing[vfp]  256 = %d\n", MIPI_Timing[vfp] % 256);
	pr_info("MIPI_Timing[hfp] / 256 = %d\n", MIPI_Timing[hfp] / 256);
	pr_info("MIPI_Timing[hfp]  256 = %d\n", MIPI_Timing[hfp] % 256);
	pr_info("------\n");
	#endif

}

void LT8911EXB_eDP_Video_cfg(struct udevice *dev)                                        // ( struct video_timing *video_format )
{
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa8 );
	LT8911EXB_IIC_Write_byte(dev, 0x2d, 0x88 );                                 // MSA from register

#ifdef _Msa_Active_Only_
	LT8911EXB_IIC_Write_byte(dev, 0x05, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x06, 0x00 );                                 //htotal
	LT8911EXB_IIC_Write_byte(dev, 0x07, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x08, 0x00 );                                 //h_start

	LT8911EXB_IIC_Write_byte(dev, 0x09, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x0a, 0x00 );                                 //hsa
	LT8911EXB_IIC_Write_byte(dev, 0x0b, (u8)( MIPI_Timing[hact] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x0c, (u8)( MIPI_Timing[hact] % 256 ) );      //hactive

	LT8911EXB_IIC_Write_byte(dev, 0x0d, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x0e, 0x00 );                                 //vtotal

	LT8911EXB_IIC_Write_byte(dev, 0x11, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x12, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x15, (u8)( MIPI_Timing[vact] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x16, (u8)( MIPI_Timing[vact] % 256 ) );      //vactive

#else

	LT8911EXB_IIC_Write_byte(dev, 0x05, (u8)( MIPI_Timing[htotal] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x06, (u8)( MIPI_Timing[htotal] % 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x07, (u8)( ( MIPI_Timing[hs] + MIPI_Timing[hbp] ) / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x08, (u8)( ( MIPI_Timing[hs] + MIPI_Timing[hbp] ) % 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x09, (u8)( MIPI_Timing[hs] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x0a, (u8)( MIPI_Timing[hs] % 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x0b, (u8)( MIPI_Timing[hact] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x0c, (u8)( MIPI_Timing[hact] % 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x0d, (u8)( MIPI_Timing[vtotal] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x0e, (u8)( MIPI_Timing[vtotal] % 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x11, (u8)( ( MIPI_Timing[vs] + MIPI_Timing[vbp] ) / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x12, (u8)( ( MIPI_Timing[vs] + MIPI_Timing[vbp] ) % 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x14, (u8)( MIPI_Timing[vs] % 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x15, (u8)( MIPI_Timing[vact] / 256 ) );
	LT8911EXB_IIC_Write_byte(dev, 0x16, (u8)( MIPI_Timing[vact] % 256 ) );

	#ifdef _uart_debug_
	pr_info("------\n");
	pr_info("(u8)( MIPI_Timing[htotal] / 256 ) = %d\n", (MIPI_Timing[htotal] / 256));
	pr_info("(u8)( MIPI_Timing[htotal]  256 ) = %d\n", (MIPI_Timing[htotal] % 256));
	pr_info("(u8)( ( MIPI_Timing[hs] + MIPI_Timing[hbp] ) / 256 )  = %d\n", ((MIPI_Timing[hs] + MIPI_Timing[hbp]) / 256));
	pr_info("(u8)( ( MIPI_Timing[hs] + MIPI_Timing[hbp] )  256 ) = %d\n", ((MIPI_Timing[hs] + MIPI_Timing[hbp]) % 256));
	pr_info("(u8)( MIPI_Timing[hs] / 256 ) = %d\n", (MIPI_Timing[hs] / 256));
	pr_info("(u8)( MIPI_Timing[hs]  256 ) = %d\n", (MIPI_Timing[hs] % 256));
	pr_info("(u8)( MIPI_Timing[hact] / 256 )  = %d\n", (MIPI_Timing[hact] / 256));
	pr_info("(u8)( MIPI_Timing[hact]  256 ) = %d\n", (MIPI_Timing[hact] % 256));

	pr_info("(u8)( ( MIPI_Timing[vs] + MIPI_Timing[vbp] ) / 256 ) = %d\n", ((MIPI_Timing[vs] + MIPI_Timing[vbp]) / 256));
	pr_info("(u8)( ( MIPI_Timing[vs] + MIPI_Timing[vbp] )  256 ) = %d\n", ((MIPI_Timing[vs] + MIPI_Timing[vbp]) % 256));

	pr_info("(u8)( MIPI_Timing[vs]  256 ) = %d\n", (MIPI_Timing[vs] % 256));
	pr_info("(u8)( MIPI_Timing[vact] / 256 )  = %d\n", (MIPI_Timing[vact] / 256));
	pr_info("(u8)( MIPI_Timing[vact]  256 ) = %d\n", (MIPI_Timing[vact] % 256));
	pr_info("------\n");
	#endif
#endif


}

void LT8911EXB_init(struct udevice *dev)
{
	u8	i;
	u8	pcr_pll_postdiv;
	u8	pcr_m;
	u16 Temp16;

	/* init */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x08, 0x7f ); // i2c over aux issue
	LT8911EXB_IIC_Write_byte(dev, 0x49, 0xff ); // enable 0x87xx

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x5a, 0x0e ); // GPIO test output

	//for power consumption//
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte(dev, 0x05, 0x06 );
	LT8911EXB_IIC_Write_byte(dev, 0x43, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x44, 0x1f );
	LT8911EXB_IIC_Write_byte(dev, 0x45, 0xf7 );
	LT8911EXB_IIC_Write_byte(dev, 0x46, 0xf6 );
	LT8911EXB_IIC_Write_byte(dev, 0x49, 0x7f );

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x82 );
#if ( eDP_lane == 2 )
	{
		LT8911EXB_IIC_Write_byte(dev, 0x12, 0x33 );
	}
#elif ( eDP_lane == 1 )
	{
		LT8911EXB_IIC_Write_byte(dev, 0x12, 0x11 );
	}
#endif

	/* mipi Rx analog */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x32, 0x51 );
	LT8911EXB_IIC_Write_byte(dev, 0x35, 0x22 ); //EQ current 0x22/0x42/0x62/0x82/0xA2/0xC2/0xe2
	LT8911EXB_IIC_Write_byte(dev, 0x3a, 0x77 ); //EQ 12.5db
	LT8911EXB_IIC_Write_byte(dev, 0x3b, 0x77 ); //EQ 12.5db

	LT8911EXB_IIC_Write_byte(dev, 0x4c, 0x0c );
	LT8911EXB_IIC_Write_byte(dev, 0x4d, 0x00 );

	/* dessc_pcr  pll analog */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x6a, 0x40 );
	LT8911EXB_IIC_Write_byte(dev, 0x6b, PCR_PLL_PREDIV );

	Temp16 = MIPI_Timing[pclk_10khz];

	if( MIPI_Timing[pclk_10khz] < 8800 )
	{
		LT8911EXB_IIC_Write_byte(dev, 0x6e, 0x82 ); //0x44:pre-div = 2 ,pixel_clk=44~ 88MHz
		pcr_pll_postdiv = 0x08;
	}else
	if( MIPI_Timing[pclk_10khz] < 17600 )
	{
		LT8911EXB_IIC_Write_byte(dev, 0x6e, 0x81 ); //0x40:pre-div = 1, pixel_clk =88~176MHz
		pcr_pll_postdiv = 0x04;
	}else
	{
		LT8911EXB_IIC_Write_byte(dev, 0x6e, 0x80 ); //0x40:pre-div = 0, pixel_clk =176~200MHz
		pcr_pll_postdiv = 0x02;
	}

	pcr_m = (u8)( Temp16 * pcr_pll_postdiv / 25 / 100 );

	/* dessc pll digital */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x85 );     // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0xa9, 0x31 );
	LT8911EXB_IIC_Write_byte(dev, 0xaa, 0x17 );
	LT8911EXB_IIC_Write_byte(dev, 0xab, 0xba );
	LT8911EXB_IIC_Write_byte(dev, 0xac, 0xe1 );
	LT8911EXB_IIC_Write_byte(dev, 0xad, 0x47 );
	LT8911EXB_IIC_Write_byte(dev, 0xae, 0x01 );
	LT8911EXB_IIC_Write_byte(dev, 0xae, 0x11 );

	/* Digital Top */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x85 );                             // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0xc0, 0x01 );                             //select mipi Rx
#ifdef _6bit_
	LT8911EXB_IIC_Write_byte(dev, 0xb0, 0xd0 );                             //enable dither
#else
	LT8911EXB_IIC_Write_byte(dev, 0xb0, 0x00 );                             // disable dither
#endif

	/* mipi Rx Digital */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xd0 );                             // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x00, _MIPI_data_PN_ + _MIPI_Lane_ % 4 ); // 0: 4 Lane / 1: 1 Lane / 2 : 2 Lane / 3: 3 Lane
	LT8911EXB_IIC_Write_byte(dev, 0x02, 0x08 );                             //settle
	LT8911EXB_IIC_Write_byte(dev, 0x03, _MIPI_data_sequence_ );             // default is 0x00
	LT8911EXB_IIC_Write_byte(dev, 0x08, 0x00 );
//	LT8911EXB_IIC_Write_byte(dev, 0x0a, 0x12 );                             //pcr mode

	LT8911EXB_IIC_Write_byte(dev, 0x0c, 0x80 );                             //fifo position
	LT8911EXB_IIC_Write_byte(dev, 0x1c, 0x80 );                             //fifo position

	// hs mode:MIPI行采样；vs mode:MIPI帧采样
	LT8911EXB_IIC_Write_byte(dev, 0x24, 0x70 );                             // 0x30  [3:0]  line limit	  //pcr mode( de hs vs)

	LT8911EXB_IIC_Write_byte(dev, 0x31, 0x0a );

	/*stage1 hs mode*/
	LT8911EXB_IIC_Write_byte(dev, 0x25, 0x90 );                             // 0x80		   // line limit
	LT8911EXB_IIC_Write_byte(dev, 0x2a, 0x3a );                             // 0x04		   // step in limit
	LT8911EXB_IIC_Write_byte(dev, 0x21, 0x4f );                             // hs_step
	LT8911EXB_IIC_Write_byte(dev, 0x22, 0xff );

	/*stage2 de mode*/
	LT8911EXB_IIC_Write_byte(dev, 0x0a, 0x02 );                             //de adjust pre line
	LT8911EXB_IIC_Write_byte(dev, 0x38, 0x02 );                             //de_threshold 1
	LT8911EXB_IIC_Write_byte(dev, 0x39, 0x04 );                             //de_threshold 2
	LT8911EXB_IIC_Write_byte(dev, 0x3a, 0x08 );                             //de_threshold 3
	LT8911EXB_IIC_Write_byte(dev, 0x3b, 0x10 );                             //de_threshold 4

	LT8911EXB_IIC_Write_byte(dev, 0x3f, 0x04 );                             //de_step 1
	LT8911EXB_IIC_Write_byte(dev, 0x40, 0x08 );                             //de_step 2
	LT8911EXB_IIC_Write_byte(dev, 0x41, 0x10 );                             //de_step 3
	LT8911EXB_IIC_Write_byte(dev, 0x42, 0x60 );                             //de_step 4

	/*stage2 hs mode*/
	LT8911EXB_IIC_Write_byte(dev, 0x1e, 0x0A );//LT8911EXB_IIC_Write_byte(dev, 0x1e, 0x01 );                             // 0x11
	LT8911EXB_IIC_Write_byte(dev, 0x23, 0xf0 );                             // 0x80			   //

	LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x80 );                             // 0xa0

#ifdef _Test_Pattern_
	LT8911EXB_IIC_Write_byte(dev, 0x26, ( pcr_m | 0x80 ) );
#else

	LT8911EXB_IIC_Write_byte(dev, 0x26, pcr_m );

#endif

	LT8911EXB_MIPI_Video_Timing(dev);         //defualt setting is 1080P

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x03, 0x7b ); //PCR reset
	LT8911EXB_IIC_Write_byte(dev, 0x03, 0xff );

#ifdef _eDP_2G7_
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte(dev, 0x19, 0x31 );
	LT8911EXB_IIC_Write_byte(dev, 0x1a, 0x36 ); // sync m
	LT8911EXB_IIC_Write_byte(dev, 0x1b, 0x00 ); // sync_k [7:0]
	LT8911EXB_IIC_Write_byte(dev, 0x1c, 0x00 ); // sync_k [13:8]

	// txpll Analog
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x82 );
	LT8911EXB_IIC_Write_byte(dev, 0x09, 0x00 ); // div hardware mode, for ssc.

//	LT8911EXB_IIC_Write_byte(dev, 0x01, 0x18 );// default : 0x18
	LT8911EXB_IIC_Write_byte(dev, 0x02, 0x42 );
	LT8911EXB_IIC_Write_byte(dev, 0x03, 0x00 ); // txpll en = 0
	LT8911EXB_IIC_Write_byte(dev, 0x03, 0x01 ); // txpll en = 1
//	LT8911EXB_IIC_Write_byte(dev, 0x04, 0x3a );// default : 0x3A

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte(dev, 0x0c, 0x10 ); // cal en = 0

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte(dev, 0x09, 0xfc );
	LT8911EXB_IIC_Write_byte(dev, 0x09, 0xfd );

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte(dev, 0x0c, 0x11 ); // cal en = 1

	// ssc
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte(dev, 0x13, 0x83 );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0x41 );
	LT8911EXB_IIC_Write_byte(dev, 0x16, 0x0a );
	LT8911EXB_IIC_Write_byte(dev, 0x18, 0x0a );
	LT8911EXB_IIC_Write_byte(dev, 0x19, 0x33 );
#endif

#ifdef _eDP_1G62_
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte(dev, 0x19, 0x31 );
	LT8911EXB_IIC_Write_byte(dev, 0x1a, 0x20 ); // sync m
	LT8911EXB_IIC_Write_byte(dev, 0x1b, 0x19 ); // sync_k [7:0]
	LT8911EXB_IIC_Write_byte(dev, 0x1c, 0x99 ); // sync_k [13:8]

	// txpll Analog
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x82 );
	LT8911EXB_IIC_Write_byte(dev, 0x09, 0x00 ); // div hardware mode, for ssc.
	//	LT8911EXB_IIC_Write_byte(dev, 0x01, 0x18 );// default : 0x18
	LT8911EXB_IIC_Write_byte(dev, 0x02, 0x42 );
	LT8911EXB_IIC_Write_byte(dev, 0x03, 0x00 ); // txpll en = 0
	LT8911EXB_IIC_Write_byte(dev, 0x03, 0x01 ); // txpll en = 1
	//	LT8911EXB_IIC_Write_byte(dev, 0x04, 0x3a );// default : 0x3A

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte(dev, 0x0c, 0x10 ); // cal en = 0

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte(dev, 0x09, 0xfc );
	LT8911EXB_IIC_Write_byte(dev, 0x09, 0xfd );

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte(dev, 0x0c, 0x11 ); // cal en = 1

	//ssc
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte(dev, 0x13, 0x83 );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0x41 );
	LT8911EXB_IIC_Write_byte(dev, 0x16, 0x0a );
	LT8911EXB_IIC_Write_byte(dev, 0x18, 0x0a );
	LT8911EXB_IIC_Write_byte(dev, 0x19, 0x33 );
#endif

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );

	for( i = 0; i < 5; i++ ) //Check Tx PLL
	{
		mdelay( 5 );
		if( LT8911EXB_IIC_Read_byte(dev, 0x37 ) & 0x02 )
		{
			pr_debug( "\r\nLT8911 tx pll locked \n");
			break;
		}else
		{
			pr_debug( "\r\nLT8911 tx pll unlocked \n");
			LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 );
			LT8911EXB_IIC_Write_byte(dev, 0x09, 0xfc );
			LT8911EXB_IIC_Write_byte(dev, 0x09, 0xfd );

			LT8911EXB_IIC_Write_byte(dev, 0xff, 0x87 );
			LT8911EXB_IIC_Write_byte(dev, 0x0c, 0x10 );
			LT8911EXB_IIC_Write_byte(dev, 0x0c, 0x11 );
		}
	}

	// AUX reset
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x07, 0xfe );
	LT8911EXB_IIC_Write_byte(dev, 0x07, 0xff );
	LT8911EXB_IIC_Write_byte(dev, 0x0a, 0xfc );
	LT8911EXB_IIC_Write_byte(dev, 0x0a, 0xfe );

	/* tx phy */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x11, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x13, 0x10 );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0x0c );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0x08 );
	LT8911EXB_IIC_Write_byte(dev, 0x13, 0x20 );

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x0e, 0x35 );
//	LT8911EXB_IIC_Write_byte(dev, 0x12, 0xff );
//	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x80 );
//	LT8911EXB_IIC_Write_byte(dev, 0x40, 0x22 );

	/*eDP Tx Digital */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa8 ); // Change Reg bank

#ifdef _Test_Pattern_

	LT8911EXB_IIC_Write_byte(dev, 0x24, 0x50 ); // bit2 ~ bit 0 : test panttern image mode
	LT8911EXB_IIC_Write_byte(dev, 0x25, 0x70 ); // bit6 ~ bit 4 : test Pattern color
	LT8911EXB_IIC_Write_byte(dev, 0x27, 0x50 ); //0x50:Pattern; 0x10:mipi video

//	LT8911EXB_IIC_Write_byte(dev, 0x2d, 0x00 ); //  pure color setting
//	LT8911EXB_IIC_Write_byte(dev, 0x2d, 0x84 ); // black color
	LT8911EXB_IIC_Write_byte(dev, 0x2d, 0x88 ); //  block

#else
	LT8911EXB_IIC_Write_byte(dev, 0x27, 0x10 ); //0x50:Pattern; 0x10:mipi video
#endif

#ifdef _6bit_
	LT8911EXB_IIC_Write_byte(dev, 0x17, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x18, 0x00 );
#else
	// _8bit_
	LT8911EXB_IIC_Write_byte(dev, 0x17, 0x10 );
	LT8911EXB_IIC_Write_byte(dev, 0x18, 0x20 );
#endif

	/* nvid */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa0 );                             // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x00, (u8)( Nvid_Val[_Nvid] / 256 ) );    // 0x08
	LT8911EXB_IIC_Write_byte(dev, 0x01, (u8)( Nvid_Val[_Nvid] % 256 ) );    // 0x00
}

void LT8911EXB_video_check(struct udevice *dev)
{
	u32 reg = 0x00;
	/* mipi byte clk check*/
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x85 );     // Change Reg bank
	LT8911EXB_IIC_Write_byte(dev, 0x1d, 0x00 );     //FM select byte clk
	LT8911EXB_IIC_Write_byte(dev, 0x40, 0xf7 );
	LT8911EXB_IIC_Write_byte(dev, 0x41, 0x30 );

	if( ScrambleMode ) {
		LT8911EXB_IIC_Write_byte(dev, 0xa1, 0x82 ); //eDP scramble mode;
	} else {
		LT8911EXB_IIC_Write_byte(dev, 0xa1, 0x02 ); // DP scramble mode;
	}

//	LT8911EXB_IIC_Write_byte(dev, 0x17, 0xf0 ); // 0xf0:Close scramble; 0xD0 : Open scramble

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte(dev, 0x09, 0x7d );
	LT8911EXB_IIC_Write_byte(dev, 0x09, 0xfd );

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x85 );

	mdelay(50);
	if( LT8911EXB_IIC_Read_byte(dev, 0x50 ) == 0x03 )
	{
		reg	   = LT8911EXB_IIC_Read_byte(dev, 0x4d );
		reg	   = reg * 256 + LT8911EXB_IIC_Read_byte(dev, 0x4e );
		reg	   = reg * 256 + LT8911EXB_IIC_Read_byte(dev, 0x4f );

		pr_debug( "\r\nvideo check: mipi byteclk = %d ", reg ); // mipi byteclk = reg * 1000
		pr_debug( "\r\nvideo check: mipi bitrate = %d ", reg * 8 ); // mipi byteclk = reg * 1000
		pr_debug( "\r\nvideo check: mipi pclk = %d ", reg /3 * 4 * 1000 ); // mipi byteclk = reg * 1000
	}else
	{
		pr_debug( "\r\nvideo check: mipi clk unstable" );
	}

	/* mipi vtotal check*/
	reg	   = LT8911EXB_IIC_Read_byte(dev, 0x76 );
	reg	   = reg * 256 + LT8911EXB_IIC_Read_byte(dev, 0x77 );

	pr_debug( "\r\nvideo check: Vtotal =  %d", reg);

	/* mipi word count check*/
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xd0 );
	reg	   = LT8911EXB_IIC_Read_byte(dev, 0x82 );
	reg	   = reg * 256 + LT8911EXB_IIC_Read_byte(dev, 0x83 );
	reg	   = reg / 3;

	pr_debug( "\r\nvideo check: Hact(word counter) =  %d", reg);

	/* mipi Vact check*/
	reg	   = LT8911EXB_IIC_Read_byte(dev, 0x85 );
	reg	   = reg * 256 + LT8911EXB_IIC_Read_byte(dev, 0x86 );

	pr_debug( "\r\nvideo check: Vact = %d", reg);
}

void DpcdWrite(struct udevice *dev, u32 Address, u8 Data )
{
	/***************************
	   注意大小端的问题!
	   这里默认是大端模式

	   Pay attention to the Big-Endian and Little-Endian!
	   The default mode is Big-Endian here.

	 ****************************/
	u8	AddressH   = 0x0f & ( Address >> 16 );
	u8	AddressM   = 0xff & ( Address >> 8 );
	u8	AddressL   = 0xff & Address;

	u8	reg;

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte(dev, 0x2b, ( 0x80 | AddressH ) );  //CMD
	LT8911EXB_IIC_Write_byte(dev, 0x2b, AddressM );             //addr[15:8]
	LT8911EXB_IIC_Write_byte(dev, 0x2b, AddressL );             //addr[7:0]
	LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x00 );                 //data lenth
	LT8911EXB_IIC_Write_byte(dev, 0x2b, Data );                 //data
	LT8911EXB_IIC_Write_byte(dev, 0x2c, 0x00 );                 //start Aux

	mdelay( 20 );                                         //more than 10ms
	reg = LT8911EXB_IIC_Read_byte(dev, 0x25 );

	if( ( reg & 0x0f ) == 0x0c )
	{
		return;
	}
}

u8 DpcdRead(struct udevice *dev, u32 Address )
{
	/***************************
	   注意大小端的问题!
	   这里默认是大端模式

	   Pay attention to the Big-Endian and Little-Endian!
	   The default mode is Big-Endian here.

	 ****************************/

	u8	DpcdValue  = 0x00;
	u8	AddressH   = 0x0f & ( Address >> 16 );
	u8	AddressM   = 0xff & ( Address >> 8 );
	u8	AddressL   = 0xff & Address;
	u8	reg;

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
	LT8911EXB_IIC_Write_byte(dev, 0x00, 0x20 );                 //Soft Link train
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte(dev, 0x2a, 0x01 );

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte(dev, 0x2b, ( 0x90 | AddressH ) );  //CMD
	LT8911EXB_IIC_Write_byte(dev, 0x2b, AddressM );             //addr[15:8]
	LT8911EXB_IIC_Write_byte(dev, 0x2b, AddressL );             //addr[7:0]
	LT8911EXB_IIC_Write_byte(dev, 0x2b, 0x00 );                 //data lenth
	LT8911EXB_IIC_Write_byte(dev, 0x2c, 0x00 );                 //start Aux read edid

	mdelay( 50 );                                         //more than 10ms
	reg = LT8911EXB_IIC_Read_byte(dev, 0x25 );
	if( ( reg & 0x0f ) == 0x0c )
	{
		if( LT8911EXB_IIC_Read_byte(dev, 0x39 ) == 0x22 )
		{
			LT8911EXB_IIC_Read_byte(dev, 0x2b );
			DpcdValue = LT8911EXB_IIC_Read_byte(dev, 0x2b );
		}
	}else
	{
		LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 ); // change bank
		LT8911EXB_IIC_Write_byte(dev, 0x07, 0xfe );
		LT8911EXB_IIC_Write_byte(dev, 0x07, 0xff );
		LT8911EXB_IIC_Write_byte(dev, 0x0a, 0xfc );
		LT8911EXB_IIC_Write_byte(dev, 0x0a, 0xfe );
	}

	return DpcdValue;
}

void LT8911EX_link_train(struct udevice *dev)
{
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte(dev, 0x06, 0xdf ); // rset VID TX
	LT8911EXB_IIC_Write_byte(dev, 0x06, 0xff );

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x85 );

//	LT8911EXB_IIC_Write_byte(dev, 0x17, 0xf0 ); // turn off scramble

	if( ScrambleMode ) {
		LT8911EXB_IIC_Write_byte(dev, 0xa1, 0x82 ); // eDP scramble mode;

		/* Aux operater init */

		LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
		LT8911EXB_IIC_Write_byte(dev, 0x00, 0x20 ); //Soft Link train
		LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa6 );
		LT8911EXB_IIC_Write_byte(dev, 0x2a, 0x01 );

		DpcdWrite(dev, 0x010a, 0x01 );
		mdelay( 10 );
		DpcdWrite(dev, 0x0102, 0x00 );
		mdelay( 10 );
		DpcdWrite(dev, 0x010a, 0x01 );

		mdelay( 20 );
	} else {
		LT8911EXB_IIC_Write_byte(dev, 0xa1, 0x02 ); // DP scramble mode;
	}

	/* Aux setup */
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
	LT8911EXB_IIC_Write_byte(dev, 0x00, 0x60 );     //Soft Link train
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte(dev, 0x2a, 0x00 );

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte(dev, 0x07, 0xfe );
	LT8911EXB_IIC_Write_byte(dev, 0x07, 0xff );
	LT8911EXB_IIC_Write_byte(dev, 0x0a, 0xfc );
	LT8911EXB_IIC_Write_byte(dev, 0x0a, 0xfe );

	/* link train */

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0x85 );
	LT8911EXB_IIC_Write_byte(dev, 0x1a, eDP_lane );

#ifdef _link_train_enable_
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
	LT8911EXB_IIC_Write_byte(dev, 0x00, 0x64 );
	LT8911EXB_IIC_Write_byte(dev, 0x01, 0x0a );
	LT8911EXB_IIC_Write_byte(dev, 0x0c, 0x85 );
	LT8911EXB_IIC_Write_byte(dev, 0x0c, 0xc5 );
#else
	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
	LT8911EXB_IIC_Write_byte(dev, 0x00, 0x00 );
	LT8911EXB_IIC_Write_byte(dev, 0x01, 0x0a );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0x80 );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0x81 );
	mdelay( 10 );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0x84 );
	mdelay( 10 );
	LT8911EXB_IIC_Write_byte(dev, 0x14, 0xc0 );
#endif
}

// void LT8911EX_link_train_result(struct udevice *dev)
// {
// 	u8 i, reg;
// 	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
// 	for( i = 0; i < 10; i++ )
// 	{
// 		reg = LT8911EXB_IIC_Read_byte(dev, 0x82 );
// 		if( reg & 0x20 )
// 		{
// 			if( ( reg & 0x1f ) == 0x1e )
// 			{
// 				pr_debug( "\r\nLink train success, 0x82 = 0x%x", reg );
// 			} else
// 			{
// 				pr_debug( "\r\nLink train fail, 0x82 = 0x%x", reg );
// 			}

// 			pr_debug( "\r\npanel link rate: 0x%x", LT8911EXB_IIC_Read_byte(dev, 0x83 ) );
// 			pr_debug( "\r\npanel link count: 0x%x", LT8911EXB_IIC_Read_byte(dev, 0x84 ) );
// 			return;
// 		}else
// 		{
// 			pr_debug( "\r\nlink trian on going..." );
// 		}
// 		mdelay( 100 );
// 	}
// }

enum
{
	_Level0_ = 0,                                               // 27.8 mA  0x83/0x00
	_Level1_,                                                   // 26.2 mA  0x82/0xe0
	_Level2_,                                                   // 24.6 mA  0x82/0xc0
	_Level3_,                                                   // 23 mA    0x82/0xa0
	_Level4_,                                                   // 21.4 mA  0x82/0x80
	_Level5_,                                                   // 18.2 mA  0x82/0x40
	_Level6_,                                                   // 16.6 mA  0x82/0x20
	_Level7_,                                                   // 15mA     0x82/0x00  // level 1
	_Level8_,                                                   // 12.8mA   0x81/0x00  // level 2
	_Level9_,                                                   // 11.2mA   0x80/0xe0  // level 3
	_Level10_,                                                  // 9.6mA    0x80/0xc0  // level 4
	_Level11_,                                                  // 8mA      0x80/0xa0  // level 5
	_Level12_,                                                  // 6mA      0x80/0x80  // level 6
};

u8	Swing_Setting1[] = { 0x83, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x81, 0x80, 0x80, 0x80, 0x80 };
u8	Swing_Setting2[] = { 0x00, 0xe0, 0xc0, 0xa0, 0x80, 0x40, 0x20, 0x00, 0x00, 0xe0, 0xc0, 0xa0, 0x80 };

u8	Level = _Level7_;

void LT8911EX_TxSwingPreSet(struct udevice *dev)
{
	LT8911EXB_IIC_Write_byte(dev, 0xFF, 0x82 );
	LT8911EXB_IIC_Write_byte(dev, 0x22, Swing_Setting1[Level] );    //lane 0 tap0
	LT8911EXB_IIC_Write_byte(dev, 0x23, Swing_Setting2[Level] );
	LT8911EXB_IIC_Write_byte(dev, 0x24, 0x80 );                     //lane 0 tap1
	LT8911EXB_IIC_Write_byte(dev, 0x25, 0x00 );

#if ( eDP_lane == 2 )
	LT8911EXB_IIC_Write_byte(dev, 0x26, Swing_Setting1[Level] );    //lane 1 tap0
	LT8911EXB_IIC_Write_byte(dev, 0x27, Swing_Setting2[Level] );
	LT8911EXB_IIC_Write_byte(dev, 0x28, 0x80 );                     //lane 1 tap1
	LT8911EXB_IIC_Write_byte(dev, 0x29, 0x00 );
#endif
}

void PCR_Status(struct udevice *dev)                                         // for debug
{
#ifdef _uart_debug_
	u8 reg;

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xd0 );
	reg = LT8911EXB_IIC_Read_byte(dev, 0x87 );

	pr_info( "\r\nReg0xD087 =	");
	pr_info( " 0x%x ", reg );
	pr_info( "\r\n " );
	if( reg & 0x10 )
	{
		pr_info( "\r\nPCR Clock stable" );
	}else
	{
		pr_info( "\r\nPCR Clock unstable" );
	}
	pr_info( "\r\n " );
#endif
}

void LT8911EXB_LinkTrainResultCheck(struct udevice *dev)
{
#ifdef _link_train_enable_
	u8	i;
	u8	val;
	//int ret;

	LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
	for( i = 0; i < 10; i++ )
	{
		val = LT8911EXB_IIC_Read_byte(dev, 0x82 );
		if( val & 0x20 )
		{
			if( ( val & 0x1f ) == 0x1e )
			{
#ifdef _uart_debug_
				pr_info( "\r\nedp link train successed: 0x%x", val );
#endif
				return;
			}else
			{
#ifdef _uart_debug_
				pr_info( "\r\nedp link train failed: 0x%x", val );
#endif
				LT8911EXB_IIC_Write_byte(dev, 0xff, 0xac );
				LT8911EXB_IIC_Write_byte(dev, 0x00, 0x00 );
				LT8911EXB_IIC_Write_byte(dev, 0x01, 0x0a );
				LT8911EXB_IIC_Write_byte(dev, 0x14, 0x80 );
				LT8911EXB_IIC_Write_byte(dev, 0x14, 0x81 );
				mdelay( 50 );
				LT8911EXB_IIC_Write_byte(dev, 0x14, 0x84 );
				mdelay( 50 );
				LT8911EXB_IIC_Write_byte(dev, 0x14, 0xc0 );
			}

#ifdef _uart_debug_

			val = LT8911EXB_IIC_Read_byte(dev, 0x83 );
			pr_info( "\r\npanel link rate: 0x%x", val );
			val = LT8911EXB_IIC_Read_byte(dev, 0x84 );
			pr_info( "\r\npanel link count:0x%x ", val );
#endif
			mdelay( 10 );
		}else
		{
			mdelay( 10 );
		}
	}
#endif
}

static int edp_panel_enable_backlight(struct udevice *dev)
{
	struct edp_panel_priv *priv = dev_get_priv(dev);
	pr_debug("%s: device %s \n", __func__, dev->name);

	Reset_LT8911EXB(dev);     // Reset LT8911EXB
	LT8911EX_ChipID(dev);     // read Chip ID

	LT8911EXB_eDP_Video_cfg(dev);

	LT8911EXB_init(dev);
	LT8911EX_TxSwingPreSet(dev);

	LT8911EXB_read_edid(dev); // for debug
	ScrambleMode = 0;
	LT8911EX_link_train(dev);
	LT8911EXB_LinkTrainResultCheck(dev);
	// LT8911EX_link_train_result(dev);	// for debug

	LT8911EXB_video_check(dev);		// just for Check MIPI Input

	pr_debug("\r\nDpcdRead(0x0202) = 0x%x\r\n",DpcdRead(dev,0x0202));

	PCR_Status(dev);			// just for Check PCR CLK

	dm_gpio_set_value(&priv->bl_gpio, 1);

	return 0;
}

static int edp_panel_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	pr_debug("%s: device %s \n", __func__, dev->name);
	memcpy(timings, &default_timing, sizeof(*timings));
	return 0;
}

static int edp_panel_of_to_plat(struct udevice *dev)
{
	pr_debug("%s: device %s \n", __func__, dev->name);
	return 0;
}

static int edp_panel_probe(struct udevice *dev)
{
	struct edp_panel_priv *priv = dev_get_priv(dev);
	int ret = 0;

	pr_debug("%s: device %s \n", __func__, dev->name);

	ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset_gpio,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_info("%s: Warning: cannot get reset GPIO: ret=%d\n",
		      __func__, ret);
	}

	ret = gpio_request_by_name(dev, "standby-gpios", 0, &priv->standby_gpio,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_info("%s: Warning: cannot get standy GPIO: ret=%d\n",
		      __func__, ret);
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0, &priv->enable_gpio,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_info("%s: Warning: cannot get enable GPIO: ret=%d\n",
		      __func__, ret);
	}

	ret = gpio_request_by_name(dev, "bl-gpios", 0, &priv->bl_gpio,
				   GPIOD_IS_OUT);
	if (ret) {
		pr_info("%s: Warning: cannot get bl GPIO: ret=%d\n",
		      __func__, ret);
	}

	dm_gpio_set_value(&priv->standby_gpio, 1);
	mdelay(50);
	dm_gpio_set_value(&priv->standby_gpio, 0);
	mdelay(50);
	dm_gpio_set_value(&priv->standby_gpio, 1);
	mdelay(100);

	dm_gpio_set_value(&priv->enable_gpio, 1);

	Reset_LT8911EXB(dev);

	ret = lt8911exb_i2c_test(dev);

	return ret;
}

static const struct panel_ops edp_panel_ops = {
	.enable_backlight = edp_panel_enable_backlight,
	.get_display_timing = edp_panel_get_display_timing,
};

static const struct udevice_id edp_panel_ids[] = {
	{ .compatible = "lontium,lt8911exb" },
	{ }
};

U_BOOT_DRIVER(edp_panel) = {
	.name			= "edp_panel",
	.id			= UCLASS_PANEL,
	.of_match		= edp_panel_ids,
	.ops			= &edp_panel_ops,
	.of_to_plat		= edp_panel_of_to_plat,
	.probe			= edp_panel_probe,
	.plat_auto		= sizeof(struct mipi_dsi_panel_plat),
	.priv_auto		= sizeof(struct edp_panel_priv),
};
