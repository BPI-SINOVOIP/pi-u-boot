/*
 * Copyright (C) 2024 Spacemit
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/charger.h>
#include <power/pmic.h>
#include <linux/delay.h>
#include <dm/lists.h>
#include <log.h>
#include <asm-generic/gpio.h>

#define SGM41515_CHRG_CTRL_0	0x00
#define SGM41515_CHRG_CTRL_1	0x01
#define SGM41515_CHRG_CTRL_2	0x02
#define SGM41515_CHRG_CTRL_3	0x03
#define SGM41515_CHRG_CTRL_4	0x04
#define SGM41515_CHRG_CTRL_5	0x05
#define SGM41515_CHRG_CTRL_6	0x06
#define SGM41515_CHRG_CTRL_7	0x07
#define SGM41515_CHRG_STAT	0x08
#define SGM41515_CHRG_FAULT	0x09
#define SGM41515_CHRG_CTRL_a	0x0a
#define SGM41515_CHRG_CTRL_b	0x0b
#define SGM41515_CHRG_CTRL_c	0x0c
#define SGM41515_CHRG_CTRL_d	0x0d
#define SGM41515_INPUT_DET	0x0e
#define SGM41515_CHRG_CTRL_f	0x0f

#define SGM41515_CHRG_EN	BIT(4)
#define SGM41515_HIZ_EN		BIT(7)
#define SGM41515_TERM_EN	BIT(7)
#define SGM41515_VAC_OVP_MASK	GENMASK(7, 6)
#define SGM41515_DPDM_ONGOING	BIT(7)
#define SGM41515_VBUS_GOOD	BIT(7)

#define SGM41515_VREG_V_MASK	GENMASK(7, 3)
#define SGM41515_VREG_V_MAX_uV	4624000
#define SGM41515_VREG_V_MIN_uV	3856000
#define SGM41515_VREG_V_DEF_uV	4208000
#define SGM41515_VREG_V_STEP_uV	32000

#define SGM41515_IINDPM_I_MASK		GENMASK(4, 0)
#define SGM41515_IINDPM_I_MIN_uA	100000
#define SGM41515_IINDPM_I_MAX_uA	3200000
#define SGM41515_IINDPM_STEP_uA		100000
#define SGM41515_IINDPM_DEF_uA		2400000

#define SGM41515_ICHRG_CUR_MASK		GENMASK(5, 0)
#define SGM41515_PG_STAT		BIT(2)

/* WDT TIMER SET  */
#define SGM41515_WDT_TIMER_MASK		GENMASK(5, 4)
#define SGM41515_WDT_TIMER_DISABLE	0
#define SGM41515_WDT_TIMER_40S		BIT(4)
#define SGM41515_WDT_TIMER_80S		BIT(5)
#define SGM41515_WDT_TIMER_160S		(BIT(4)| BIT(5))

#define SGM41515_BOOSTV		(BIT(4)| BIT(5))
#define SGM41515_BOOST_LIM	BIT(7)
#define SGM41515_OTG_EN		BIT(5)

#define SGM41515_PRECHRG_CUR_MASK	GENMASK(7, 4)
#define SGM41515_TERMCHRG_CUR_MASK	GENMASK(3, 0)
#define SGM41515_PRECHRG_I_DEF_uA	120000
#define SGM41515_TERMCHRG_I_DEF_uA	120000
static const unsigned int IPRECHG_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

static const unsigned int ITERM_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

static const unsigned int BOOST_VOLT_LIMIT[] = {
	4850000, 5000000, 5150000, 5300000
};

struct sgm41515_charger {
	struct gpio_desc nqon;
	struct gpio_desc charge_en;
	u32 ichg;	/* charge current */
	u32 ilim;	/* input current */
	u32 vreg;	/* regulation voltage */
	u32 iterm;	/* termination current */
	u32 iprechg;	/* precharge current */
	u32 vlim;	/* minimum system voltage limit */
	u32 max_ichg;
	u32 max_vreg;

};

static int sgm41515_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_read(dev, reg, buff, len);
	if(ret) {
		debug("read error from device: %p register: %#x!\n", dev, reg);
		return ret;
	}
	return 0;
}

static int sgm41515_write(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_write(dev, reg, buff, len);
	if(ret) {
		debug("write error to device: %p register: %#x!\n", dev, reg);
		return ret;
	}
	return 0;
}

static int sgm41515_set_input_curr_lim(struct udevice *dev, int iindpm)
{
	int ret;
	unsigned char reg, reg_val;

	if (iindpm < SGM41515_IINDPM_I_MIN_uA ||
			iindpm > SGM41515_IINDPM_I_MAX_uA)
		return -EINVAL;
	reg_val = (iindpm-SGM41515_IINDPM_I_MIN_uA) / SGM41515_IINDPM_STEP_uA;

	ret = sgm41515_read(dev, SGM41515_CHRG_CTRL_0, &reg, 1);
	reg &= ~SGM41515_IINDPM_I_MASK;
	reg_val |= reg;
	ret = sgm41515_write(dev, SGM41515_CHRG_CTRL_0, &reg_val, 1);
	return ret;
}

