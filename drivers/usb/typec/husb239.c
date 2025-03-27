// SPDX-License-Identifier: GPL-2.0+
/*
 * husb239 typec controller
 *
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <i2c.h>
#include <linux/usb/pd.h>
#include <linux/delay.h>

#define HUSB239_REG_PORTROLE		0x00
#define HUSB239_REG_CONTROL			0x01
#define HUSB239_REG_CONTROL1		0x02
#define HUSB239_REG_MANUAL			0x03
#define HUSB239_REG_RESET			0x04
#define HUSB239_REG_MASK			0x05
#define HUSB239_REG_MASK1			0x06
#define HUSB239_REG_MASK2			0x07
#define HUSB239_REG_INT				0x09
#define HUSB239_REG_INT1			0x0A
#define HUSB239_REG_INT2			0x0B
#define HUSB239_REG_USER_CFG0		0x0C
#define HUSB239_REG_USER_CFG1		0x0D
#define HUSB239_REG_USER_CFG2		0x0E

#define HUSB239_REG_GO_COMMAND		0x18
#define HUSB239_REG_SRC_PDO			0x19

#define HUSB239_REG_STATUS			0x63
#define HUSB239_REG_STATUS1			0x64
#define HUSB239_REG_TYPE			0x65
#define HUSB239_REG_DPDM_STATUS		0x66
#define HUSB239_REG_CONTRACT_STATUS0	0x67
#define HUSB239_REG_CONTRACT_STATUS1	0x68

#define HUSB239_REG_SRC_PDO_5V		0x6A
#define HUSB239_REG_SRC_PDO_9V		0x6B
#define HUSB239_REG_SRC_PDO_12V		0x6C

#define HUSB239_REG_MAX				0xFF

#define HUSB239_REG_PORTROLE_ORIENTDEB			BIT(6)
#define HUSB239_REG_PORTROLE_MASK				GENMASK(5, 4)
#define HUSB239_REG_PORTROLE_DRP_DEFAULT		(0x0 << 4)
#define HUSB239_REG_PORTROLE_DRP_TRY_SNK		(0x1 << 4)
#define HUSB239_REG_PORTROLE_DRP_TRY_SRC		(0x2 << 4)
#define HUSB239_REG_PORTROLE_AUDIOACC			BIT(3)
#define HUSB239_REG_PORTROLE_DRP				BIT(2)

#define HUSB239_REG_CONTROL_T_DRP				GENMASK(7, 6)
#define HUSB239_REG_CONTROL_T_DRP_60			(0x0 << 6)
#define HUSB239_REG_CONTROL_T_DRP_70			(0x1 << 6)
#define HUSB239_REG_CONTROL_T_DRP_80			(0x2 << 6)
#define HUSB239_REG_CONTROL_T_DRP_90			(0x3 << 6)
#define HUSB239_REG_CONTROL_DRPTOGGLE			GENMASK(5, 4)
#define HUSB239_REG_CONTROL_DRPTOGGLE_60_40		(0x0 << 4)
#define HUSB239_REG_CONTROL_DRPTOGGLE_50_50		(0x1 << 4)
#define HUSB239_REG_CONTROL_DRPTOGGLE_40_60		(0x2 << 4)
#define HUSB239_REG_CONTROL_DRPTOGGLE_30_70		(0x3 << 4)
#define HUSB239_REG_CONTROL_HOST_CUR			GENMASK(2, 1)
#define HUSB239_REG_CONTROL_HOST_CUR_DEFAULT	(0x1 << 1)
#define HUSB239_REG_CONTROL_HOST_CUR_1_5A		(0x2 << 1)
#define HUSB239_REG_CONTROL_HOST_CUR_3A			(0x3 << 1)
#define HUSB239_REG_CONTROL_INT_MASK			BIT(0)

#define HUSB239_REG_CONTROL1_EN_DPM_HIZ			BIT(5)
#define HUSB239_REG_CONTROL1_VDM_RESPOND		BIT(4)
#define HUSB239_REG_CONTROL1_I2C_ENABLE			BIT(3)
#define HUSB239_REG_CONTROL1_TCCDEB				GENMASK(2, 0)
#define HUSB239_REG_CONTROL1_TCCDEB_150			(0x3 << 0)

#define HUSB239_REG_RESET_SW_RST				BIT(0)

#define HUSB239_REG_MASK_ALL					0xFF

#define HUSB239_REG_MASK_FLGIN					BIT(7)
#define HUSB239_REG_MASK_OREINT					BIT(6)
#define HUSB239_REG_MASK_FAULT					BIT(5)
#define HUSB239_REG_MASK_VBUS_CHG				BIT(4)
#define HUSB239_REG_MASK_VBUS_OV				BIT(3)
#define HUSB239_REG_MASK_BC_LVL					BIT(2)
#define HUSB239_REG_MASK_DETACH					BIT(1)
#define HUSB239_REG_MASK_ATTACH					BIT(0)

#define HUSB239_REG_MASK1_TSD					BIT(7)
#define HUSB239_REG_MASK1_VBUS_UV				BIT(6)
#define HUSB239_REG_MASK1_DR_ROLE				BIT(5)
#define HUSB239_REG_MASK1_PR_ROLE				BIT(4)
#define HUSB239_REG_MASK1_SRC_ALERT				BIT(3)
#define HUSB239_REG_MASK1_FRC_FAIL				BIT(2)
#define HUSB239_REG_MASK1_FRC_SUCC				BIT(1)
#define HUSB239_REG_MASK1_VDM_MSG				BIT(0)

#define HUSB239_REG_MASK2_EXIT_EPR				BIT(3)
#define HUSB239_REG_MASK2_GO_FAIL				BIT(2)
#define HUSB239_REG_MASK2_EPR_MODE				BIT(1)
#define HUSB239_REG_MASK2_PD_HV					BIT(0)

#define HUSB239_REG_USER_CFG2_PD_PRIOR			BIT(2)

#define HUSB239_REG_GO_COMMAND_MASK				GENMASK(4, 0)
#define HUSB239_REG_GO_PDO_SELECT				0x1

#define HUSB239_REG_SRC_PDO_SEL_MASK			GENMASK(7, 3)
#define HUSB239_REG_SRC_PDO_SEL_5V				(0x1 << 3)
#define HUSB239_REG_SRC_PDO_SEL_9V				(0x2 << 3)
#define HUSB239_REG_SRC_PDO_SEL_12V				(0x3 << 3)

#define HUSB239_REG_STATUS_AMS_PROCESS			BIT(7)
#define HUSB239_REG_STATUS_PD_EPR_SNK			BIT(6)
#define HUSB239_REG_STATUS_ORIENT_MASK			GENMASK(5, 4)
#define HUSB239_REG_STATUS_CC1_RPRD				(0x1 << 4)
#define HUSB239_REG_STATUS_CC2_RPRD				(0x2 << 4)
#define HUSB239_REG_STATUS_TSD					BIT(3)
#define HUSB239_REG_STATUS_BC_LVL				GENMASK(2, 1)
#define HUSB239_REG_STATUS_ATTACH				BIT(0)

#define HUSB239_REG_STATUS1_FLGIN				BIT(7)
#define HUSB239_REG_STATUS1_POWER_ROLE			BIT(6)
#define HUSB239_REG_STATUS1_PD_HV				BIT(5)
#define HUSB239_REG_STATUS1_PD_COMM				BIT(4)
#define HUSB239_REG_STATUS1_SRC_ALERT			BIT(3)
#define HUSB239_REG_STATUS1_AMS_SUCC			BIT(2)
#define HUSB239_REG_STATUS1_FAULT				BIT(1)
#define HUSB239_REG_STATUS1_DATA_ROLE			BIT(0)

#define HUSB239_REG_TYPE_CC_RX_ACTIVE			BIT(7)
#define HUSB239_REG_TYPE_DEBUGSRC				BIT(6)
#define HUSB239_REG_TYPE_DEBUGSNK				BIT(5)
#define HUSB239_REG_TYPE_SINK					BIT(4)
#define HUSB239_REG_TYPE_SOURCE					BIT(3)
#define HUSB239_REG_TYPE_ACTIVECABLE			BIT(2)
#define HUSB239_REG_TYPE_AUDIOVBUS				BIT(1)
#define HUSB239_REG_TYPE_AUDIO					BIT(0)

#define HUSB239_PD_CONTRACT_MASK				GENMASK(7, 4)
#define HUSB239_PD_CONTRACT_SHIFT				4

#define HUSB239_REG_SRC_DETECT					(0x1 << 7)

#define HUSB239_DATA_ROLE(s)					(!!((s) & BIT(0)))
#define HUSB239_PD_COMM(s)						(!!((s) & BIT(4)))
#define HUSB239_POWER_ROLE(s)					(!!((s) & BIT(6)))

enum husb239_snkcap {
	SNKCAP_5V = 1,
	SNKCAP_9V,
	SNKCAP_12V,
	SNKCAP_15V,
	SNKCAP_20V,
	MAX_SNKCAP
};

struct husb239_plat {
	u32 req_voltage;/* mv */

	u32 voltage;/* mv */
	u32 op_current;/* ma */

	u32 src_pdo[PDO_MAX_OBJECTS];
	u32 sink_pdo[PDO_MAX_OBJECTS];

	u8 src_pdo_nr;
	u8 sink_pdo_nr;
	u32 sink_voltage;/* mv */
	u32 sink_current;/* ma */
};

