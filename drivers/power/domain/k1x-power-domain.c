#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <errno.h>
#include <mapmem.h>
#include <regmap.h>
#include <power-domain-uclass.h>

#define MAX_REGMAP              5

#define MPMU_REGMAP_INDEX       0
#define APMU_REGMAP_INDEX       1

#define APMU_POWER_STATUS_REG   0xf0

enum pm_domain_id {
	K1X_PMU_VPU_PWR_DOMAIN,
	K1X_PMU_GPU_PWR_DOMAIN,
	K1X_PMU_LCD_PWR_DOMAIN,
	K1X_PMU_ISP_PWR_DOMAIN,
	K1X_PMU_AUD_PWR_DOMAIN,
	K1X_PMU_GNSS_PWR_DOMAIN,
	K1X_PMU_HDMI_PWR_DOMAIN,
};

struct pm_domain_desc {
	int reg_pwr_ctrl;
	int pm_qos;
	int bit_hw_mode;
	int bit_sleep2;
	int bit_sleep1;
	int bit_isolation;
	int bit_auto_pwr_on;
	int bit_hw_pwr_stat;
	int bit_pwr_stat;
	int use_hw;
	int pm_index;
};

struct spacemit_k1x_pd_platdata {
	struct regmap *regmap[MAX_REGMAP];
	struct pm_domain_desc *desc;
};

static struct pm_domain_desc k1x_pm_domain_desc[] = {
	[K1X_PMU_VPU_PWR_DOMAIN] = {
		.reg_pwr_ctrl = 0xa8,
		.pm_qos = 12,
		.bit_sleep2 = 3,
		.bit_sleep1 = 2,
		.bit_isolation = 1,
		.bit_pwr_stat = 1,
		.bit_hw_pwr_stat = 9,
		.pm_index = K1X_PMU_VPU_PWR_DOMAIN,
	},

	[K1X_PMU_GPU_PWR_DOMAIN] = {
		.reg_pwr_ctrl = 0xd0,
		.pm_qos = 12,
		.bit_sleep2 = 3,
		.bit_sleep1 = 2,
		.bit_isolation = 1,
		.bit_pwr_stat = 0,
		.pm_index = K1X_PMU_GPU_PWR_DOMAIN,
	},

	[K1X_PMU_LCD_PWR_DOMAIN] = {
		.reg_pwr_ctrl = 0x380,
		.pm_qos = 12,
		.bit_hw_mode = 4,
		.bit_sleep2 = 3,
		.bit_sleep1 = 2,
		.bit_isolation = 1,
		.bit_auto_pwr_on = 0,
		.bit_pwr_stat = 4,
		.bit_hw_pwr_stat = 12,
		.use_hw = 1,
		.pm_index = K1X_PMU_LCD_PWR_DOMAIN,
	},

	[K1X_PMU_ISP_PWR_DOMAIN] = {
		.reg_pwr_ctrl = 0x37c,
		.pm_qos = 12,
		.bit_hw_mode = 4,
		.bit_sleep2 = 3,
		.bit_sleep1 = 2,
		.bit_isolation = 1,
		.bit_auto_pwr_on = 0,
		.bit_pwr_stat = 2,
		.bit_hw_pwr_stat = 10,
		.pm_index = K1X_PMU_ISP_PWR_DOMAIN,
	},

	[K1X_PMU_AUD_PWR_DOMAIN] = {
		.reg_pwr_ctrl = 0x378,
		.pm_qos = 15,
		.bit_hw_mode = 4,
		.bit_sleep2 = 3,
		.bit_sleep1 = 2,
		.bit_isolation = 1,
		.bit_auto_pwr_on = 0,
		.bit_pwr_stat = 3,
		.bit_hw_pwr_stat = 11,
		.use_hw = 1,
		.pm_index = K1X_PMU_AUD_PWR_DOMAIN,
	},

	[K1X_PMU_GNSS_PWR_DOMAIN] = {
		.reg_pwr_ctrl = 0x13c,
		.pm_qos = 15,
		.bit_hw_mode = 4,
		.bit_sleep2 = 3,
		.bit_sleep1 = 2,
		.bit_isolation = 1,
		.bit_auto_pwr_on = 0,
		.bit_pwr_stat = 6,
		.bit_hw_pwr_stat = 14,
		.pm_index = K1X_PMU_GNSS_PWR_DOMAIN,
	},

	[K1X_PMU_HDMI_PWR_DOMAIN] = {
		.reg_pwr_ctrl = 0x3f4,
		.pm_qos = 12,
		.bit_hw_mode = 4,
		.bit_sleep2 = 3,
		.bit_sleep1 = 2,
		.bit_isolation = 1,
		.bit_auto_pwr_on = 0,
		.bit_pwr_stat = 7,
		.bit_hw_pwr_stat = 15,
		.use_hw = 1,
		.pm_index = K1X_PMU_HDMI_PWR_DOMAIN,
	},
};

static const struct udevice_id spacemit_power_domain_of_match[] = {
	{ .compatible = "spacemit,k1x-pm-domain", .data = (ulong)k1x_pm_domain_desc, },
	{ /* sentinel */ }
};

