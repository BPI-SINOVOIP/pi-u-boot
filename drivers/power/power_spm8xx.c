// SPDX-License-Identifier: GPL-2.0+

#include <i2c.h>
#include <common.h>
#include <asm/global_data.h>
#include <linux/bug.h>
#include <asm/barrier.h>
#include <power/spacemit/spacemit_pmic.h>

DECLARE_GLOBAL_DATA_PTR;

SPM8821_BUCK_LINER_RANGE; SPM8821_LDO_LINER_RANGE /* ; SPM8821_SWITCH_LINER_RANGE */;
SPM8821_REGULATOR_BUCK_DESC; SPM8821_REGULATOR_LDO_DESC/* ; SPM8821_REGULATOR_SWITCH_DESC */;

PM853_BUCK_LINER_RANGE1; PM853_BUCK_LINER_RANGE2; PM853_LDO_LINER_RANGE1; PM853_LDO_LINER_RANGE2;
PM853_LDO_LINER_RANGE3; PM853_LDO_LINER_RANGE4; /* PM853_SWITCH_LINER_RANGE; */
PM853_REGULATOR_BUCK_DESC; PM853_REGULATOR_LDO_DESC; /* PM853_REGULATOR_SWITCH_DESC; */

SY8810L_BUCK_LINER_RANGE;SY8810L_REGULATOR_DESC;

static const char *global_compatible[] = {
	"spacemit,pm853",
	"spacemit,spm8821",
	"spacemit,sy8810l",
};

void __regulator_desc_find(const char *name, const struct pm8xx_buck_desc **buck_desc,
		const struct pm8xx_buck_desc **ldo_desc, int *num_buck, int *num_ldo)
{
	if (strcmp(name, global_compatible[0]) == 0) {
		*buck_desc = pm853_buck_desc;
		*num_buck = sizeof(pm853_buck_desc) / sizeof(pm853_buck_desc[0]);
		*ldo_desc = pm853_ldo_desc;
		*num_ldo = sizeof(pm853_ldo_desc) / sizeof(pm853_ldo_desc[0]);
	}

	if (strcmp(name, global_compatible[1]) == 0) {
		*buck_desc = spm8821_buck_desc;
		*num_buck = sizeof(spm8821_buck_desc) / sizeof(spm8821_buck_desc[0]);
		*ldo_desc = spm8821_ldo_desc;
		*num_ldo = sizeof(spm8821_ldo_desc) / sizeof(spm8821_ldo_desc[0]);
	}

	if (strcmp(name, global_compatible[2]) == 0) {
		*buck_desc = sy8810l_buck_desc;
		*num_buck = sizeof(sy8810l_buck_desc) / sizeof(sy8810l_buck_desc[0]);
		*ldo_desc = NULL;
		*num_ldo = 0;
	}
}

/**
 * linear_range_get_value - fetch a value from given range
 * @r:	  pointer to linear range where value is looked from
 * @selector:   selector for which the value is searched
 * @val:	address where found value is updated
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
 * @r:	  pointer to array of linear ranges where value is looked from
 * @ranges:     amount of ranges in an array
 * @selector:   selector for which the value is searched
 * @val:	address where found value is updated
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
 * @r:	  pointer to linear range where value is looked from
 *
 * Return: the largest value in the given range
 */
static unsigned int linear_range_get_max_value(const struct pm8xx_linear_range *r)
{
	return r->min + (r->max_sel - r->min_sel) * r->step;
}

