// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <log.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <linux/bug.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include "regulator_common.h"
#include <power/spacemit/spacemit_pmic.h>

SPM8821_BUCK_LINER_RANGE; SPM8821_LDO_LINER_RANGE; SPM8821_SWITCH_LINER_RANGE;
SPM8821_REGULATOR_BUCK_DESC; SPM8821_REGULATOR_LDO_DESC; SPM8821_REGULATOR_SWITCH_DESC;
SPM8821_REGULATOR_MATCH_DATA;

PM853_BUCK_LINER_RANGE1; PM853_BUCK_LINER_RANGE2; PM853_LDO_LINER_RANGE1; PM853_LDO_LINER_RANGE2;
PM853_LDO_LINER_RANGE3; PM853_LDO_LINER_RANGE4; PM853_SWITCH_LINER_RANGE;
PM853_REGULATOR_BUCK_DESC; PM853_REGULATOR_LDO_DESC; PM853_REGULATOR_SWITCH_DESC;
PM853_REGULATOR_MATCH_DATA;

SY8810L_BUCK_LINER_RANGE; SY8810L_REGULATOR_DESC; SY8810L_REGULATOR_MATCH_DATA;

/**
 * linear_range_get_value - fetch a value from given range
 * @r:          pointer to linear range where value is looked from
 * @selector:   selector for which the value is searched
 * @val:        address where found value is updated
 *
 * Search given ranges for value which matches given selector.
 *
 * Return: 0 on success, -EINVAL given selector is not found from any of the
 * ranges.
 */
static int linear_range_get_value(const struct pm8xx_linear_range *r, unsigned int selector,
                           unsigned int *val)
{
        if (r->min_sel > selector || r->max_sel < selector)
                return -EINVAL;

        *val = r->min + (selector - r->min_sel) * r->step;

        return 0;
}

/**
 * linear_range_get_value_array - fetch a value from array of ranges
 * @r:          pointer to array of linear ranges where value is looked from
 * @ranges:     amount of ranges in an array
 * @selector:   selector for which the value is searched
 * @val:        address where found value is updated
 *
 * Search through an array of ranges for value which matches given selector.
 *
 * Return: 0 on success, -EINVAL given selector is not found from any of the
 * ranges.
 */
static int linear_range_get_value_array(const struct pm8xx_linear_range *r, int ranges,
                                 unsigned int selector, unsigned int *val)
{
        int i;

        for (i = 0; i < ranges; i++)
                if (r[i].min_sel <= selector && r[i].max_sel >= selector)
                        return linear_range_get_value(&r[i], selector, val);

        return -EINVAL;
}

/**
 * regulator_desc_list_voltage_linear_range - List voltages for linear ranges
 *
 * @desc: Regulator desc for regulator which volatges are to be listed
 * @selector: Selector to convert into a voltage
 *
 * Regulators with a series of simple linear mappings between voltages
 * and selectors who have set linear_ranges in the regulator descriptor
 * can use this function prior regulator registration to list voltages.
 * This is useful when voltages need to be listed during device-tree
 * parsing.
 */
static int regulator_desc_list_voltage_linear_range(const struct pm8xx_buck_desc *desc,
                                             unsigned int selector)
{
        unsigned int val;
        int ret;

        BUG_ON(!desc->n_linear_ranges);

        ret = linear_range_get_value_array(desc->linear_ranges,
                                           desc->n_linear_ranges, selector,
                                           &val);
        if (ret)
                return ret;

        return val;
}

/**
 * linear_range_get_max_value - return the largest value in a range
 * @r:          pointer to linear range where value is looked from
 *
 * Return: the largest value in the given range
 */
static unsigned int linear_range_get_max_value(const struct pm8xx_linear_range *r)
{
        return r->min + (r->max_sel - r->min_sel) * r->step;
}