#define STEP_BOUNDARY 0x7d
static void husb239_update_operating_status(struct udevice *dev)
{
	struct husb239_plat *husb239 = dev_get_plat(dev);
	int status, status0, status1, type, pd_contract;
	u32 voltage, op_current;

	status = dm_i2c_reg_read(dev, HUSB239_REG_STATUS);
	if (status < 0)
		return;

	type = dm_i2c_reg_read(dev, HUSB239_REG_TYPE);
	if (status < 0)
		return;

	dev_info(dev, "update operating status: %x type: %x\n", status, type);

	if (!(status & HUSB239_REG_STATUS_ATTACH)) {
		husb239->voltage = 0;
		husb239->op_current = 0;
		goto out;
	}

	status0 = dm_i2c_reg_read(dev, HUSB239_REG_CONTRACT_STATUS0);
	if (status0 < 0)
		return;

	dev_info(dev, "contract status0: %x\n", status0);
	pd_contract = (status0 & HUSB239_PD_CONTRACT_MASK) >> HUSB239_PD_CONTRACT_SHIFT;

	if (!(type & HUSB239_REG_TYPE_SINK))
		return;

	if (!pd_contract) {
		husb239->voltage = 5000;
		husb239->op_current = 500;
		goto out;
	}

	switch (pd_contract) {
	case SNKCAP_5V:
		voltage = 5000;
		break;
	case SNKCAP_9V:
		voltage = 9000;
		break;
	case SNKCAP_12V:
		voltage = 12000;
		break;
	case SNKCAP_15V:
		voltage = 15000;
		break;
	case SNKCAP_20V:
		voltage = 20000;
		break;
	default:
		return;
	}

	status1 = dm_i2c_reg_read(dev, HUSB239_REG_CONTRACT_STATUS1);
	if (status1 < 0)
		return;

	if (status1 < STEP_BOUNDARY)
		op_current = 500 + status1 * 20;
	else
		op_current = 3000 + (status1 - STEP_BOUNDARY) * 40;

	/* covert mV/mA to uV/uA */
	husb239->voltage = voltage;
	husb239->op_current = op_current;

out:
	dev_info(dev, "update sink voltage: %dmV current: %dmA\n", husb239->voltage, husb239->op_current);
}