/**
 * linear_range_get_selector_high - return linear range selector for value
 * @r:	  pointer to linear range where selector is looked from
 * @val:	value for which the selector is searched
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

static int __board_pmic_init(const char *name)
{
	unsigned char regval;
	const char *s;
	u32 value, min, max;
	const struct pm8xx_buck_desc *buck_desc, *ldo_desc;
	int offset, bus, ret, sub_offset, len, saddr, i, num_buck, num_ldo, sel;

	offset = fdt_node_offset_by_compatible(gd->fdt_blob, -1, name);
	if (offset < 0) {
		pr_info("%s Get %s node error\n", __func__, name);
		return -EINVAL;
	}

	saddr = fdtdec_get_uint(gd->fdt_blob, offset, "reg", 0);
	if (!saddr) {
		pr_info("%s: %s Node has no reg\n", __func__, name);
		return -EINVAL;
	}

	bus = fdtdec_get_uint(gd->fdt_blob, offset, "bus", 0);
	if (!bus) {
		pr_info("%s: %s Node has no bus\n", __func__, name);
		return -EINVAL;
	}

	ret = i2c_set_bus_num(bus);
	if (ret < 0) {
		pr_info("%s: %s set i2c bus number error\n", __func__, name);
		return -EINVAL;
	}

	ret = i2c_probe(saddr);
	if (ret < 0) {
//		pr_info("%s: %s probe i2c failed\n", __func__, name);
		return -EINVAL;
	}

	__regulator_desc_find(name, &buck_desc, &ldo_desc, &num_buck, &num_ldo);

	offset = fdt_first_subnode(gd->fdt_blob, offset);

	for (sub_offset = fdt_first_subnode(gd->fdt_blob, offset);
		sub_offset >= 0;
		sub_offset = fdt_next_subnode(gd->fdt_blob, sub_offset)) {

		/* find regulator-boot-on property */
		if (!fdt_getprop(gd->fdt_blob, sub_offset, "regulator-boot-on", &len))
			continue;

		max = fdtdec_get_uint(gd->fdt_blob, sub_offset, "regulator-max-microvolt", 0);
		if (!max)
			continue;

		min = fdtdec_get_uint(gd->fdt_blob, sub_offset, "regulator-min-microvolt", 0);
		if (!min)
			continue;

		value = fdtdec_get_uint(gd->fdt_blob, sub_offset, "regulator-init-microvolt", 0);

		/* find wich dcdc or ldo */
		s = fdt_get_name(gd->fdt_blob, sub_offset, &len);

		if ((strncmp(s, "DCDC_REG", 8) == 0) || (strncmp(s, "EDCDC_REG", 9) == 0)) {
			for (i = 0; i < num_buck; ++i) {
				if (strcmp(buck_desc[i].name, s) == 0) {

					/* enable the regulator */
					i2c_read(saddr, buck_desc[i].enable_reg, 1, &regval, 1);
					regval |= (1 << (ffs(buck_desc[i].enable_msk) - 1));
					i2c_write(saddr, buck_desc[i].enable_reg, 1, &regval, 1);


					/* set the regulator */
					if (value) {
						sel = regulator_map_voltage_linear_range(buck_desc + i, value, value);

						if (sel >= 0) {
							sel <<= ffs(buck_desc[i].vsel_msk) - 1;
							i2c_read(saddr, buck_desc[i].vsel_reg, 1, &regval, 1);
							regval = (regval & ~buck_desc[i].vsel_msk) | sel;
							i2c_write(saddr, buck_desc[i].vsel_reg, 1, &regval, 1);
						}
					}
					break;
				}
			}
		}

		if (strncmp(s, "LDO_REG", 7) == 0) {
			for (i = 0; i < num_ldo; ++i) {
				if (strcmp(ldo_desc[i].name, s) == 0) {
					/* enable the regulator */
					i2c_read(saddr, ldo_desc[i].enable_reg, 1, &regval, 1);
					regval |= (1 << (ffs(ldo_desc[i].enable_msk) - 1));
					i2c_write(saddr, ldo_desc[i].enable_reg, 1, &regval, 1);

					/* set the regulator */
					if (value) {
						sel = regulator_map_voltage_linear_range(ldo_desc + i, value, value);

						if (sel >= 0) {
							sel <<= ffs(ldo_desc[i].vsel_msk) - 1;
							i2c_read(saddr, ldo_desc[i].vsel_reg, 1, &regval, 1);
							regval = (regval & ~ldo_desc[i].vsel_msk) | sel;
							i2c_write(saddr, ldo_desc[i].vsel_reg, 1, &regval, 1);
						}
					}

					break;
				}
			}
		}
	}

	return 0;
}

int board_pmic_init(void)
{
	int i;

	for (i = 0; i < sizeof(global_compatible) / sizeof(global_compatible[0]); ++i)
		__board_pmic_init(global_compatible[i]);

	return 0;
}