/**
 * linear_range_get_selector_high - return linear range selector for value
 * @r:          pointer to linear range where selector is looked from
 * @val:        value for which the selector is searched
 * @selector:   address where found selector value is updated
 * @found:      flag to indicate that given value was in the range
 *
 * Return selector for which range value is closest match for given
 * input value. Value is matching if it is equal or higher than given
 * value. If given value is in the range, then @found is set true.
 *
 * Return: 0 on success, -EINVAL if range is invalid or does not contain
 * value greater or equal to given value
 */
static int linear_range_get_selector_high(const struct pm8xx_linear_range *r,
                                   unsigned int val, unsigned int *selector,
                                   bool *found)
{
        *found = false;

        if (linear_range_get_max_value(r) < val)
                return -EINVAL;

        if (r->min > val) {
                *selector = r->min_sel;
                return 0;
        }

        *found = true;

        if (r->step == 0)
                *selector = r->max_sel;
        else
                *selector = DIV_ROUND_UP(val - r->min, r->step) + r->min_sel;

        return 0;
}

/**
 * regulator_map_voltage_linear_range - map_voltage() for multiple linear ranges
 *
 * @rdev: Regulator to operate on
 * @min_uV: Lower bound for voltage
 * @max_uV: Upper bound for voltage
 *
 * Drivers providing linear_ranges in their descriptor can use this as
 * their map_voltage() callback.
 */
static int regulator_map_voltage_linear_range(const struct pm8xx_buck_desc *desc,
                                       int min_uV, int max_uV)
{
        const struct pm8xx_linear_range *range;
        int ret = -EINVAL;
        unsigned int sel;
        bool found;
        int voltage, i;

        if (!desc->n_linear_ranges) {
                BUG_ON(!desc->n_linear_ranges);
                return -EINVAL;
        }

        for (i = 0; i < desc->n_linear_ranges; i++) {
                range = &desc->linear_ranges[i];

                ret = linear_range_get_selector_high(range, min_uV, &sel,
                                                     &found);
                if (ret)
                        continue;
                ret = sel;

                /*
                 * Map back into a voltage to verify we're still in bounds.
                 * If we are not, then continue checking rest of the ranges.
                 */
		voltage = regulator_desc_list_voltage_linear_range(desc, sel);
                if (voltage >= min_uV && voltage <= max_uV)
                        break;
        }

        if (i == desc->n_linear_ranges)
                return -EINVAL;

        return ret;
}

static const struct pm8xx_buck_desc *get_buck_reg(struct udevice *pmic, int num)
{
	struct pm8xx_priv *priv = dev_get_priv(pmic);
	struct regulator_match_data *math = (struct regulator_match_data *)priv->match;

	return math->buck_desc + num;

	return NULL;
}

static int buck_get_value(struct udevice *dev)
{
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_buck_reg(dev->parent, buck);
	int mask = info->vsel_msk;
	int ret;
	unsigned int val;

	if (info == NULL)
		return -ENOSYS;

	ret = pmic_reg_read(dev->parent, info->vsel_reg);
	if (ret < 0)
		return ret;
	val = ret & mask;

	val >>= ffs(mask) - 1;

	return regulator_desc_list_voltage_linear_range(info, val);
}

static int buck_set_value(struct udevice *dev, int uvolt)
{
	int sel, ret = -EINVAL;
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_buck_reg(dev->parent, buck);

	if (info == NULL)
		return -ENOSYS;

	sel = regulator_map_voltage_linear_range(info, uvolt, uvolt);
	if (sel >=0) {
		/* has get the selctor */
		 sel <<= ffs(info->vsel_msk) - 1;
		 ret = pmic_clrsetbits(dev->parent, info->vsel_reg, info->vsel_msk, sel);
	}

	return ret;
}