static int husb239_usbpd_detect(struct udevice *dev)
{
	int ret;
	int count = 10;

	while(--count) {
		ret = dm_i2c_reg_read(dev, HUSB239_REG_CONTRACT_STATUS0);
		if(ret < 0)
			return ret;

		dev_info(dev, "husb239 detect pd, contract status0: %x\n", ret);
		if (((ret & HUSB239_PD_CONTRACT_MASK) >> HUSB239_PD_CONTRACT_SHIFT) == SNKCAP_5V) {
			husb239_update_operating_status(dev);
			break;
		}

		/* check attach status */
		ret = dm_i2c_reg_read(dev, HUSB239_REG_STATUS);
		if(ret < 0)
			return ret;

		if (!(ret & HUSB239_REG_STATUS_ATTACH))
			return -ENODEV;

		mdelay(50);
	}

	if (count == 0)
		return -EINVAL;

	return 0;
}

static int husb239_usbpd_request_voltage(struct udevice *dev)
{
	struct husb239_plat *husb239 = dev_get_plat(dev);
	int ret, snk_sel;
	int count = 10;

	if (husb239->req_voltage == husb239->voltage)
		return 0;

	switch (husb239->req_voltage) {
	case 5000:
		snk_sel = SNKCAP_5V;
		break;
	case 9000:
		snk_sel = SNKCAP_9V;
		break;
	case 12000:
		snk_sel = SNKCAP_12V;
		break;
	case 15000:
		snk_sel = SNKCAP_15V;
		break;
	case 20000:
		snk_sel = SNKCAP_20V;
		break;
	default:
		dev_info(dev, "voltage %d is not support, use default 9v\n", husb239->req_voltage);
		snk_sel = SNKCAP_9V;
		break;
	}

	while(--count) {
		ret = dm_i2c_reg_read(dev, HUSB239_REG_SRC_PDO_5V + snk_sel - 1);
		if(ret < 0)
			return ret;

		dev_info(dev, "husb239_attach src_pdo: %x\n", ret);
		if (ret & HUSB239_REG_SRC_DETECT)
			break;

		/* check attach status */
		ret = dm_i2c_reg_read(dev, HUSB239_REG_STATUS);
		if(ret < 0)
			return ret;

		if (!(ret & HUSB239_REG_STATUS_ATTACH))
			return -ENODEV;

		mdelay(100);
	}

	if (count == 0)
		return -EINVAL;

	dev_info(dev, "pd detect\n");
	ret = dm_i2c_reg_clrset(dev, HUSB239_REG_SRC_PDO,
				HUSB239_REG_SRC_PDO_SEL_MASK, (snk_sel << 3));
	if (ret)
		return ret;

	count = 20;
	while(--count) {
		ret = dm_i2c_reg_read(dev, HUSB239_REG_TYPE);
		if(ret < 0)
			return ret;

		dev_info(dev, "husb239 check cc rx active, type: %x\n", ret);
		if (!(ret & HUSB239_REG_TYPE_CC_RX_ACTIVE))
			break;

		/* check attach status */
		ret = dm_i2c_reg_read(dev, HUSB239_REG_STATUS);
		if (ret)
			return ret;

		if (!(ret & HUSB239_REG_STATUS_ATTACH))
			return -ENODEV;

		mdelay(100);
	}

	ret = dm_i2c_reg_clrset(dev, HUSB239_REG_GO_COMMAND,
				HUSB239_REG_GO_COMMAND_MASK, HUSB239_REG_GO_PDO_SELECT);
	if (ret)
		return ret;

	return ret;
}

