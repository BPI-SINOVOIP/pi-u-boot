// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <dm.h>
#include <dm/lists.h>
#include <errno.h>
#include <log.h>
#include <power/spacemit/spacemit_pmic.h>
#include <power/pmic.h>
#include <sysreset.h>

DECLEAR_PM853_REGULATOR_MATCH_DATA;
DECLEAR_SPM8821_REGULATOR_MATCH_DATA;
DECLEAR_SY8810L_REGULATOR_MATCH_DATA;

static int pm8xx_reg_count(struct udevice *dev)
{
	struct pm8xx_priv *priv = dev_get_priv(dev);

	return priv->match->max_registers;
}

static int pm8xx_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_read(dev, reg, buff, len);
	if (ret) {
		debug("read error from device: %p register: %#x!\n", dev, reg);
		return ret;
	}

	return 0;
}

static int pm8xx_write(struct udevice *dev, uint reg, const uint8_t *buff,
			  int len)
{
	int ret;

	ret = dm_i2c_write(dev, reg, buff, len);
	if (ret) {
		debug("write error to device: %p register: %#x!\n", dev, reg);
		return ret;
	}

	return 0;
}

static struct dm_pmic_ops pm8xx_ops = {
	.reg_count = pm8xx_reg_count,
	.read = pm8xx_read,
	.write = pm8xx_write,
};

static const struct udevice_id pm8xx_ids[] = {
	{ .compatible = "spacemit,spm8821", .data = (ulong)&spm8821_regulator_match_data, },
	{ .compatible = "spacemit,pm853", .data = (ulong)&pm853_regulator_match_data, },
	{ .compatible = "spacemit,sy8810l", .data = (ulong)&sy8810l_regulator_match_data, },
	{ }
};

static int pm8xx_probe(struct udevice *dev)
{
	struct pm8xx_priv *priv = dev_get_priv(dev);
	ulong driver_data = dev_get_driver_data(dev);

	priv->match = (struct regulator_match_data *)driver_data;

	return 0;
}

#if CONFIG_IS_ENABLED(PMIC_CHILDREN)
static const struct pmic_child_info pmic_children_info[] = {
	{ .prefix = "DCDC_REG", .driver = "pm8xx_buck"},
	{ .prefix = "EDCDC_REG", .driver = "pm8xx_buck"},
	{ .prefix = "LDO_REG", .driver = "pm8xx_ldo"},
	{ .prefix = "SWITCH_REG", .driver = "pm8xx_switch"},
	{ },
};

static int pm8xx_bind(struct udevice *dev)
{
	ofnode regulators_node;
	int children;

	regulators_node = dev_read_subnode(dev, "regulators");
	if (!ofnode_valid(regulators_node)) {
		debug("%s: %s regulators subnode not found!\n", __func__,
		      dev->name);
		return -ENXIO;
	}

	debug("%s: '%s' - found regulators subnode\n", __func__, dev->name);

	children = pmic_bind_children(dev, regulators_node, pmic_children_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	/* Always return success for this device */
	return 0;
}
#endif

U_BOOT_DRIVER(spacemit_pm8xx) = {
	.name = "spacemit_pm8xx",
	.id = UCLASS_PMIC,
	.of_match = pm8xx_ids,
#if CONFIG_IS_ENABLED(PMIC_CHILDREN)
	.bind = pm8xx_bind,
#endif
	.priv_auto	  = sizeof(struct pm8xx_priv),
	.probe = pm8xx_probe,
	.ops = &pm8xx_ops,
};