static int buck_set_suspend_value(struct udevice *dev, int uvolt)
{
	/* the hardware has already support the function */
/**
 *	int sel, ret = -EINVAL;
 *	int buck = dev->driver_data - 1;
 *	const struct pm8xx_buck_desc *info = get_buck_reg(dev->parent, buck);
 *
 *	if (info == NULL)
 *		return -ENOSYS;
 *
 *	sel = regulator_map_voltage_linear_range(info, uvolt, uvolt);
 *	if (sel >=0) {
 *		 // has get the selctor
 *		 sel <<= ffs(info->vsel_sleep_msk) - 1;
 *		 ret = pmic_clrsetbits(dev->parent, info->vsel_sleep_reg, info->vsel_sleep_msk, sel);
 *	}
 *
 *	return ret;
 */
	return 0;
}

static int buck_get_suspend_value(struct udevice *dev)
{
	/* the hardware has already support the function */
/**
 *	int buck = dev->driver_data - 1;
 *	const struct pm8xx_buck_desc *info = get_buck_reg(dev->parent, buck);
 *	int mask = info->vsel_sleep_msk;
 *	int ret;
 *	unsigned int val;
 *
 *	if (info == NULL)
 *		return -ENOSYS;
 *
 *	ret = pmic_reg_read(dev->parent, info->vsel_sleep_reg);
 *	if (ret < 0)
 *		return ret;
 *	val = ret & mask;
 *
 *	val >>= ffs(mask) - 1;
 *
 *	return regulator_desc_list_voltage_linear_range(info, val);
 */
	return 0;
}

static int buck_get_enable(struct udevice *dev)
{
	int ret, val;
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_buck_reg(dev->parent, buck);
	int mask = info->enable_msk;

	if (info == NULL)
		return -ENOSYS;

	ret = pmic_reg_read(dev->parent, info->enable_reg);
	if (ret < 0)
		return ret;

	val = ret & mask;

	val >>= ffs(mask) - 1;

	return val;
}

static int buck_set_enable(struct udevice *dev, bool enable)
{
	int ret;
	unsigned int val = 0;
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_buck_reg(dev->parent, buck);
	int mask = info->enable_msk;

	ret = pmic_reg_read(dev->parent, info->enable_reg);
	if (ret < 0)
		return ret;

	val = (unsigned int)ret;
	val &= mask;
	val >>= ffs(mask) - 1;

	if (enable == val)
		return 0;

	val = enable << (ffs(mask) - 1);

	ret = pmic_clrsetbits(dev->parent, info->enable_reg, info->enable_msk, val);
	if (ret < 0)
		return ret;

	return 0;
}

static int buck_set_suspend_enable(struct udevice *dev, bool enable)
{
	/* TODO */
	return 0;
}

static int buck_get_suspend_enable(struct udevice *dev)
{
	/* TODO */
	return 0;
}

static int pm8xx_buck_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_plat *uc_pdata;

	uc_pdata = dev_get_uclass_plat(dev);

	uc_pdata->type = REGULATOR_TYPE_BUCK;
	uc_pdata->mode_count = 0;

	return 0;
}

static const struct dm_regulator_ops pm8xx_buck_ops = {
	.get_value  = buck_get_value,
	.set_value  = buck_set_value,
	.set_suspend_value = buck_set_suspend_value,
	.get_suspend_value = buck_get_suspend_value,
	.get_enable = buck_get_enable,
	.set_enable = buck_set_enable,
	.set_suspend_enable = buck_set_suspend_enable,
	.get_suspend_enable = buck_get_suspend_enable,
};

U_BOOT_DRIVER(pm8xx_buck) = {
	.name = "pm8xx_buck",
	.id = UCLASS_REGULATOR,
	.ops = &pm8xx_buck_ops,
	.probe = pm8xx_buck_probe,
};

static const struct pm8xx_buck_desc *get_ldo_reg(struct udevice *pmic, int num)
{
	struct pm8xx_priv *priv = dev_get_priv(pmic);
	struct regulator_match_data *math = (struct regulator_match_data *)priv->match;

	return math->ldo_desc + num;

	return NULL;
}