static int husb239_attach(struct udevice *dev)
{
	struct husb239_plat *husb239 = dev_get_plat(dev);
	int type;

	type = dm_i2c_reg_read(dev, HUSB239_REG_TYPE);
	if(type < 0)
		return type;

	dev_info(dev, "husb239 attach type: %x\n", type);
	if (type & HUSB239_REG_TYPE_SINK) {
		if (!husb239_usbpd_detect(dev)) {
			husb239->req_voltage = husb239->sink_voltage;/* mv */
			husb239_usbpd_request_voltage(dev);
		}
	}
	return 0;
}

static void husb239_detach(struct udevice *dev)
{
	husb239_update_operating_status(dev);
	return;
}

static void husb239_pd_contract(struct udevice *dev)
{
	husb239_update_operating_status(dev);
	return;
}

int husb239_detect(struct udevice *dev)
{
	int int0, int1, int2;

	int0 = dm_i2c_reg_read(dev, HUSB239_REG_INT);
	int1 = dm_i2c_reg_read(dev, HUSB239_REG_INT1);
	int2 = dm_i2c_reg_read(dev, HUSB239_REG_INT2);
	if (int0 || int1)
		dev_info(dev, "int0: 0x%x int1: 0x%x\n", int0, int1);

	dm_i2c_reg_write(dev, HUSB239_REG_INT, int0);
	dm_i2c_reg_write(dev, HUSB239_REG_INT1, int1);
	dm_i2c_reg_write(dev, HUSB239_REG_INT2, int2);

	if (int1 & HUSB239_REG_MASK_ATTACH)
		husb239_attach(dev);

	if (int1 & HUSB239_REG_MASK_DETACH)
		husb239_detach(dev);

	if (int0 & HUSB239_REG_MASK2_PD_HV)
		husb239_pd_contract(dev);

	return 0;
}

