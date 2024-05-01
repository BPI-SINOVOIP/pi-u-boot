#ifndef __SY8810L_H__
#define __SY8810L_H__

enum SY8810L_reg {
	SY8810L_ID_DCDC1,
};

#define SPACEMIT_SY8810L_MAX_REG	0x2

#define SY8810L_BUCK_VSEL_MASK		0x3f
#define SY8810L_BUCK_EN_MASK		0x80

#define SY8810L_BUCK_CTRL_REG		0x1
#define SY8810L_BUCK_VSEL_REG		0x0

#define SY8810L_BUCK_LINER_RANGE					\
static const struct pm8xx_linear_range sy8810l_buck_ranges[] = {	\
        REGULATOR_LINEAR_RANGE(600000, 0x0, 0x5a, 10000),		\
};

#define SY8810L_REGULATOR_DESC		\
static const struct pm8xx_buck_desc sy8810l_buck_desc[] = {			\
	/* BUCK */		\
	PM8XX_DESC_COMMON(SY8810L_ID_DCDC1, "EDCDC_REG1",			\
			91, SY8810L_BUCK_VSEL_REG, SY8810L_BUCK_VSEL_MASK,	\
			SY8810L_BUCK_CTRL_REG, SY8810L_BUCK_EN_MASK,		\
			0, 0,							\
			sy8810l_buck_ranges),	\
};

#define SY8810L_REGULATOR_MATCH_DATA						\
struct regulator_match_data sy8810l_regulator_match_data = {			\
	.nr_buck_desc = ARRAY_SIZE(sy8810l_buck_desc),				\
	.buck_desc = sy8810l_buck_desc,						\
	.nr_ldo_desc = 0,							\
	.ldo_desc = NULL,							\
	.nr_switch_desc = 0,							\
	.switch_desc = NULL,							\
	.name = "sy8810l",							\
	.max_registers = 0x2,/* SPACEMIT_SY8810L_MAX_REG */			\
};

#define DECLEAR_SY8810L_REGULATOR_MATCH_DATA	extern struct regulator_match_data sy8810l_regulator_match_data;

#endif