static int sgm41515_get_current(struct udevice *dev)
{
    struct sgm41515_charger *priv = dev_get_priv(dev);
    return priv->ichg;
}

static int sgm41515_set_chrg_curr(struct udevice *dev, unsigned int uA)
{
	int ret;
	unsigned char reg, reg_val;

	if (uA <= 40000)
		reg_val = uA / 5000;
	else if (uA <= 110000)
		reg_val = 0x08 + (uA -40000) / 10000;
	else if (uA <= 270000)
		reg_val = 0x0F + (uA -110000) / 20000;
	else if (uA <= 540000)
		reg_val = 0x17 + (uA -270000) / 30000;
	else if (uA <= 1500000)
		reg_val = 0x20 + (uA -540000) / 60000;
	else if (uA <= 2940000)
		reg_val = 0x30 + (uA -1500000) / 120000;
	else
		reg_val = 0x3d;

	ret = sgm41515_read(dev, SGM41515_CHRG_CTRL_2, &reg, 1);
	reg &= ~SGM41515_ICHRG_CUR_MASK;
	reg_val |= reg;
	ret = sgm41515_write(dev, SGM41515_CHRG_CTRL_2, &reg_val, 1);
	return ret;
}

static int sgm41515_set_chrg_volt(struct udevice *dev, int chrg_volt)
{
	int ret;
	unsigned char reg, reg_val;
	struct sgm41515_charger *priv = dev_get_priv(dev);

	if (chrg_volt < SGM41515_VREG_V_MIN_uV)
		chrg_volt = SGM41515_VREG_V_MIN_uV;
	else if (chrg_volt > priv->max_vreg)
		chrg_volt = priv->max_vreg;

	reg_val = (chrg_volt-SGM41515_VREG_V_MIN_uV) / SGM41515_VREG_V_STEP_uV;
	reg_val = reg_val<<3;

	ret = sgm41515_read(dev, SGM41515_CHRG_CTRL_4, &reg, 1);
	reg &= ~SGM41515_VREG_V_MASK;
	reg_val |= reg;
	ret = sgm41515_write(dev, SGM41515_CHRG_CTRL_4, &reg_val, 1);
	return ret;
}

static int sgm41515_set_watchdog_timer(struct udevice *dev, int time)
{
	int ret;
	unsigned char reg, reg_val;

	if (time == 0)
		reg_val = SGM41515_WDT_TIMER_DISABLE;
	else if (time == 40)
		reg_val = SGM41515_WDT_TIMER_40S;
	else if (time == 80)
		reg_val = SGM41515_WDT_TIMER_80S;
	else
		reg_val = SGM41515_WDT_TIMER_160S;

	ret = sgm41515_read(dev, SGM41515_CHRG_CTRL_5, &reg, 1);
	reg &= ~SGM41515_WDT_TIMER_MASK;
	reg_val |= reg;
	ret = sgm41515_write(dev, SGM41515_CHRG_CTRL_5, &reg_val, 1);

	return ret;
}

static int sgm41515_set_output_volt(struct udevice *dev, int uV)
{
	int ret = 0;
	int i = 0;
	unsigned char reg, reg_val;
	while(i < 4){
		if (uV == BOOST_VOLT_LIMIT[i]){
			reg_val = (unsigned char)i;
			break;
		}
		i++;
	}
	if (reg_val < 0)
		return reg_val;
	reg_val = reg_val << 4;
	ret = sgm41515_read(dev, SGM41515_CHRG_CTRL_6, &reg, 1);
	reg &= ~SGM41515_BOOSTV;
	reg_val |= reg;
	ret = sgm41515_write(dev, SGM41515_CHRG_CTRL_6, &reg_val, 1);

	return ret;
}

static int sgm41515_set_term_curr(struct udevice *dev, int uA)
{
	unsigned char reg, reg_val;

	for(reg_val = 1; reg_val < 16 && uA >= ITERM_CURRENT_STABLE[reg_val]; reg_val++)
		;
	reg_val--;
	sgm41515_read(dev, SGM41515_CHRG_CTRL_3, &reg, 1);
	reg &= ~SGM41515_TERMCHRG_CUR_MASK;
	reg_val |= reg;

	return sgm41515_write(dev, SGM41515_CHRG_CTRL_3, &reg_val, 1);
}

static int sgm41515_set_prechrg_curr(struct udevice *dev, int uA)
{
	unsigned char reg, reg_val;

	for(reg_val = 1; reg_val < 16 && uA >= IPRECHG_CURRENT_STABLE[reg_val]; reg_val++)
		;
	reg_val--;
	reg_val = reg_val << 4;
	sgm41515_read(dev, SGM41515_CHRG_CTRL_3, &reg, 1);
	reg &= ~SGM41515_PRECHRG_CUR_MASK;
	reg_val |= reg;

	return sgm41515_write(dev, SGM41515_CHRG_CTRL_3, &reg_val, 1);
}