static int ldo_get_value(struct udevice *dev)
{
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_ldo_reg(dev->parent, buck);
	int mask = info->vsel_msk;
	int ret;
	unsigned int val;

	if (info == NULL)
		return -ENOSYS;

	ret = pmic_reg_read(dev->parent, info->vsel_reg);
	if (ret < 0)
		return ret;

	val = ret & mask;

	val >>= ffs(mask) - 1;

	return regulator_desc_list_voltage_linear_range(info, val);
}

static int ldo_set_value(struct udevice *dev, int uvolt)
{
	int sel, ret = -EINVAL;
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_ldo_reg(dev->parent, buck);

	if (info == NULL)
		return -ENOSYS;

	sel = regulator_map_voltage_linear_range(info, uvolt, uvolt);
	if (sel >=0) {
		/* has get the selctor */
		 sel <<= ffs(info->vsel_msk) - 1;
		 ret = pmic_clrsetbits(dev->parent, info->vsel_reg, info->vsel_msk, sel);
	}

	return ret;
}

static int ldo_set_suspend_value(struct udevice *dev, int uvolt)
{
/**
 *	int sel, ret = -EINVAL;
 *	int buck = dev->driver_data - 1;
 *	const struct pm8xx_buck_desc *info = get_ldo_reg(dev->parent, buck);
 *
 *	if (info == NULL)
 *		return -ENOSYS;
 *
 *	sel = regulator_map_voltage_linear_range(info, uvolt, uvolt);
 *	if (sel >=0) {
 *	
 *		 sel <<= ffs(info->vsel_sleep_msk) - 1;
 *		 ret = pmic_clrsetbits(dev->parent, info->vsel_sleep_reg, info->vsel_sleep_msk, sel);
 *	}
 *
 *	return ret;
 */
	return 0;
}

static int ldo_get_suspend_value(struct udevice *dev)
{
/**
 *	int buck = dev->driver_data - 1;
 *	const struct pm8xx_buck_desc *info = get_ldo_reg(dev->parent, buck);
 *	int mask = info->vsel_sleep_msk;
 *	int ret;
 *	unsigned int val;
 *
 *	if (info == NULL)
 *		return -ENOSYS;
 *
 *	ret = pmic_reg_read(dev->parent, info->vsel_sleep_reg);
 *	if (ret < 0)
 *		return ret;
 *	val = ret & mask;
 *
 *	val >>= ffs(mask) - 1;
 *
 *	return regulator_desc_list_voltage_linear_range(info, val);
 */
	return 0;
}

static int ldo_get_enable(struct udevice *dev)
{
	int ret;
	unsigned int val;
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_ldo_reg(dev->parent, buck);
	int mask = info->enable_msk;

	if (info == NULL)
		return -ENOSYS;

	ret = pmic_reg_read(dev->parent, info->enable_reg);
	if (ret < 0)
		return ret;

	val = ret & mask;

	val >>= ffs(mask) - 1;

	return val;
}

static int ldo_set_enable(struct udevice *dev, bool enable)
{
	int ret;
	unsigned int val = 0;
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_ldo_reg(dev->parent, buck);
	int mask = info->enable_msk;

	ret = pmic_reg_read(dev->parent, info->enable_reg);
	if (ret < 0)
		return ret;

	val = (unsigned int)ret;
	val &= mask;
	val >>= ffs(mask) - 1;

	if (enable == val)
		return 0;

	val = enable << (ffs(mask) - 1);

	ret = pmic_clrsetbits(dev->parent, info->enable_reg, info->enable_msk, val);
	if (ret < 0)
		return ret;

	return 0;
}

static int ldo_set_suspend_enable(struct udevice *dev, bool enable)
{
	/* TODO */
	return 0;
}

static int ldo_get_suspend_enable(struct udevice *dev)
{
	/* TODO */
	return 0;
}