int husb239_detect_pd(void)
{
	struct udevice *dev;

	for (uclass_get_device_by_driver(UCLASS_I2C_GENERIC,
		 DM_DRIVER_GET(husb239), &dev); dev;
		 uclass_next_device(&dev)) {
		if (!device_active(dev))
			continue;
		husb239_detect(dev);
	}

	return 0;
}

static int __maybe_unused husb239_chip_init(struct udevice *dev)
{
	int ret;
#if 0
	/* Chip soft reset */
	ret = dm_i2c_reg_clrset(dev, HUSB239_REG_RESET,
				HUSB239_REG_RESET_SW_RST, HUSB239_REG_RESET_SW_RST);
	if (ret < 0)
		return ret;
#endif
	mdelay(10);

	/* PORTROLE init */
	ret = dm_i2c_reg_write(dev, HUSB239_REG_PORTROLE,
				HUSB239_REG_PORTROLE_ORIENTDEB |
				HUSB239_REG_PORTROLE_DRP_TRY_SNK |
				HUSB239_REG_PORTROLE_AUDIOACC |
				HUSB239_REG_PORTROLE_DRP);
	if (ret < 0)
		return ret;

	ret = dm_i2c_reg_write(dev, HUSB239_REG_CONTROL,
				HUSB239_REG_CONTROL_T_DRP_70 |
				HUSB239_REG_CONTROL_DRPTOGGLE_60_40 |
				HUSB239_REG_CONTROL_HOST_CUR_3A);
	if (ret < 0)
		return ret;

	/*  enable USB Communications Capable
	 *  0xb8 =0x25; 0xcb =0x37; 0xdf= 0x48; 0x1f=0x33;
	 *  0x4a[3]=1b; 0x1f= 0x00;
	 */
	ret = dm_i2c_reg_write(dev, 0xB8, 0x25)
			| dm_i2c_reg_write(dev, 0xCB, 0x37)
			| dm_i2c_reg_write(dev, 0xDF, 0x48)
			| dm_i2c_reg_write(dev, 0x1F, 0x33);
	if (ret < 0)
		return ret;

	ret = dm_i2c_reg_clrset(dev, 0x4A, 0x8, 0x8);
	if (ret < 0)
		return ret;

	ret = dm_i2c_reg_write(dev, 0x1F, 0x00);
	if (ret < 0)
		return ret;

	ret = dm_i2c_reg_write(dev, HUSB239_REG_CONTROL1,
				HUSB239_REG_CONTROL1_VDM_RESPOND |
				HUSB239_REG_CONTROL1_I2C_ENABLE |
				HUSB239_REG_CONTROL1_TCCDEB_150);
	if (ret < 0)
		return ret;

	/* PD has high Priority */
	ret = dm_i2c_reg_clrset(dev, HUSB239_REG_USER_CFG2,
				HUSB239_REG_USER_CFG2_PD_PRIOR, HUSB239_REG_USER_CFG2_PD_PRIOR);
	if (ret < 0)
		return ret;

	/* Mask all interruption */
	ret = (dm_i2c_reg_write(dev, HUSB239_REG_MASK, HUSB239_REG_MASK_ALL) ||
			dm_i2c_reg_write(dev, HUSB239_REG_MASK1, HUSB239_REG_MASK_ALL) ||
			dm_i2c_reg_write(dev, HUSB239_REG_MASK2, HUSB239_REG_MASK_ALL));
	if (ret < 0)
		return ret;

	/* Clear all interruption */
	dm_i2c_reg_write(dev, HUSB239_REG_INT, 0xFF);
	dm_i2c_reg_write(dev, HUSB239_REG_INT1, 0xFF);
	dm_i2c_reg_write(dev, HUSB239_REG_INT2, 0xFF);

	return 0;
}

