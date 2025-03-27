/*
 * Copyright (C) 2024 Spacemit
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/battery.h>
#include <power/pmic.h>
#include <linux/delay.h>
#include <dm/lists.h>
#include <log.h>

#define REG_VERSION			0x0
#define REG_VCELL			0x2
#define REG_SOC				0x4
#define REG_RRT_ALERT			0x6
#define REG_CONFIG			0x8
#define REG_MODE			0xA
#define REG_VTEMPL			0xC
#define REG_VTEMPH			0xD
#define REG_BATINFO			0x10

#define MODE_SLEEP_MASK		(0x3<<6)
#define MODE_SLEEP		(0x3<<6)
#define MODE_NORMAL		(0x0<<6)
#define MODE_QUICK_START	(0x3<<4)
#define MODE_RESTART		(0xf<<0)

#define CONFIG_UPDATE_FLG	(0x1<<1)
#define ATHD			(0x0<<3)        // ATHD = 0%
#define MASK_ATHD		GENMASK(7, 3)
#define MASK_SOC		GENMASK(12, 0)

#define BATTERY_CAPACITY_ERROR	40*1000
#define BATTERY_CHARGING_ZERO	1800*1000

#define UI_FULL	100
#define DECIMAL_MAX 80
#define DECIMAL_MIN 20

#define CHARGING_ON 1
#define NO_CHARGING 0

#define SIZE_BATINFO	64
static unsigned char config_info[SIZE_BATINFO] = {};

/*struct cw_battery {
	struct udevice *dev;
	int charger_mode;
	int capacity;
	int voltage;
	int status;
	int change;
	int raw_soc;
	int time_to_empty;
};*/

static int cw2015_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_read(dev, reg, buff, len);
	if(ret) {
		debug("read error from device: %p register: %#x!\n", dev, reg);
		return ret;
	}
	return 0;
}

static int cw2015_write(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_write(dev, reg, buff, len);
	if(ret) {
		debug("write error to device: %p register: %#x!\n", dev, reg);
		return ret;
	}
	return 0;
}

static int cw_get_voltage(struct udevice *dev, unsigned int *uV)
{
	int ret;
	unsigned char reg_val[2];
	u16 value16, value16_1, value16_2, value16_3;

	ret = cw2015_read(dev, REG_VCELL, reg_val, 2);
	if(ret < 0) {
		return ret;
	}
	value16 = (reg_val[0] << 8) + reg_val[1];

	ret = cw2015_read(dev, REG_VCELL, reg_val, 2);
	if(ret < 0) {
		return ret;
	}
	value16_1 = (reg_val[0] << 8) + reg_val[1];

	ret = cw2015_read(dev, REG_VCELL, reg_val, 2);
	if(ret < 0) {
		return ret;
	}
	value16_2 = (reg_val[0] << 8) + reg_val[1];

	if(value16 > value16_1) {
		value16_3 = value16;
		value16 = value16_1;
		value16_1 = value16_3;
	}

	if(value16_1 > value16_2) {
		value16_3 =value16_1;
		value16_1 =value16_2;
		value16_2 =value16_3;
	}

	if(value16 >value16_1) {
		value16_3 =value16;
		value16 =value16_1;
		value16_1 =value16_3;
	}

	*uV = value16_1 * 625 / 2048 * 1000;
	return 0;
}

