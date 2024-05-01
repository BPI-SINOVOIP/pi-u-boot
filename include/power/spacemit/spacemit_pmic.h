#ifndef __SPACEMIT_PMIC_H__
#define __SPACEMIT_PMIC_H__

#include <linux/kernel.h>
struct regulator_match_data;

struct pm8xx_priv {
	struct regulator_match_data *match;
};

struct pm8xx_linear_range {
	unsigned int min;
	unsigned int min_sel;
	unsigned int max_sel;
	unsigned int step;
};

struct pm8xx_buck_desc {
	const char *name;
	int n_voltages;
	int vsel_reg;
	int vsel_msk;
	int enable_reg;
	int enable_msk;
	int vsel_sleep_reg;
	int vsel_sleep_msk;
	int n_linear_ranges;
	const struct pm8xx_linear_range *linear_ranges;
};

/* regulator: match data */
struct regulator_match_data {
        int nr_buck_desc;
        const struct pm8xx_buck_desc *buck_desc;
        int nr_ldo_desc;
        const struct pm8xx_buck_desc *ldo_desc;
        int nr_switch_desc;
        const struct pm8xx_buck_desc *switch_desc;
	int max_registers;
        const char *name;
};

/* Initialize struct linear_range for regulators */
#define REGULATOR_LINEAR_RANGE(_min_uV, _min_sel, _max_sel, _step_uV)   \
{                                                                       \
        .min            = _min_uV,                                      \
        .min_sel        = _min_sel,                                     \
        .max_sel        = _max_sel,                                     \
        .step           = _step_uV,                                     \
}

/* common regulator defination */
#define PM8XX_DESC_COMMON(_id, _match, _nv, _vr, _vm, _er, _em, _vs, _vsm, _lr)		\
	[_id] = {							\
		.name		= (_match),				\
		.n_voltages     = (_nv),				\
		.vsel_reg       = (_vr),				\
		.vsel_msk       = (_vm),				\
		.vsel_sleep_reg = (_vs),				\
		.vsel_sleep_msk = (_vsm),				\
		.enable_reg	= (_er),				\
		.enable_msk	= (_em),				\
		.linear_ranges	= (_lr),				\
		.n_linear_ranges	= ARRAY_SIZE(_lr),		\
	}


#include "spm8821.h"
#include "pm853.h"
#include "sy8810l.h"

#endif /* __SPACEMIT_PMIC_H__ */