static int husb239_probe(struct udevice *dev)
{
	struct husb239_plat *husb239 = dev_get_plat(dev);
	int ret, i, vol, cur;
	ofnode connector;
	int count = 10;

	connector = dev_read_subnode(dev, "connector");
	if (ofnode_valid(connector)) {
		ret = ofnode_read_size(connector, "sink-pdos");
		ret /= sizeof(fdt32_t);
		if (ret > 0) {
			husb239->sink_pdo_nr = min_t(u8, ret, PDO_MAX_OBJECTS);
			ret = ofnode_read_u32_array(connector, "sink-pdos",
								 husb239->sink_pdo,
								 husb239->sink_pdo_nr);
			if (ret < 0) {
				dev_err(dev, "sink cap validate failed: %d\n", ret);
				return ret;
			}

			for (i = 0; i < husb239->sink_pdo_nr; i++) {
				switch (pdo_type(husb239->sink_pdo[i])) {
				case PDO_TYPE_FIXED:
					vol = pdo_fixed_voltage(husb239->sink_pdo[i]);
					break;
				case PDO_TYPE_BATT:
				case PDO_TYPE_VAR:
					vol = pdo_max_voltage(husb239->sink_pdo[i]);
					cur = pdo_max_current(husb239->sink_pdo[i]);
					break;
				case PDO_TYPE_APDO:
				default:
					ret = 0;
					break;
				}
				/* max sink voltage/current */
				husb239->sink_voltage = max(5000, vol);/* mv */
				husb239->sink_current = max(500, cur);/* ma */
				dev_dbg(dev, "sink voltage: %dmV sink current: %dmA\n",
							husb239->sink_voltage, husb239->sink_current);
			}
		}
	}

	while(--count) {
		ret = husb239_chip_init(dev);
		if (ret < 0) {
			dev_err(dev, "husb239 init chip fail\n");
			continue;
		}
		break;
	}

	ret = dm_i2c_reg_read(dev, HUSB239_REG_STATUS);
	if(ret < 0)
		return ret;

	if (ret & HUSB239_REG_STATUS_ATTACH)
		husb239_attach(dev);

	return 0;
}

static const struct udevice_id husb239_i2c_ids[] = {
	{ .compatible = "hynetek,husb239" },
	{ }
};

U_BOOT_DRIVER(husb239) = {
	.name		= "husb239",
	.id			= UCLASS_I2C_GENERIC,
	.of_match	= husb239_i2c_ids,
	.probe		= husb239_probe,
	.plat_auto = sizeof(struct husb239_plat),
};