static int sgm41515_hw_init(struct udevice *dev)
{
	int ret, val;
	struct sgm41515_charger *priv = dev_get_priv(dev);
	/* set input current limit */
	ret = sgm41515_set_input_curr_lim(dev, priv->ilim);
	if(ret) {
		printf("set input current failed\n");
		return ret;
	}
	/* set charge current and voltage limit */
	ret = sgm41515_set_chrg_curr(dev, priv->ichg);
	if(ret) {
		printf("set charge current failed\n");
		return ret;
	}
	ret = sgm41515_set_chrg_volt(dev, priv->max_vreg);
	if(ret) {
		printf("set charge voltage failed\n");
		return ret;
	}
	sgm41515_set_watchdog_timer(dev, 0);
	sgm41515_set_output_volt(dev, 5000000);
	val = dev_read_u32_default(dev, "sgm41515-prechrg-uA", SGM41515_PRECHRG_I_DEF_uA);
	sgm41515_set_prechrg_curr(dev, val);
	val = dev_read_u32_default(dev, "sgm41515-termchrg-uA", SGM41515_TERMCHRG_I_DEF_uA);
	sgm41515_set_term_curr(dev, val);
	return 0;
}

static int sgm41515_enable_charge(struct udevice *dev)
{
	int ret;
	unsigned char reg, reg_val;
	struct sgm41515_charger *priv = dev_get_priv(dev);
	ret = gpio_request_by_name(dev, "ch-en-gpios", 0, &priv->charge_en,
				   GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Warning: cannot get enable GPIO: ret=%d\n",
			  __func__, ret);
		return ret;
	}
	/* enable charge */
	dm_gpio_set_value(&priv->charge_en, 0);

	ret = gpio_request_by_name(dev, "nqon-gpios", 0, &priv->nqon,
				   GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Warning: cannot get enable GPIO: ret=%d\n",
			  __func__, ret);
		return ret;
	}
	/* enable charge */
	dm_gpio_set_value(&priv->nqon, 1);

	reg_val = SGM41515_CHRG_EN;
	ret = sgm41515_read(dev, SGM41515_CHRG_CTRL_1, &reg, 1);
	reg &= ~SGM41515_CHRG_EN;
	reg_val |= reg;
	ret = sgm41515_write(dev, SGM41515_CHRG_CTRL_1, &reg_val, 1);
	if(ret)
		return ret;
	return 0;
}

static int sgm41515_get_status(struct udevice *dev)
{
	unsigned char reg;
	int ret;
	bool online;
	struct sgm41515_charger *priv = dev_get_priv(dev);

	ret = sgm41515_read(dev, SGM41515_CHRG_STAT, &reg, 1);
	if(ret) {
		printf("failed to get charger status\n");
		return ret;
	}

	online = !!(reg & SGM41515_PG_STAT);
	if(online) {
		printf("vbus is attached\n");
		ret = sgm41515_set_input_curr_lim(dev, priv->ilim);
		if(ret) {
			printf("set input current failed\n");
			return ret;
		}
		return 1;
	} else {
		printf("vbus is not attached\n");
		return 0;
	}
	return 0;
}

static int sgm41515_probe(struct udevice *dev)
{
	int ret;
	int input_current, ichrg_limit, vchrg_limit;
	struct sgm41515_charger *priv = dev_get_priv(dev);

	input_current = dev_read_u32_default(dev, "sgm41515-cur-input-uA", 2000000);
	ichrg_limit = dev_read_u32_default(dev, "sgm41515-ichrg-uA", 2000000);
	vchrg_limit = dev_read_u32_default(dev, "sgm41515-vchrg-uV", 4350000);
	priv->max_vreg = vchrg_limit;
	priv->ichg = ichrg_limit;
	priv->ilim = input_current;

	ret = sgm41515_hw_init(dev);
	if(ret)
		return ret;
	/* enable charger */
	sgm41515_enable_charge(dev);
	printf("sgm41515 charger register successfully!\n");
	return 0;
}

static const struct udevice_id sgm41515_ids[] = {
	{ .compatible = "spacemit,sgm41515", .data = 0 },
	{ }
};

static struct dm_charger_ops sgm41515_chg_ops = {
	.get_status = sgm41515_get_status,
	.set_current = sgm41515_set_chrg_curr,
	.get_current = sgm41515_get_current,
};

U_BOOT_DRIVER(sgm41515_charger) = {
	.name = "sgm41515-charger",
	.id = UCLASS_CHARGER,
	.of_match = sgm41515_ids,
	.ops = &sgm41515_chg_ops,
	.probe = sgm41515_probe,
	.priv_auto = sizeof(struct sgm41515_charger),
};