static int spacemit_power_domain_of_xlate(struct power_domain *pd, struct ofnode_phandle_args *args)
{
	struct spacemit_k1x_pd_platdata *priv = dev_get_priv(pd->dev);

	debug("%s(power_domain=%p, id=%d)\n", __func__, pd, args->args[0]);

	if (args->args_count < 1) {
		printf("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	pd->priv = (void *)(priv->desc + args->args[0]);
	pd->id = args->args[0];

	return 0;
}

static int k1x_pd_power_off(struct spacemit_k1x_pd_platdata *skp, struct pm_domain_desc *desc)
{
	unsigned int val;
	int loop;

	if (!desc->use_hw) {
		/* this is the sw type */
		regmap_read(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, &val);
		val &= ~(1 << desc->bit_isolation);
		regmap_write(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, val);

		udelay(15);

		/* mcu power off */
		regmap_read(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, &val);
		val &= ~((1 << desc->bit_sleep1) | (1 << desc->bit_sleep2));
		regmap_write(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, val);

		udelay(15);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(skp->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
			if ((val & (1 << desc->bit_pwr_stat)) == 0)
				break;
			udelay(5);
		}
	} else {
		/* LCD */
		regmap_read(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, &val);
		val &= ~(1 << desc->bit_auto_pwr_on);
		val &= ~(1 << desc->bit_hw_mode);
		regmap_write(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, val);

		udelay(30);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(skp->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
			if ((val & (1 << desc->bit_hw_pwr_stat)) == 0)
				break;
			udelay(5);
		}
	}

	if (loop < 0) {
		debug("power-off domain: %d, error\n", desc->pm_index);
		return -EBUSY;
	}

	return 0;
}

static int k1x_pd_power_on(struct spacemit_k1x_pd_platdata *skp, struct pm_domain_desc *desc)
{
	int loop;
	unsigned int val;

	regmap_read(skp->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
	if (val & (1 << desc->bit_pwr_stat))
		return 0;

	if (!desc->use_hw) {
		/* mcu power on */
		regmap_read(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, &val);
		val |= (1 << desc->bit_sleep1);
		regmap_write(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, val);

		udelay(25);

		regmap_read(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, &val);
		val |= (1 << desc->bit_sleep2) | (1 << desc->bit_sleep1);
		regmap_write(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, val);

		udelay(25);

		/* disable isolation */
		regmap_read(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, &val);
		val |= (1 << desc->bit_isolation);
		regmap_write(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, val);

		udelay(15);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(skp->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
			if (val & (1 << desc->bit_pwr_stat))
				break;
			udelay(6);
		}
	} else {
		/* LCD */
		regmap_read(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, &val);
		val |= (1 << desc->bit_auto_pwr_on);
		val |= (1 << desc->bit_hw_mode);
		regmap_write(skp->regmap[APMU_REGMAP_INDEX], desc->reg_pwr_ctrl, val);

		udelay(310);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(skp->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
			if (val & (1 << desc->bit_hw_pwr_stat))
				break;
			udelay(6);
		}
	}

	if (loop < 0) {
		pr_err("power-off domain: %d, error\n", desc->pm_index);
		return -EBUSY;
	}

	return 0;
}

static int spacemit_power_domain_on(struct power_domain *pd)
{
	struct pm_domain_desc *pd_priv = pd->priv;
	struct spacemit_k1x_pd_platdata *priv = dev_get_priv(pd->dev);

	debug("%s(pd=%p, id=%lu)\n", __func__, pd, pd->id);

	/* domain_on */
	k1x_pd_power_on(priv, pd_priv);

	return 0;
}

static int spacemit_power_domain_off(struct power_domain *pd)
{
	struct pm_domain_desc *pd_priv = pd->priv;
	struct spacemit_k1x_pd_platdata *priv = dev_get_priv(pd->dev);

	debug("%s(pd=%p, id=%lu)\n", __func__, pd, pd->id);
	/* domain_off */
	k1x_pd_power_off(priv, pd_priv);

	return 0;
}

static int spacemit_power_domain_probe(struct udevice *dev)
{
	int ret;
	struct spacemit_k1x_pd_platdata *priv = dev_get_priv(dev);
	ulong driver_data = dev_get_driver_data(dev);

	priv->desc = (struct pm_domain_desc *)driver_data;
	ret = regmap_init_mem_index(dev_ofnode(dev),
			&priv->regmap[MPMU_REGMAP_INDEX], 0);
	if (ret) {
		printf("%s:%d, error\n", __func__, __LINE__);
		return ret;
	}

	ret = regmap_init_mem_index(dev_ofnode(dev),
			&priv->regmap[APMU_REGMAP_INDEX], 1);
	if (ret) {
		printf("%s:%d, error\n", __func__, __LINE__);
		return ret;
	}
	return 0;
}

static struct power_domain_ops spacemit_power_domain_ops = {
	.on = spacemit_power_domain_on,
	.off = spacemit_power_domain_off,
	.of_xlate = spacemit_power_domain_of_xlate,
};

U_BOOT_DRIVER(spacemit_pm_domains) = {
	.name = "spacemit-k1x-pm-domains",
	.id = UCLASS_POWER_DOMAIN,
	.of_match = spacemit_power_domain_of_match,
	.probe = spacemit_power_domain_probe,
	.priv_auto = sizeof(struct spacemit_k1x_pd_platdata),
	.ops = &spacemit_power_domain_ops,
};