int cw_update_config_info(struct udevice *dev)
{
	int ret;
	unsigned char reg_val;
	int i;
	unsigned char reset_val;

	// make sure no in sleep mode
	ret = cw2015_read(dev, REG_MODE, &reg_val, 1);
	if(ret < 0) {
		return ret;
	}
	reset_val = reg_val;
	if((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP) {
		return -1;
	}

	// update new battery info
	for (i = 0; i < SIZE_BATINFO; i++) {
		ret = cw2015_write(dev, REG_BATINFO + i, &config_info[i], 1);
		if(ret < 0)
			return ret;
	}

	reg_val = 0x00;
	reg_val |= CONFIG_UPDATE_FLG;   // set UPDATE_FLAG
	reg_val &= 0x07;                // clear ATHD
	reg_val |= ATHD;                // set ATHD
	ret = cw2015_write(dev, REG_CONFIG, &reg_val, 1);
	if(ret < 0)
		return ret;

	mdelay(50);
	// reset
	reg_val = 0x00;
	reset_val &= ~(MODE_RESTART);
	reg_val = reset_val | MODE_RESTART;
	ret = cw2015_write(dev, REG_MODE, &reg_val, 1);
	if(ret < 0) 
		return ret;

	mdelay(10);

	ret = cw2015_write(dev, REG_MODE, &reset_val, 1);
	if(ret < 0) 
		return ret;

	mdelay(100);
	printf("cw2015 update config success!\n");

	return 0;
}

static int cw_init(struct udevice *dev)
{
	int ret;
	int i;
	unsigned char reg_val = MODE_NORMAL;

	ret = cw2015_write(dev, REG_MODE, &reg_val, 1);
	if (ret < 0)
		return ret;
	ret = cw2015_read(dev, REG_CONFIG, &reg_val, 1);
	if(ret < 0)
		return ret;
	if (!(reg_val & CONFIG_UPDATE_FLG)) {
		debug("update config flg is true, need update config\n");
		ret = cw_update_config_info(dev);
		if (ret < 0) {
			printf("%s : update config fail\n", __func__);
			return ret;
		}
	} else {
		for(i = 0; i < SIZE_BATINFO; i++) {
			ret = cw2015_read(dev, (REG_BATINFO + i), &reg_val, 1);
			if (ret < 0)
			return ret;

			debug("%X\n", reg_val);
			if (config_info[i] != reg_val)
				break;
		}
		if (i != SIZE_BATINFO) {
			reg_val = MODE_SLEEP;
			ret = cw2015_write(dev, REG_MODE, &reg_val, 1);
			if (ret < 0)
				return ret;

			mdelay(30);
			reg_val = MODE_NORMAL;
			ret = cw2015_write(dev, REG_MODE, &reg_val, 1);
			if (ret < 0)
				return ret;

			printf("config didn't match, need update config\n");
			ret = cw_update_config_info(dev);
			if (ret < 0){
				return ret;
			}
		}
	}

	mdelay(10);
	for (i = 0; i < 30; i++) {
		ret = cw2015_read(dev, REG_SOC, &reg_val, 1);
		if (ret < 0)
			return ret;
		else if (reg_val <= 0x64)
			break;
		mdelay(120);
	}

	if (i >= 30 ){
		reg_val = MODE_SLEEP;
		ret = cw2015_write(dev, REG_MODE, &reg_val, 1);
		printf("cw2015 input unvalid power error, cw2015 join sleep mode\n");
		return -1;
	}

	printf("cw2015 init success!\n");
	return 0;
}

static int cw_get_capacity(struct udevice *dev)
{
	int ret;
	unsigned char reg_val[2];

	ret = cw2015_read(dev, REG_SOC, reg_val, 1);
	if (ret < 0)
		return ret;
	return reg_val[0];
}

static int cw_get_status(struct udevice *dev)
{
	int voltage;
	int ret = cw_get_voltage(dev, &voltage);
	if(ret)
		return ret;

	if(voltage <= 3000000)
		ret = BAT_STATE_VERY_LOW;
	else if(voltage <= 3400000)
		ret = BAT_STATE_NEED_CHARGING;
	else if(voltage > 3400000 && voltage <= 4350000)
		ret = BAT_STATE_NORMAL;
	else
		ret = BAT_STATE_NOT_PRESENT;

	return ret;
}

static struct dm_battery_ops cw2015_battery_ops = {
	.get_voltage = cw_get_voltage,
	.get_status = cw_get_status,
	.get_soc = cw_get_capacity,
};

static int cw2015_probe(struct udevice *dev)
{
	int ret, i;
	int loop = 0;
	const char *data;

	data = dev_read_u8_array_ptr(dev, "cellwise,battery-profile", SIZE_BATINFO);
	if (!data)
		return -1;
	for(i = 0; i < SIZE_BATINFO; i++)
		config_info[i] = *(data + i);
	ret = cw_init(dev);
	while ((loop++ < 3) && (ret != 0)) {
		mdelay(200);
		ret = cw_init(dev);
	}
	if (ret) {
		printf("%s : cw2015 init fail!\n", __func__);
		return ret;
	}
	return 0;
}

static const struct udevice_id cw2015_ids[] = {
	{ .compatible = "spacemit,cw2015", .data = 0 },
	{ }
};

U_BOOT_DRIVER(cw_battery) = {
	.name = "cw-bat",
	.id = UCLASS_BATTERY,
	.of_match = cw2015_ids,
	.ops = &cw2015_battery_ops,
	.probe = cw2015_probe,
	//.priv_auto	= sizeof(struct cw_battery),
};