static const struct dm_regulator_ops pm8xx_ldo_ops = {
	.get_value  = ldo_get_value,
	.set_value  = ldo_set_value,
	.set_suspend_value = ldo_set_suspend_value,
	.get_suspend_value = ldo_get_suspend_value,
	.get_enable = ldo_get_enable,
	.set_enable = ldo_set_enable,
	.set_suspend_enable = ldo_set_suspend_enable,
	.get_suspend_enable = ldo_get_suspend_enable,
};

static int pm8xx_ldo_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_plat *uc_pdata;

	uc_pdata = dev_get_uclass_plat(dev);

	uc_pdata->type = REGULATOR_TYPE_LDO;
	uc_pdata->mode_count = 0;

	return 0;
}

U_BOOT_DRIVER(pm8xx_ldo) = {
	.name = "pm8xx_ldo",
	.id = UCLASS_REGULATOR,
	.ops = &pm8xx_ldo_ops,
	.probe = pm8xx_ldo_probe,
};

static const struct pm8xx_buck_desc *get_switch_reg(struct udevice *pmic, int num)
{
	struct pm8xx_priv *priv = dev_get_priv(pmic);
	struct regulator_match_data *math = (struct regulator_match_data *)priv->match;

	return math->switch_desc + num;

	return NULL;
}

static int switch_get_value(struct udevice *dev)
{
	return 0;
}

static int switch_set_value(struct udevice *dev, int uvolt)
{
	return 0;
}

static int switch_get_enable(struct udevice *dev)
{
	int ret;
	unsigned int val;
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_switch_reg(dev->parent, buck);
	int mask = info->enable_msk;

	if (info == NULL)
		return -ENOSYS;

	ret = pmic_reg_read(dev->parent, info->enable_reg);
	if (ret < 0)
		return ret;

	val = ret & mask;

	val >>= ffs(mask) - 1;

	return val;
}

static int switch_set_enable(struct udevice *dev, bool enable)
{
	int ret;
	unsigned int val = 0;
	int buck = dev->driver_data - 1;
	const struct pm8xx_buck_desc *info = get_switch_reg(dev->parent, buck);
	int mask = info->enable_msk;

	ret = pmic_reg_read(dev->parent, info->enable_reg);
	if (ret < 0)
		return ret;

	val = (unsigned int)ret;
	val &= mask;
	val >>= ffs(mask) - 1;

	if (enable == val)
		return 0;

	val = enable << (ffs(mask) - 1);

	ret = pmic_clrsetbits(dev->parent, info->enable_reg, info->enable_msk, val);
	if (ret < 0)
		return ret;

	return 0;

}

static int switch_set_suspend_enable(struct udevice *dev, bool enable)
{
	/* TODO */
	return 0;
}

static int switch_get_suspend_enable(struct udevice *dev)
{
	/* TODO */
	return 0;
}

static int switch_set_suspend_value(struct udevice *dev, int uvolt)
{
	return 0;
}

static int switch_get_suspend_value(struct udevice *dev)
{
	return 0;
}

static const struct dm_regulator_ops pm8xx_switch_ops = {
	.get_value  = switch_get_value,
	.set_value  = switch_set_value,
	.get_enable = switch_get_enable,
	.set_enable = switch_set_enable,
	.set_suspend_enable = switch_set_suspend_enable,
	.get_suspend_enable = switch_get_suspend_enable,
	.set_suspend_value = switch_set_suspend_value,
	.get_suspend_value = switch_get_suspend_value,
};

static int pm8xx_switch_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_plat *uc_pdata;

	uc_pdata = dev_get_uclass_plat(dev);

	uc_pdata->type = REGULATOR_TYPE_FIXED;
	uc_pdata->mode_count = 0;

	return 0;
}

U_BOOT_DRIVER(pm8xx_switch) = {
	.name = "pm8xx_switch",
	.id = UCLASS_REGULATOR,
	.ops = &pm8xx_switch_ops,
	.probe = pm8xx_switch_probe,
};
